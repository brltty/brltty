/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include "misc.h"

char **
splitString (const char *string, char delimiter) {
  char **array;

  {
    int count = 2;
    const char *character = string;
    while (*character)
      if (*character++ == delimiter)
        count++;
    if (!(array = malloc(count * sizeof(*array)))) return NULL;
  }

  {
    char **element = array;
    while (1) {
      const char *end = strchr(string, delimiter);
      int length = end? end-string: strlen(string);

      if (!(*element = malloc(length+1))) {
        *element = NULL;
        deallocateStrings(array);
        return NULL;
      }

      memcpy(*element, string, length);
      (*element++)[length] = 0;

      if (!end) break;
      string = end + 1;
    }
    *element = NULL;
  }
  return array;
}

void
deallocateStrings (char **array) {
  char **element = array;
  while (*element) free(*element++);
  free(array);
}

#ifdef HAVE_SYSLOG_H
   #include <syslog.h>
   static int syslogOpened = 0;
#endif /* HAVE_SYSLOG_H */
static int logLevel = LOG_INFO;
static int printLevel = LOG_NOTICE;
int loggedProblemCount = 0;

void
LogOpen (int toConsole) {
#ifdef HAVE_SYSLOG_H
  if (!syslogOpened) {
    int flags = LOG_PID;
    if (toConsole) flags |= LOG_CONS;
    openlog("brltty", flags, LOG_DAEMON);
    syslogOpened = 1;
  }
#endif /* HAVE_SYSLOG_H */
}

void
LogClose (void) {
#ifdef HAVE_SYSLOG_H
  if (syslogOpened) {
    syslogOpened = 0;
    closelog();
  }
#endif /* HAVE_SYSLOG_H */
}

void
LogPrint (int level, char *format, ...) {
  va_list argp;
  va_start(argp, format);

  if (level <= LOG_ERR) ++loggedProblemCount;

  if (level <= logLevel) {
#ifdef HAVE_SYSLOG_H
    if (syslogOpened) {
#ifdef HAVE_VSYSLOG
      vsyslog(level, format, argp);
#else /* HAVE_VSYSLOG */
      char buffer[0X100];
      vsnprintf(buffer, sizeof(buffer), format, argp);
      syslog(level, "%s", buffer);
#endif /* HAVE_VSYSLOG */
      goto done;
    }
#endif /* HAVE_SYSLOG_H */
    level = printLevel;
  }
done:

  if (level <= printLevel) {
    vfprintf(stderr, format, argp);
    fprintf(stderr, "\n");
    fflush(stderr);
  }

  va_end(argp);
}

void
LogError (const char *action) {
  LogPrint(LOG_ERR, "%s error %d: %s.", action, errno, strerror(errno));
}

void
LogBytes (const char *description, const unsigned char *data, unsigned int length) {
   if (length) {
      char buffer[(length * 3) + 1];
      char *out = buffer;
      const unsigned char *in = data;
      while (length--) out += sprintf(out, " %2.2X", *in++);
      LogPrint(LOG_DEBUG, "%s:%s", description, buffer);
   }
}

int
setLogLevel (int level) {
  int previous = logLevel;
  logLevel = level;
  return previous;
}

int
setPrintLevel (int level) {
  int previous = printLevel;
  printLevel = level;
  return previous;
}

int
setPrintOff (void) {
  return setPrintLevel(-1);
}

int
getConsole (void) {
  static int console = -1;
  if (console == -1) {
    if ((console = open("/dev/console", O_WRONLY)) != -1) {
      LogPrint(LOG_DEBUG, "Console opened: fd=%d", console);
    } else {
      LogError("Console open");
    }
  }
  return console;
}

int
writeConsole (const unsigned char *address, size_t count) {
  int console = getConsole();
  if (console != -1) {
    if (write(console, address, count) != -1) {
      return 1;
    } else {
      LogError("Console write");
    }
  }
  return 0;
}

int
ringBell (void) {
  static unsigned char bellSequence[] = {0X07};
  return writeConsole(bellSequence, sizeof(bellSequence));
}

static void
noMemory (void) {
  LogPrint(LOG_CRIT, "Insufficient memory: %s", strerror(errno));
  exit(3);
}

void *
mallocWrapper (size_t size) {
  void *address = malloc(size);
  if (!address && size) noMemory();
  return address;
}

void *
reallocWrapper (void *address, size_t size) {
  if (!(address = realloc(address, size)) && size) noMemory();
  return address;
}

char *
strdupWrapper (const char *string) {
  char *address = strdup(string);
  if (!address) noMemory();
  return address;
}

char *
makePath (const char *directory, const char *file) {
  const int count = 3;
  const char *components[count];
  int lengths[count];
  const int last = count - 1;
  int first = last;
  int length = 0;
  int index;
  char *path;
  components[last] = file;
  if (file[0] != '/') {
    if (directory && *directory) {
      if (directory[strlen(directory)-1] != '/') components[--first] = "/";
      components[--first] = directory;
    }
  }
  for (index=first; index<=last; ++index) {
    length += lengths[index] = strlen(components[index]);
  }
  path = mallocWrapper(length+1);
  {
    char *target = path;
    for (index=first; index<=last; ++index) {
      length = lengths[index];
      memcpy(target, components[index], length);
      target += length;
    }
    *target = 0;
  }
  return path;
}

int
makeDirectory (const char *path) {
  struct stat status;
  if (stat(path, &status) != -1) {
    if (S_ISDIR(status.st_mode)) return 1;
    LogPrint(LOG_ERR, "Not a directory: %s", path);
  } else if (errno != ENOENT) {
    LogPrint(LOG_ERR, "Directory status error: %s: %s", path, strerror(errno));
  } else {
    LogPrint(LOG_NOTICE, "Creating directory: %s", path);
    if (mkdir(path, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) != -1) return 1;
    LogPrint(LOG_ERR, "Directory creation error: %s: %s", path, strerror(errno));
  }
  return 0;
}

char *
getDevicePath (const char *path) {
  const char *prefix = DEVICE_DIRECTORY;
  const unsigned int prefixLength = strlen(prefix);
  const unsigned int pathLength = strlen(path);

  if (prefixLength) {
    if (*path != '/') {
      char buffer[prefixLength + 1 +  pathLength + 1];
      unsigned int length = 0;

      memcpy(&buffer[length], prefix, prefixLength);
      length += prefixLength;

      if (buffer[length-1] != '/') buffer[length++] = '/';

      memcpy(&buffer[length], path, pathLength);
      length += pathLength;

      buffer[length] = 0;
      return strdupWrapper(buffer);
    }
  }
  return strdupWrapper(path);
}

int
isQualifiedDevice (const char **path, const char *qualifier) {
  size_t count = strcspn(*path, ":");
  if (count == strlen(*path)) return 0;
  if (!qualifier) return 1;
  if (!count) return 0;

  {
    int ok = strncmp(*path, qualifier, count) == 0;
    if (ok) *path += count + 1;
    return ok;
  }
}

void
unsupportedDevice (const char *path) {
  LogPrint(LOG_WARNING, "Unsupported device: %s", path);
}

int
awaitInput (int descriptor, int milliseconds) {
  fd_set mask;
  struct timeval timeout;

  FD_ZERO(&mask);
  FD_SET(descriptor, &mask);

  memset(&timeout, 0, sizeof(timeout));
  timeout.tv_sec = milliseconds / 1000;
  timeout.tv_usec = (milliseconds % 1000) * 1000;

  while (1) {
    switch (select(descriptor+1, &mask, NULL, NULL, &timeout)) {
      case -1:
        if (errno == EINTR) continue;
        LogError("Input wait");
        return 0;

      case 0:
        if (milliseconds > 0)
          LogPrint(LOG_DEBUG, "Input wait timed out after %d %s.",
                   milliseconds, ((milliseconds == 1)? "millisecond": "milliseconds"));
        errno = EAGAIN;
        return 0;

      default:
        return 1;
    }
  }
}

int
readChunk (
  int descriptor,
  unsigned char *buffer, int *offset, int count,
  int initialTimeout, int subsequentTimeout
) {
  while (count > 0) {
    int amount = read(descriptor, buffer+*offset, count);

    if (amount == -1) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN) goto noInput;
      if (errno == EWOULDBLOCK) goto noInput;
      LogError("Read");
      return 0;
    }

    if (amount == 0) {
      int timeout;
    noInput:
      if ((timeout = *offset? subsequentTimeout: initialTimeout)) {
        if (awaitInput(descriptor, timeout)) continue;
        LogPrint(LOG_WARNING, "Input byte missing at offset %d.", *offset);
      } else {
        errno = EAGAIN;
      }
      return 0;
    }

    *offset += amount;
    count -= amount;
  }
  return 1;
}

int
changeOpenFlags (int fileDescriptor, int clear, int set) {
  int flags;
  if ((flags = fcntl(fileDescriptor, F_GETFL)) != -1) {
    flags &= ~clear;
    flags |= set;
    if (fcntl(fileDescriptor, F_SETFL, flags) != -1) {
      return 1;
    } else {
      LogError("F_SETFL");
    }
  } else {
    LogError("F_GETFL");
  }
  return 0;
}

int
setOpenFlags (int fileDescriptor, int state, int flags) {
  if (state) {
    return changeOpenFlags(fileDescriptor, 0, flags);
  } else {
    return changeOpenFlags(fileDescriptor, flags, 0);
  }
}

int
setBlockingIo (int fileDescriptor, int state) {
  return setOpenFlags(fileDescriptor, state, O_NONBLOCK);
}

int
setCloseOnExec (int fileDescriptor) {
  return fcntl(fileDescriptor, F_SETFD, FD_CLOEXEC) != -1;
}

int
readLine (FILE *file, char **buffer, size_t *size) {
  char *line;

  if (ferror(file)) return 0;
  if (feof(file)) return 0;
  if (!*size) *buffer = mallocWrapper(*size = 0X80);

  if ((line = fgets(*buffer, *size, file))) {
    size_t length = strlen(line); /* Line length including new-line. */

    /* No trailing new-line means that the buffer isn't big enough. */
    while (line[length-1] != '\n') {
      /* If necessary, extend the buffer. */
      if ((*size - (length + 1)) == 0)
        *buffer = reallocWrapper(*buffer, (*size <<= 1));

      /* Read the rest of the line into the end of the buffer. */
      if (!(line = fgets(&(*buffer)[length], *size-length, file))) {
        if (!ferror(file)) return 1;
        LogError("read");
        return 0;
      }

      length += strlen(line); /* New total line length. */
      line = *buffer; /* Point to the beginning of the line. */
    }

    line[--length] = 0; /* Remove trailing new-line. */
    return 1;
  } else if (ferror(file)) {
    LogError("read");
  }

  return 0;
}

/* Process each line of an input text file safely.
 * This routine handles the actual reading of the file,
 * insuring that the input buffer is always big enough,
 * and calls a caller-supplied handler once for each line in the file.
 * The caller-supplied data pointer is passed straight through to the handler.
 */
int
processLines (FILE *file, int (*handler) (char *line, void *data), void *data) {
  char *buffer = NULL;
  size_t bufferSize = 0;

  while (readLine(file, &buffer, &bufferSize))
    if (!handler(buffer, data))
      break;

  if (buffer) free(buffer);
  return !ferror(file);
}

size_t safe_write (int fd, const unsigned char *buffer, size_t length) {
  const unsigned char *address = buffer;

  while (length > 0) {
    size_t count = write(fd, address, length);

    if (count == -1) {
      if (errno == EINTR) continue;
      if (errno != EAGAIN) return -1;
      count = 0;
    }

    if (count) {
      address += count;
      length -= count;
    } else {
      delay(100);
    }
  }
  return address - buffer;
}

void
delay (int milliseconds) {
  if (milliseconds > 0) {
    struct timeval timeout;
    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;
    select(0, NULL, NULL, NULL, &timeout);
  }
}

long int
millisecondsBetween (const struct timeval *from, const struct timeval *to) {
  return ((to->tv_sec - from->tv_sec) * 1000) + ((to->tv_usec - from->tv_usec) / 1000);
}

long int
millisecondsSince (const struct timeval *from) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return millisecondsBetween(from, &now);
}

void
shortdelay (int milliseconds) {
  static int tickLength = 0;
  struct timeval start;
  gettimeofday(&start, NULL);
  if (!tickLength)
    if (!(tickLength = 1000 / sysconf(_SC_CLK_TCK)))
      tickLength = 1;
  if (milliseconds >= tickLength) delay(milliseconds / tickLength * tickLength);
  while (millisecondsSince(&start) < milliseconds);
}

int
timeout_yet (int milliseconds) {
  static struct timeval start = {0, 0};

  if (milliseconds) {
    return millisecondsSince(&start) >= milliseconds;
  }

  gettimeofday(&start, NULL);
  return 0;
}

int
isInteger (int *value, const char *word) {
  if (*word) {
    char *end;
    *value = strtol(word, &end, 0);
    if (!*end) return 1;
  }
  return 0;
}

int
isFloat (float *value, const char *word) {
  if (*word) {
    char *end;
    *value = strtod(word, &end);
    if (!*end) return 1;
  }
  return 0;
}

int
validateInteger (int *value, const char *description, const char *word, const int *minimum, const int *maximum) {
   if (*word && !isInteger(value, word)) {
      LogPrint(LOG_ERR, "The %s must be an integer: %s",
               description, word);
      return 0;
   }
   if (minimum) {
      if (*value < *minimum) {
         LogPrint(LOG_ERR, "The %s must not be less than %d: %d",
                  description, *minimum, *value);
         return 0;
      }
   }
   if (maximum) {
      if (*value > *maximum) {
         LogPrint(LOG_ERR, "The %s must not be greater than %d: %d",
                  description, *maximum, *value);
         return 0;
      }
   }
   return 1;
}

int
validateFloat (float *value, const char *description, const char *word, const float *minimum, const float *maximum) {
   if (*word && !isFloat(value, word)) {
      LogPrint(LOG_ERR, "The %s must be a floating-point number: %s",
               description, word);
      return 0;
   }
   if (minimum) {
      if (*value < *minimum) {
         LogPrint(LOG_ERR, "The %s must not be less than %g: %g",
                  description, *minimum, *value);
         return 0;
      }
   }
   if (maximum) {
      if (*value > *maximum) {
         LogPrint(LOG_ERR, "The %s must not be greater than %g: %g",
                  description, *maximum, *value);
         return 0;
      }
   }
   return 1;
}

int
validateChoice (unsigned int *value, const char *description, const char *word, const char *const *choices) {
   int length = strlen(word);
   *value = 0;
   if (length) {
      int index = 0;
      while (choices[index]) {
         if (length <= strlen(choices[index])) {
            if (strncasecmp(word, choices[index], length) == 0) {
               *value = index;
               return 1;
            }
         }
         ++index;
      }
      LogPrint(LOG_ERR, "Unsupported %s: %s", description, word);
      return 0;
   }
   return 1;
}

int
validateFlag (unsigned int *value, const char *description, const char *word, const char *on, const char *off) {
   const char *choices[] = {off, on, NULL};
   return validateChoice(value, description, word, choices);
}

int
validateOnOff (unsigned int *value, const char *description, const char *word) {
   return validateFlag(value, description, word, "on", "off");
}

int
validateYesNo (unsigned int *value, const char *description, const char *word) {
   return validateFlag(value, description, word, "yes", "no");
}

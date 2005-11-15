/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#include "prologue.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */

#include "misc.h"

char **
splitString (const char *string, char delimiter, int *count) {
  char **array = NULL;

  if (string) {
    while (1) {
      const char *start = string;
      int index = 0;

      while (1) {
        const char *end = strchr(start, delimiter);
        int length = end? end-start: strlen(start);

        if (array) {
          char *element = mallocWrapper(length+1);
          memcpy(element, start, length);
          element[length] = 0;
          array[index] = element;
        }
        index++;

        if (!end) break;
        start = end + 1;
      }

      if (array) {
        array[index] = NULL;
        if (count) *count = index;
        break;
      }

      array = mallocWrapper((index + 1) * sizeof(*array));
    }
  } else if (count) {
    *count = 0;
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
static const char *printPrefix = NULL;
static int printLevel = LOG_NOTICE;

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
LogPrint (int level, const char *format, ...) {
  va_list argp;

  if (level <= logLevel) {
#ifdef HAVE_SYSLOG_H
    if (syslogOpened) {
#ifdef HAVE_VSYSLOG
      va_start(argp, format);
      vsyslog(level, format, argp);
      va_end(argp);
#else /* HAVE_VSYSLOG */
      char buffer[0X100];
      va_start(argp, format);
      vsnprintf(buffer, sizeof(buffer), format, argp);
      va_end(argp);
      syslog(level, "%s", buffer);
#endif /* HAVE_VSYSLOG */
      goto done;
    }
    level = printLevel;
#endif /* HAVE_SYSLOG_H */
  }
#ifdef HAVE_SYSLOG_H
done:
#endif /* HAVE_SYSLOG_H */

  if (level <= printLevel) {
    FILE *stream = stderr;
    if (printPrefix) fprintf(stream, "%s: ", printPrefix);
    va_start(argp, format);
    vfprintf(stream, format, argp);
    va_end(argp);
    fprintf(stream, "\n");
    fflush(stream);
  }
}

void
LogError (const char *action) {
  LogPrint(LOG_ERR, "%s error %d: %s.", action, errno, strerror(errno));
}

#ifdef WINDOWS
void
LogWindowsCodeError (DWORD error, const char *action) {
  char *message;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (char *)&message, 0, NULL);

  {
    char *end = strpbrk(message, "\r\n");
    if (end) *end = 0;
  }

  LogPrint(LOG_ERR, "%s error %ld: %s", action, error, message);
  LocalFree(message);
}
void
LogWindowsError (const char *action) {
  DWORD error = GetLastError();
  LogWindowsCodeError(error, action);
}
void
LogWindowsSocketError(const char *action) {
  DWORD error = WSAGetLastError();
  LogWindowsCodeError(error, action);
}
#endif /* WINDOWS */

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

const char *
setPrintPrefix (const char *prefix) {
  const char *previous = printPrefix;
  printPrefix = prefix;
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
getWorkingDirectory (void) {
  size_t size = 0X80;
  char *buffer = NULL;

  do {
    buffer = reallocWrapper(buffer, size<<=1);

    if (getcwd(buffer, size)) return buffer;
  } while (errno == ERANGE);

  if (buffer) free(buffer);
  return NULL;
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

void
fixPath (char **path, const char *extension, const char *prefix) {
  const unsigned int prefixLength = strlen(prefix);
  const unsigned int pathLength = strlen(*path);
  const unsigned int extensionLength = strlen(extension);
  char buffer[prefixLength + pathLength + extensionLength + 1];
  unsigned int length = 0;

  if (prefixLength) {
    if (!strchr(*path, '/')) {
      if (strncmp(*path, prefix, prefixLength) != 0) {
        memcpy(&buffer[length], prefix, prefixLength);
        length += prefixLength;
      }
    }
  }

  memcpy(&buffer[length], *path, pathLength);
  length += pathLength;

  if (extensionLength) {
    if ((pathLength < extensionLength) ||
        (memcmp(&buffer[length-extensionLength], extension, extensionLength) != 0)) {
      memcpy(&buffer[length], extension, extensionLength);
      length += extensionLength;
    }
  }

  if (length > pathLength) {
    buffer[length] = 0;
    *path = strdupWrapper(buffer);
  }
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
    if (mkdir(path
#ifndef __MINGW32__
              , S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH
#endif /* __MINGW32__ */
             ) != -1) return 1;
    LogPrint(LOG_ERR, "Directory creation error: %s: %s", path, strerror(errno));
  }
  return 0;
}

const char *
getDeviceDirectory (void) {
  static const char *deviceDirectory = NULL;

  if (!deviceDirectory) {
    const char *directory = DEVICE_DIRECTORY;
    const unsigned int directoryLength = strlen(directory);

    static const char *const variables[] = {"DTDEVROOT", "UTDEVROOT", NULL};
    const char *const*variable = variables;

    while (*variable) {
      const char *root = getenv(*variable);

      if (root && *root) {
        const unsigned int rootLength = strlen(root);
        char path[rootLength + directoryLength + 1];
        snprintf(path, sizeof(path), "%s%s", root, directory);
        if (access(path, F_OK) != -1) {
          deviceDirectory = strdupWrapper(path);
          goto found;
        }

        if (errno != ENOENT)
          LogPrint(LOG_ERR, "device directory error: %s (%s): %s",
                   path, *variable, strerror(errno));
      }

      ++variable;
    }

    deviceDirectory = directory;
  found:
    LogPrint(LOG_DEBUG, "device directory: %s", deviceDirectory);
  }

  return deviceDirectory;
}

char *
getDevicePath (const char *device) {
  const char *directory = getDeviceDirectory();
  const unsigned int directoryLength = strlen(directory);
  const unsigned int deviceLength = strlen(device);

  if (directoryLength) {
    if (*device != '/') {
      char path[directoryLength + 1 +  deviceLength + 1];
      unsigned int length = 0;

      memcpy(&path[length], directory, directoryLength);
      length += directoryLength;

      if (path[length-1] != '/') path[length++] = '/';

      memcpy(&path[length], device, deviceLength);
      length += deviceLength;

      path[length] = 0;
      return strdupWrapper(path);
    }
  }
  return strdupWrapper(device);
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

    if (--length > 0)
      if (line[length-1] == '\r')
        --length;
    line[length] = 0; /* Remove trailing new-line. */
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

void
formatInputError (char *buffer, int size, const char *file, const int *line, const char *format, va_list argp) {
  int length = 0;

  if (file) {
    int count;
    snprintf(&buffer[length], size-length, "%s%n", file, &count);
    length += count;
  }

  if (line) {
    int count;
    snprintf(&buffer[length], size-length, "[%d]%n", *line, &count);
    length += count;
  }

  if (length) {
    int count;
    snprintf(&buffer[length], size-length, ": %n", &count);
    length += count;
  }

  vsnprintf(&buffer[length], size-length, format, argp);
}

void
approximateDelay (int milliseconds) {
  if (milliseconds > 0) {
#ifdef WINDOWS
#ifdef __CYGWIN__
    usleep(milliseconds*1000);
#else /* __CYGWIN__ */
    Sleep(milliseconds);
#endif /* __CYGWIN__ */
#else /* WINDOWS */
    struct timeval timeout;
    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;
    select(0, NULL, NULL, NULL, &timeout);
#endif /* WINDOWS */
  }
}

#ifdef __MINGW32__
void
gettimeofday (struct timeval *tvp, void *tzp) {
  DWORD time = GetTickCount();
  /* this is not 49.7 days-proof ! */
  tvp->tv_sec = time / 1000;
  tvp->tv_usec = (time % 1000) * 1000;
}

void
usleep (int usec) {
  if (usec > 0) {
    approximateDelay((usec+999)/1000);
  }
}
#endif /* __MINGW32__ */

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
accurateDelay (int milliseconds) {
  static int tickLength = 0;
  struct timeval start;
  gettimeofday(&start, NULL);
  if (!tickLength) {
#if defined(_SC_CLK_TCK)
    tickLength = 1000 / sysconf(_SC_CLK_TCK);
#elif defined(CLK_TCK)
    tickLength = 1000 / CLK_TCK;
#elif defined(HZ)
    tickLength = 1000 / HZ;
#else
#error cannot determine tick length
#endif /* tick length */
    if (!tickLength) tickLength = 1;
  }
  if (milliseconds >= tickLength) approximateDelay(milliseconds / tickLength * tickLength);
  while (millisecondsSince(&start) < milliseconds);
}

int
hasTimedOut (int milliseconds) {
  static struct timeval start = {0, 0};

  if (milliseconds) return millisecondsSince(&start) >= milliseconds;

  gettimeofday(&start, NULL);
  return 1;
}

int
rescaleInteger (int value, int from, int to) {
  return (to * (value + (from / (to * 2)))) / from;
}

int
isInteger (int *value, const char *string) {
  if (*string) {
    char *end;
    long l = strtol(string, &end, 0);
    if (!*end) {
      *value = l;
      return 1;
    }
  }
  return 0;
}

int
isFloat (float *value, const char *string) {
  if (*string) {
    char *end;
    double d = strtod(string, &end);
    if (!*end) {
      *value = d;
      return 1;
    }
  }
  return 0;
}

int
validateInteger (int *value, const char *string, const int *minimum, const int *maximum) {
  int i;
  if (*string && !isInteger(&i, string)) return 0;
  if (minimum && (*value < *minimum)) return 0;
  if (maximum && (*value > *maximum)) return 0;
  *value = i;
  return 1;
}

int
validateFloat (float *value, const char *string, const float *minimum, const float *maximum) {
  float f;
  if (*string && !isFloat(&f, string)) return 0;
  if (minimum && (*value < *minimum)) return 0;
  if (maximum && (*value > *maximum)) return 0;
  *value = f;
  return 1;
}

int
validateChoice (unsigned int *value, const char *string, const char *const *choices) {
  int length = strlen(string);
  *value = 0;
  if (!length) return 1;

  {
    int index = 0;
    while (choices[index]) {
      if (strncasecmp(string, choices[index], length) == 0) {
        *value = index;
        return 1;
      }
      ++index;
    }
  }

  return 0;
}

int
validateFlag (unsigned int *value, const char *string, const char *on, const char *off) {
  const char *choices[] = {off, on, NULL};
  return validateChoice(value, string, choices);
}

int
validateOnOff (unsigned int *value, const char *string) {
  return validateFlag(value, string, "on", "off");
}

int
validateYesNo (unsigned int *value, const char *string) {
  return validateFlag(value, string, "yes", "no");
}

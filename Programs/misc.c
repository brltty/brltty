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

/* misc.c - Miscellaneous all-purpose routines
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
#include <termios.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "misc.h"
#include "brl.h"

#ifdef HAVE_SYSLOG_H
   #include <syslog.h>
   static int syslogOpened = 0;
#endif /* HAVE_SYSLOG_H */
static int logLevel = LOG_INFO;
static int printLevel = LOG_NOTICE;
int loggedProblemCount = 0;

void
LogOpen(int toConsole)
{
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
LogClose(void)
{
#ifdef HAVE_SYSLOG_H
  if (syslogOpened) {
    syslogOpened = 0;
    closelog();
  }
#endif /* HAVE_SYSLOG_H */
}

void
LogPrint (int level, char *format, ...)
{
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
LogError (const char *action)
{
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

void
setLogLevel(int level)
{
  logLevel = level;
}

void
setPrintLevel(int level)
{
  printLevel = level;
}

void
setPrintOff(void)
{
  setPrintLevel(-1);
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
      }
      LogError("Console write");
   }
   return 0;
}

int
ringBell (void) {
   static unsigned char bellSequence[] = {0X07};
   return writeConsole(bellSequence, sizeof(bellSequence));
}

static void
noMemory (void)
{
   LogPrint(LOG_CRIT, "Insufficient memory: %s", strerror(errno));
   exit(3);
}

void *
mallocWrapper (size_t size) {
   void *address = malloc(size);
   if (!address)
      noMemory();
   return address;
}

void *
reallocWrapper (void *address, size_t size) {
   if (!(address = realloc(address, size)))
      noMemory();
   return address;
}

char *
strdupWrapper (const char *string) {
   char *address = strdup(string);
   if (!address)
      noMemory();
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

static char *
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
isSerialDevice (const char **path) {
  const char *prefix = "serial:";
  int length = strlen(prefix);
  if (strncmp(*path, prefix, length) == 0) {
    *path += length;
    return 1;
  }
  return !strchr(*path, ':');
}

void
unsupportedDevice (const char *path) {
  LogPrint(LOG_WARNING, "Unsupported device: %s", path);
}

int
openSerialDevice (const char *path, int *descriptor, struct termios *attributes) {
  char *device;
  if ((device = getDevicePath(path))) {
    if ((*descriptor = open(device, O_RDWR|O_NOCTTY|O_NONBLOCK)) != -1) {
      if (isatty(*descriptor)) {
        int flags;
        if ((flags = fcntl(*descriptor, F_GETFL)) != -1) {
          flags &= ~O_NONBLOCK;
          if (fcntl(*descriptor, F_SETFL, flags) != -1) {
            if (!attributes || (tcgetattr(*descriptor, attributes) != -1)) {
              LogPrint(LOG_DEBUG, "Serial device opened: %s: fd=%d", device, *descriptor);
              free(device);
              return 1;
            } else {
              LogPrint(LOG_ERR, "Cannot get attributes for '%s': %s", device, strerror(errno));
            }
          } else {
            LogError("F_SETFL");
          }
        } else {
          LogError("F_GETFL");
        }
      } else {
        LogPrint(LOG_ERR, "Not a serial device: %s", device);
      }
      close(*descriptor);
      *descriptor = -1;
    } else {
      LogPrint(LOG_ERR, "Cannot open '%s': %s", device, strerror(errno));
    }
    free(device);
  }
  return 0;
}

void
rawSerialDevice (struct termios *attributes) {
  attributes->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  attributes->c_oflag &= ~OPOST;
  attributes->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  attributes->c_cflag &= ~(CSIZE | PARENB);
  attributes->c_cflag |= CS8;
}

int
setSerialDevice (int descriptor, struct termios *attributes, speed_t baud) {
  if (cfsetispeed(attributes, baud) != -1) {
    if (cfsetospeed(attributes, baud) != -1) {
      if (tcsetattr(descriptor, TCSANOW, attributes) != -1) {
        return 1;
      } else {
        LogError("Serial device attributes set");
      }
    } else {
      LogError("Serial device output speed set");
    }
  } else {
    LogError("Serial device input speed set");
  }
  return 0;
}

int
resetSerialDevice (int descriptor, struct termios *attributes, speed_t baud) {
  if (tcflush(descriptor, TCOFLUSH) != -1) {
    if (setSerialDevice(descriptor, attributes, B0)) {
      delay(500);
      if (tcflush(descriptor, TCIFLUSH) != -1) {
        if (setSerialDevice(descriptor, attributes, baud)) {
          return 1;
        }
      } else {
        LogError("TCIFLUSH");
      }
    }
  } else {
    LogError("TCOFLUSH");
  }
  return 0;
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
        LogPrint(LOG_DEBUG, "Input wait timed out after %d %s.",
                 milliseconds, ((milliseconds == 1)? "millisecond": "milliseconds"));
        return 0;
      default:
        return 1;
    }
  }
}

int
readChunk (int descriptor, unsigned char *buffer, int *offset, int count, int timeout) {
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
    noInput:
      if (*offset) {
        if (awaitInput(descriptor, timeout)) continue;
        LogPrint(LOG_WARNING, "Input byte missing at offset %d.", *offset);
      }
      return 0;
    }
    *offset += amount;
    count -= amount;
  }
  return 1;
}

void
setCloseOnExec (int fileDescriptor) {
   fcntl(fileDescriptor, F_SETFD, FD_CLOEXEC);
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

/* Write data safely by continually retrying the write system call until
 * all of the requested data has been transferred.
 * This routine is a wrapper for the write system call which is fully compatible
 * with it except that the caller does not need to handle the various
 * scenarios which can occur when a signal interrupts the system call.
 */
size_t safe_write (int fd, const unsigned char *buffer, size_t length)
{
  const unsigned char *address = buffer;

  /* Keep on looping while there's still some data to be written. */
  while (length > 0)
    {
      /* Write the rest of the data. */
      size_t count = write(fd, address, length);

      /* Handle errors. */
      if (count == -1)
        {
          /* If the system call was interrupted by a signal, then restart it. */
          if (errno == EINTR)
            {
              continue;
            }

          /* Return all other errors to the caller. */
          return -1;
        }

      /* In case the system call was interrupted by a signal
       * after some, but not all, of the data was written,
       * point to the remainder of the buffer and try again.
       */
      address += count;
      length -= count;
    }

  /* Return the number of bytes which were actually written. */
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
elapsedMilliseconds (const struct timeval *from, const struct timeval *to) {
  return ((to->tv_sec - from->tv_sec) * 1000) + ((to->tv_usec - from->tv_usec) / 1000);
}

void
shortdelay (int milliseconds) {
  static int tickLength = 0;
  struct timeval start;
  gettimeofday(&start, NULL);
  if (!tickLength)
    if (!(tickLength = 1000 / sysconf(_SC_CLK_TCK)))
      tickLength = 1;
  if (milliseconds >= tickLength) {
    delay(milliseconds / tickLength * tickLength);
  }
  while (1) {
    struct timeval now;
    gettimeofday(&now, NULL);
    if (elapsedMilliseconds(&start, &now) >= milliseconds) break;
  }
}

int
timeout_yet (int milliseconds) {
  static struct timeval start = {0, 0};

  if (milliseconds) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return elapsedMilliseconds(&start, &now) >= milliseconds;
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
isFloat (double *value, const char *word) {
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
validateFloat (double *value, const char *description, const char *word, const double *minimum, const double *maximum) {
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

typedef struct {
  int integer;
  speed_t baud;
} BaudEntry;
static BaudEntry baudTable[] = {
   #ifdef B50
   {     50, B50},
   #endif
   #ifdef B75
   {     75, B75},
   #endif
   #ifdef B110
   {    110, B110},
   #endif
   #ifdef B134
   {    134, B134},
   #endif
   #ifdef B150
   {    150, B150},
   #endif
   #ifdef B200
   {    200, B200},
   #endif
   #ifdef B300
   {    300, B300},
   #endif
   #ifdef B600
   {    600, B600},
   #endif
   #ifdef B1200
   {   1200, B1200},
   #endif
   #ifdef B1800
   {   1800, B1800},
   #endif
   #ifdef B2400
   {   2400, B2400},
   #endif
   #ifdef B4800
   {   4800, B4800},
   #endif
   #ifdef B9600
   {   9600, B9600},
   #endif
   #ifdef B19200
   {  19200, B19200},
   #endif
   #ifdef B38400
   {  38400, B38400},
   #endif
   #ifdef B57600
   {  57600, B57600},
   #endif
   #ifdef B115200
   { 115200, B115200},
   #endif
   #ifdef B230400
   { 230400, B230400},
   #endif
   #ifdef B460800
   { 460800, B460800},
   #endif
   #ifdef B500000
   { 500000, B500000},
   #endif
   #ifdef B576000
   { 576000, B576000},
   #endif
   #ifdef B921600
   { 921600, B921600},
   #endif
   #ifdef B1000000
   {1000000, B1000000},
   #endif
   #ifdef B1152000
   {1152000, B1152000},
   #endif
   #ifdef B1500000
   {1500000, B1500000},
   #endif
   #ifdef B2000000
   {2000000, B2000000},
   #endif
   #ifdef B2500000
   {2500000, B2500000},
   #endif
   #ifdef B3000000
   {3000000, B3000000},
   #endif
   #ifdef B3500000
   {3500000, B3500000},
   #endif
   #ifdef B4000000
   {4000000, B4000000},
   #endif
   {      0, B0}
};

int
validateBaud (speed_t *value, const char *description, const char *word, const unsigned int *choices) {
   int integer;
   if (!*word || isInteger(&integer, word)) {
      BaudEntry *entry = baudTable;
      while (entry->integer) {
         if (integer == entry->integer) {
            if (choices) {
               while (*choices) {
                  if (integer == *choices) {
                     break;
                  }
                  ++choices;
               }
               if (!*choices) {
                  LogPrint(LOG_ERR, "Unsupported %s: %d",
                           description, integer);
                  return 0;
               }
            }
            *value = entry->baud;
            return 1;
         }
         ++entry;
      }
   }
   LogPrint(LOG_ERR, "Invalid %s: %d",
            description, integer);
   return 0;
}

int
baud2integer (speed_t baud) {
  BaudEntry *entry = baudTable;
  while (entry->integer) {
    if (baud == entry->baud)
      return entry->integer;
    ++entry;
  }
  return -1;
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

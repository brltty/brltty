/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

#define __EXTENSIONS__        /* for time.h */

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
#include <linux/kd.h>

#include "misc.h"
#include "config.h"
#include "common.h"

#ifdef USE_SYSLOG
   #include <syslog.h>
   static int syslogOpened = 0;
#endif
static int logLevel = LOG_INFO;
static int stderrLevel = LOG_NOTICE;
int ProblemCount = 0;

void
LogOpen(int toConsole)
{
#ifdef USE_SYSLOG
  if (!syslogOpened) {
    int flags = LOG_PID;
    if (toConsole) flags |= LOG_CONS;
    openlog("brltty", flags, LOG_DAEMON);
    syslogOpened = 1;
  }
#endif
}

void
LogClose(void)
{
#ifdef USE_SYSLOG
  if (syslogOpened) {
    syslogOpened = 0;
    closelog();
  }
#endif
}

void
LogPrint (int level, char *format, ...)
{
  va_list argp;
  va_start(argp, format);

  if (level <= LOG_ERR)
    ++ProblemCount;

  if (level <= logLevel) {
#ifdef USE_SYSLOG
    if (syslogOpened) {
      vsyslog(level, format, argp);
      goto done;
    }
#endif
    level = stderrLevel;
  }
done:

  if (level <= stderrLevel) {
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
      unsigned char buffer[(length * 3) + 1];
      unsigned char *out = buffer;
      const unsigned char *in = data;
      while (length--) out += sprintf(out, " %2.2X", *in++);
      LogPrint(LOG_DEBUG, "%s:%s", description, buffer);
   }
}

void
SetLogLevel(int level)
{
  logLevel = level;
}

void
SetStderrLevel(int level)
{
  stderrLevel = level;
}

void
SetStderrOff(void)
{
  SetStderrLevel(-1);
}

int
getConsole (void) {
   static int console = -1;
   if (console == -1) {
      if ((console = open("/dev/tty0", O_WRONLY)) != -1) {
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

int
timedBeep (unsigned short frequency, unsigned short milliseconds) {
   if (ioctl(getConsole(), KDMKTONE,
             ((milliseconds << 0X10) | (1190000 / frequency))) == -1) {
      LogError("ioctl KDMKTONE");
      return 0;
   }
   return 1;
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

int
openSerialDevice (const char *path, int *descriptor, struct termios *attributes) {
  if ((*descriptor = open(path, O_RDWR|O_NOCTTY)) != -1) {
    if (!attributes || (tcgetattr(*descriptor, attributes) != -1)) return 1;
    LogPrint(LOG_ERR, "Cannot get attributes for '%s': %s", path, strerror(errno));
    close(*descriptor);
    *descriptor = -1;
  } else {
    LogPrint(LOG_ERR, "Cannot open '%s': %s", path, strerror(errno));
  }
  return 0;
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
  if (setSerialDevice(descriptor, attributes, B0)) {
    if (tcflush(descriptor, TCIOFLUSH) != -1) {
      if (setSerialDevice(descriptor, attributes, baud)) {
        return 1;
      }
    } else {
      LogError("Serial device flush");
    }
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

// Process each line of an input text file safely.
// This routine handles the actual reading of the file,
// insuring that the input buffer is always big enough,
// and calls a caller-supplied handler once for each line in the file.
// The caller-supplied data pointer is passed straight through to the handler.
int
processLines (FILE *file, void (*handler) (char *line, void *data), void *data) {
  int ok = 1;
  size_t buff_size = 0X80; // Initial buffer size.
  char *buff_addr = mallocWrapper(buff_size); // Allocate the buffer.

  // Keep looping, once per line, until end-of-file.
  while (1) {
    char *line; // Will point to each line that is read.
    if ((line = fgets(buff_addr, buff_size, file)) != NULL) {
      size_t line_len = strlen(line); // Line length including new-line.

      // No trailing new-line means that the buffer isn't big enough.
      while (line[line_len-1] != '\n') {
        // Extend the buffer, keeping track of its new size.
        buff_addr = reallocWrapper(buff_addr, (buff_size <<= 1));

        // Read the rest of the line into the end of the buffer.
        if (!(line = fgets(buff_addr+line_len, buff_size-line_len, file))) {
          if (ferror(file))
            LogPrint(LOG_ERR, "File read error: %s", strerror(errno));
          else
            LogPrint(LOG_ERR, "Incomplete last line.");
          ok = 0;
          goto done;
        }

        line_len += strlen(line); // New total line length.
        line = buff_addr; // Point to the beginning of the line.
      }
      line[--line_len] = 0; // Remove trailing new-line.

      handler(line, data);
    } else {
      if (ferror(file)) {
        LogPrint(LOG_ERR, "File read error: %s", strerror(errno));
        ok = 0;
      }
      break;
    }
  }
done:

  // Deallocate the buffer.
  free(buff_addr);

  return ok;
}

// Write data safely by continually retrying the write system call until
// all of the requested data has been transferred.
// This routine is a wrapper for the write system call which is fully compatible
// with it except that the caller does not need to handle the various
// scenarios which can occur when a signal interrupts the system call.
size_t safe_write (int fd, const unsigned char *buffer, size_t length)
{
  const unsigned char *address = buffer;

  // Keep on looping while there's still some data to be written.
  while (length > 0)
    {
      // Write the rest of the data.
      size_t count = write(fd, address, length);

      // Handle errors.
      if (count == -1)
        {
          // If the system call was interrupted by a signal, then restart it.
          if (errno == EINTR)
            {
              continue;
            }

          // Return all other errors to the caller.
          return -1;
        }

      // In case the system call was interrupted by a signal
      // after some, but not all, of the data was written,
      // point to the remainder of the buffer and try again.
      address += count;
      length -= count;
    }

  // Return the number of bytes which were actually written.
  return address - buffer;
}

unsigned long
elapsed_msec (struct timeval *t1, struct timeval *t2)
{
  unsigned long diff, error = 0xFFFFFFFF;
  if (t1->tv_sec > t2->tv_sec)
    return error;
  diff = (t2->tv_sec - t1->tv_sec) * 1000L;
  if (diff == 0 && t1->tv_usec > t2->tv_usec)
    return error;
  diff += (t2->tv_usec - t1->tv_usec) / 1000L;
  return diff;
}

void
shortdelay (unsigned msec)
{
  struct timeval start, now;
  struct timezone tz;
  gettimeofday(&start, &tz);
  do
    {
      gettimeofday(&now, &tz);
    }
  while (elapsed_msec(&start, &now) < msec);
}

void
delay (int msec)
{
  struct timeval del;

  del.tv_sec = 0;
  del.tv_usec = msec * 1000;
  select(0, NULL, NULL, NULL, &del);
}

int
timeout_yet (int msec)
{
  static struct timeval start = {0, 0};

  if (msec)                /* initialiseation */
    {
      struct timeval now;
      gettimeofday(&now, NULL);
      return (now.tv_sec * 1000000 + now.tv_usec) -
             (start.tv_sec * 1000000 + start.tv_usec) >= (msec * 1000);
    }

  gettimeofday(&start, NULL);
  return 0;
}

int
validateInteger (int *integer, const char *description, const char *value, const int *minimum, const int *maximum) {
   char *end;
   *integer = strtol(value, &end, 0);
   if (*end) {
      LogPrint(LOG_ERR, "The %s must be an integer: %s",
               description, value);
      return 0;
   }
   if (minimum) {
      if (*integer < *minimum) {
         LogPrint(LOG_ERR, "The %s must not be less than %d: %d",
                  description, *minimum, *integer);
         return 0;
      }
   }
   if (maximum) {
      if (*integer > *maximum) {
         LogPrint(LOG_ERR, "The %s must not be greater than %d: %d",
                  description, *maximum, *integer);
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
validateBaud (speed_t *baud, const char *description, const char *value, const unsigned int *choices) {
   int integer;
   if (validateInteger(&integer, description, value, NULL, NULL)) {
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
            *baud = entry->baud;
            return 1;
         }
         ++entry;
      }
      LogPrint(LOG_ERR, "Invalid %s: %d",
               description, integer);
   }
   return 0;
}

int
baud2integer (speed_t baud)
{
  BaudEntry *entry = baudTable;
  while (entry->integer) {
    if (baud == entry->baud)
      return entry->integer;
    ++entry;
  }
  return -1;
}

int
validateChoice (unsigned int *choice, const char *description, const char *value, const char *const *choices) {
   int length = strlen(value);
   if (length) {
      int index = 0;
      while (choices[index]) {
         if (length <= strlen(choices[index])) {
            if (strncasecmp(value, choices[index], length) == 0) {
               *choice = index;
               return 1;
            }
         }
         ++index;
      }
      LogPrint(LOG_ERR, "Unsupported %s: %s", description, value);
      return 0;
   }
   *choice = 0;
   return 1;
}

int
validateFlag (unsigned int *flag, const char *description, const char *value, const char *on, const char *off) {
   const char *choices[] = {off, on, NULL};
   return validateChoice(flag, description, value, choices);
}

int
validateOnOff (unsigned int *flag, const char *description, const char *value) {
   return validateFlag(flag, description, value, "on", "off");
}

int
validateYesNo (unsigned int *flag, const char *description, const char *value) {
   return validateFlag(flag, description, value, "yes", "no");
}

/* Reverse a 256x256 mapping, used for charset maps. */
void reverseTable(unsigned char *origtab, unsigned char *revtab) {
  int i;
  memset(revtab, 0, 0X100);
  for(i=0XFF; i>=0; i--) revtab[origtab[i]] = i;
}

/* Functions which support horizontal status cells, e.g. Papenmeier. */
/* this stuff is real wired - dont try to understand */

/* Dots for landscape (counterclockwise-rotated) digits. */
const unsigned char landscapeDigits[11] = {
  B1+B5+B2,    B4,          B4+B1,       B4+B5,       B4+B5+B2,
  B4+B2,       B4+B1+B5,    B4+B1+B5+B2, B4+B1+B2,    B1+B5,
  B1+B2+B4+B5
};

/* Format landscape representation of numbers 0 through 99. */
int
landscapeNumber (int x) {
  return landscapeDigits[(x / 10) % 10] | (landscapeDigits[x % 10] << 4);  
}

/* Format landscape flag state indicator. */
int
landscapeFlag (int number, int on) {
  int dots = landscapeDigits[number % 10];
  if (on) dots |= landscapeDigits[10] << 4;
  return dots;
}

/* Dots for seascape (clockwise-rotated) digits. */
const unsigned char seascapeDigits[11] = {
  B5+B1+B4,    B2,          B2+B5,       B2+B1,       B2+B1+B4,
  B2+B4,       B2+B5+B1,    B2+B5+B1+B4, B2+B5+B4,    B5+B1,
  B1+B2+B4+B5
};

/* Format seascape representation of numbers 0 through 99. */
int
seascapeNumber (int x) {
  return (seascapeDigits[(x / 10) % 10] << 4) | seascapeDigits[x % 10];  
}

/* Format seascape flag state indicator. */
int
seascapeFlag (int number, int on) {
  int dots = seascapeDigits[number % 10] << 4;
  if (on) dots |= seascapeDigits[10];
  return dots;
}

/* Dots for portrait digits - 2 numbers in one cells */
const unsigned char portraitDigits[11] = {
  B2+B4+B5, B1, B1+B2, B1+B4, B1+B4+B5, B1+B5, B1+B2+B4, 
  B1+B2+B4+B5, B1+B2+B5, B2+B4, B1+B2+B4+B5
};

/* Format portrait representation of numbers 0 through 99. */
int
portraitNumber (int x) {
  return portraitDigits[(x / 10) % 10] | (portraitDigits[x % 10]<<4);  
}

/* Format portrait flag state indicator. */
int
portraitFlag (int number, int on) {
  int dots = portraitDigits[number % 10] << 4;
  if (on) dots |= portraitDigits[10];
  return dots;
}

/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

#define __EXTENSIONS__	/* for time.h */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>

#include "misc.h"
#include "config.h"
#include "brl.h"
#include "common.h"

struct brltty_env env;		/* environment (i.e. global) parameters */

/*
 * Output braille translation tables.
 * The files *.auto.h (the default tables) are generated at compile-time.
 */
unsigned char texttrans[0X100] =
{
  #include "text.auto.h"
};
unsigned char untexttrans[0X100];

unsigned char attribtrans[0X100] =
{
  #include "attrib.auto.h"
};

/*
 * Status cells support 
 * remark: the Papenmeier has a column with 22 cells, 
 * all other terminals use up to 5 bytes
 */
unsigned char statcells[MAXNSTATCELLS];	/* status cell buffer */

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

// Read data safely by continually retrying the read system call until all
// of the requested data has been transferred or an end-of-file is encountered.
// This routine is a wrapper for the read system call which is fully compatible
// with it except that the caller does not need to handle the various
// scenarios which can occur when a signal interrupts the system call.
size_t safe_read (int fd, unsigned char *buffer, size_t length)
{
  unsigned char *address = buffer;

  // Keep looping while there's still some data to be read.
  while (length > 0)
    {
      // Read the rest of the data.
      size_t count = read(fd, address, length);

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

      // Stop looping if the end of the file has been reached.
      if (count == 0)
        break;

      // In case the system call was interrupted by a signal
      // after some, but not all, of the data was read,
      // point to the remainder of the buffer and try again.
      address += count;
      length -= count;
    }

  // Return the number of bytes which were actually read.
  return address - buffer;
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

  if (msec)		/* initialiseation */
    {
      struct timeval now;
      gettimeofday(&now, NULL);
      return (now.tv_sec * 1000000 + now.tv_usec) -
	     (start.tv_sec * 1000000 + start.tv_usec) >= (msec * 1000);
    }

  gettimeofday(&start, NULL);
  return 0;
}

#ifdef USE_SYSLOG
   #include <syslog.h>
   static int syslogOpened = 0;
#endif
static int logPriority;
static int stderrPriority;
int ProblemCount = 0;

void
LogOpen(void)
{
#ifdef USE_SYSLOG
  if (!syslogOpened) {
    static char name[0X20]; // Must be static as syslog points at it.
    snprintf(name, sizeof(name), "brltty[%d]", getpid());
    openlog(name, LOG_CONS, LOG_DAEMON);
    syslogOpened = 1;
  }
#endif
  logPriority = LOG_INFO;
  stderrPriority = LOG_NOTICE;
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
SetLogPriority(int priority)
{
  logPriority = priority;
}

void
SetStderrPriority(int priority)
{
  stderrPriority = priority;
}

void
SetStderrOff(void)
{
  SetStderrPriority(-1);
}

void
LogPrint (int priority, char *format, ...)
{
  va_list argp;
  va_start(argp, format);

  if (priority <= LOG_ERR)
    ++ProblemCount;

  if (priority <= logPriority) {
#ifdef USE_SYSLOG
    if (syslogOpened) {
      vsyslog(priority, format, argp);
      goto done;
    }
#endif
    priority = stderrPriority;
  }
done:

  if (priority <= stderrPriority) {
    vfprintf(stderr, format, argp);
    fprintf(stderr, "\n");
    fflush(stderr);
  }

  va_end(argp);
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
strdupWrapper (char *string) {
   if (!(string = strdup(string)))
      noMemory();
   return string;
}

int
validateInteger (int *integer, char *description, char *value, int *minimum, int *maximum) {
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
validateBaud (speed_t *baud, char *description, char *value, unsigned int *choices) {
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
validateChoice (unsigned int *choice, char *description, char *value, char **choices) {
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
   }
   LogPrint(LOG_ERR, "Unsupported %s: %s", description, value);
   return 0;
}

int
validateFlag (unsigned int *flag, char *description, char *value, char *on, char *off) {
   char *choices[] = {off, on, NULL};
   return validateChoice(flag, description, value, choices);
}

int
validateOnOff (unsigned int *flag, char *description, char *value) {
   return validateFlag(flag, description, value, "on", "off");
}

int
validateYesNo (unsigned int *flag, char *description, char *value) {
   return validateFlag(flag, description, value, "yes", "no");
}

/* Functions which support horizontal status cells, e.g. Papenmeier. */
/* this stuff is real wired - dont try to understand */

/* logical layout   real bits 
    1 4               1 2
    2 5         -->   3 4
    3 6               5 6
    7 8               7 8
  Bx: x from logical layout, value from real layout  
*/

#define B1 1
#define B2 4
#define B3 16
#define B4 2
#define B5 8
#define B6 32
#define B7 64
#define B8 128

/* Dots for landscape (counterclockwise-rotated) digits. */
const unsigned char landscape_digits[11] =
{
  B1+B5+B2,    B4,          B4+B1,       B4+B5,       B4+B5+B2,
  B4+B2,       B4+B1+B5,    B4+B1+B5+B2, B4+B1+B2,    B1+B5,
  B1+B2+B4+B5
};

/* Format landscape representation of numbers 0 through 99. */
int landscape_number(int x)
{
  return landscape_digits[(x / 10) % 10] | (landscape_digits[x % 10] << 4);  
}

/* Format landscape flag state indicator. */
int landscape_flag(int number, int on)
{
  int dots = landscape_digits[number % 10];
  if (on)
    dots |= landscape_digits[10] << 4;
  return dots;
}

/* Dots for seascape (clockwise-rotated) digits. */
const unsigned char seascape_digits[11] =
{
  B5+B1+B4,    B2,          B2+B5,       B2+B1,       B2+B1+B4,
  B2+B4,       B2+B5+B1,    B2+B5+B1+B4, B2+B5+B4,    B5+B1,
  B1+B2+B4+B5
};

/* Format seascape representation of numbers 0 through 99. */
int seascape_number(int x)
{
  return (seascape_digits[(x / 10) % 10] << 4) | seascape_digits[x % 10];  
}

/* Format seascape flag state indicator. */
int seascape_flag(int number, int on)
{
  int dots = seascape_digits[number % 10] << 4;
  if (on)
    dots |= seascape_digits[10];
  return dots;
}

/* Dots for portrait digits - 2 numbers in one cells */
const unsigned char portrait_digits[11] =
{
  B2+B4+B5, B1, B1+B2, B1+B4, B1+B4+B5, B1+B5, B1+B2+B4, 
  B1+B2+B4+B5, B1+B2+B5, B2+B4, B1+B2+B4+B5
};

/* Format portrait representation of numbers 0 through 99. */
int portrait_number(int x)
{
  return portrait_digits[(x / 10) % 10] | (portrait_digits[x % 10]<<4);  
}

/* Format portrait flag state indicator. */
int portrait_flag(int number, int on)
{
  int dots = portrait_digits[number % 10] << 4;
  if (on)
    dots |= portrait_digits[10];
  return dots;
}

/* Reverse a 256x256 mapping, used for charset maps. */
void reverseTable(unsigned char *origtab, unsigned char *revtab)
{
  int i;
  memset(revtab, 0, 256);
  for(i=255; i>=0; i--)
    revtab[origtab[i]] = i;
}

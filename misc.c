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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>

#include "misc.h"
#include "config.h"
#include "brl.h"
#include "common.h"

struct brltty_env env;		/* environment (i.e. global) parameters */

/*
 * Output braille translation tables.
 * The files *.auto.h (the default tables) are generated at compile-time.
 */
unsigned char texttrans[256] =
{
  #include "text.auto.h"
};
unsigned char untexttrans[256];

unsigned char attribtrans[256] =
{
  #include "attrib.auto.h"
};

/*
 * Status cells support 
 * remark: the Papenmeier has a column with 22 cells, 
 * all other terminals use up to 5 bytes
 */
unsigned char statcells[MAXNSTATCELLS];	/* status cell buffer */

// Process each line of an input text file safely.
// This routine handles the actual reading of the file,
// insuring that the input buffer is always big enough,
// and calls a caller-supplied handler once for each line in the file.
// The caller-supplied data pointer is passed straight through to the handler.
void process_lines (FILE *file,
                    void (*handler) (char *line, void *data),
		    void *data)
{
  size_t buff_size = 0X80; // Initial buffer size.
  char *buff_addr = malloc(buff_size); // Allocate the buffer.
  char *line; // Will point to each line that is read.

  // Keep looping, once per line, until end-of-file.
  while ((line = fgets(buff_addr, buff_size, file)) != NULL)
    {
      size_t line_len = strlen(line); // Line length including new-line.

      // No trailing new-line means that the buffer isn't big enough.
      while (line[line_len-1] != '\n')
        {
          // Extend the buffer, keeping track of its new size.
          buff_addr = realloc(buff_addr, (buff_size <<= 1));

          // Read the rest of the line into the end of the buffer.
          line = fgets(buff_addr+line_len, buff_size-line_len, file);

          line_len += strlen(line); // New total line length.
          line = buff_addr; // Point to the beginning of the line.
        }
      line[line_len -= 1] = '\0'; // Remove trailing new-line.

      handler(line, data);
    }

  // Deallocate the buffer.
  free(buff_addr);
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

#ifdef USE_SYSLOG
#include <syslog.h>
#include <stdarg.h>
#endif

unsigned
elapsed_msec (struct timeval *t1, struct timeval *t2)
{
  unsigned diff, error = 0xFFFFFFFF;
  if (t1->tv_sec > t2->tv_sec)
    return (error);
  diff = (t2->tv_sec - t1->tv_sec) * 1000L;
  if (diff == 0 && t1->tv_usec > t2->tv_usec)
    return (error);
  diff += (t2->tv_usec - t1->tv_usec) / 1000L;
  return (diff);
}


void
shortdelay (unsigned msec)
{
  struct timeval start, now;
  struct timezone tz;
  gettimeofday (&start, &tz);
  do
    {
      gettimeofday (&now, &tz);
    }
  while (elapsed_msec (&start, &now) < msec);
}


void
delay (int msec)
{
  struct timeval del;

  del.tv_sec = 0;
  del.tv_usec = msec * 1000;
  select (0, NULL, NULL, NULL, &del);
}


int
timeout_yet (int msec)
{
  static struct timeval tstart =
  {0, 0};
  struct timeval tnow;

  if (msec == 0)		/* initialiseation */
    {
      gettimeofday (&tstart, NULL);
      return 0;
    }
  gettimeofday (&tnow, NULL);
  return ((tnow.tv_sec * 1e6 + tnow.tv_usec) - \
	  (tstart.tv_sec * 1e6 + tstart.tv_usec) >= msec * 1e3);
}

#ifdef USE_SYSLOG

static int LogOpened = 0;
static int LogPrio;
static int ErrPrio;

void LogOpen(void)
{
  static char name[0X20]; // Must be static as syslog points at it.
  sprintf(name, "brltty[%d]", getpid());
  openlog(name,LOG_CONS,LOG_DAEMON);
  LogPrio = LOG_INFO;
  ErrPrio = LOG_INFO;
  LogOpened = 1;
}

void LogClose(void)
{
  closelog();
  LogOpened = 0;
}

void SetLogPrio(int prio)
{
  LogPrio = prio;
}

void SetErrPrio(int prio)
{
  ErrPrio = prio;
}

void LogPrint(int prio, char *fmt, ...)
{
  va_list argp;

  if(LogOpened && (prio <= LogPrio)){
    va_start(argp, fmt);
    vsyslog(prio, fmt, argp);
    va_end(argp);
  }
}

void LogAndStderr(int prio, char *fmt, ...)
{
  va_list argp;

  va_start(argp, fmt);

  if(prio <= ErrPrio){
    vfprintf(stderr, fmt, argp);
    fprintf(stderr,"\n");
  }
  if(LogOpened && (prio <= LogPrio))
    vsyslog(prio, fmt, argp);

  va_end(argp);
}

#else /* don't USE_SYSLOG */

void LogOpen(void) {}
void LogClose(void) {}
void SetLogPrio(int prio) {}
void SetErrPrio(int prio) {}
void LogPrint(int prio, char *fmt, ...) {}
void LogAndStderr(int prio, char *fmt, ...) {}
#endif


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

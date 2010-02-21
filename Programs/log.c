/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "log.h"

#if defined(HAVE_SYSLOG_H)
static int syslogOpened = 0;

#elif defined(WINDOWS)
static HANDLE windowsEventLog = INVALID_HANDLE_VALUE;

static WORD
toEventType (int level) {
  if (level <= LOG_ERR) return EVENTLOG_ERROR_TYPE;
  if (level <= LOG_WARNING) return EVENTLOG_WARNING_TYPE;
  return EVENTLOG_INFORMATION_TYPE;
}

#elif defined(__MSDOS__)
static int dosLogFile = -1;

#endif /* system log internal definitions */

static int logLevel = LOG_NOTICE;
static const char *printPrefix = NULL;
static int printLevel = LOG_NOTICE;

void
LogOpen (int toConsole) {
#if defined(HAVE_SYSLOG_H)
  if (!syslogOpened) {
    int flags = LOG_PID;
    if (toConsole) flags |= LOG_CONS;
    openlog("brltty", flags, LOG_DAEMON);
    syslogOpened = 1;
  }
#elif defined(WINDOWS)
  if (windowsEventLog == INVALID_HANDLE_VALUE) {
    windowsEventLog = RegisterEventSource(NULL, "brltty");
  }
#elif defined(__MSDOS__)
  if (dosLogFile == -1) {
    dosLogFile = open("brltty.log", O_WRONLY|O_CREAT|O_TRUNC,
                      (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
  }
#endif /* open system log */
}

void
LogClose (void) {
#if defined(HAVE_SYSLOG_H)
  if (syslogOpened) {
    closelog();
    syslogOpened = 0;
  }
#elif defined(WINDOWS)
  if (windowsEventLog != INVALID_HANDLE_VALUE) {
    DeregisterEventSource(windowsEventLog);
    windowsEventLog = INVALID_HANDLE_VALUE;
  }
#elif defined(__MSDOS__)
  if (dosLogFile != -1) {
    close(dosLogFile);
    dosLogFile = -1;
  }
#endif /* close system log */
}

void
LogPrint (int level, const char *format, ...) {
  int reason = errno;
  va_list argp;

  if (level <= logLevel) {
#if defined(HAVE_SYSLOG_H)
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
#define NEED_DONE
    }
#elif defined(WINDOWS)
    if (windowsEventLog != INVALID_HANDLE_VALUE) {
      char buffer[0X100];
      const char *strings[] = {buffer};
      va_start(argp, format);
      vsnprintf(buffer, sizeof(buffer), format, argp);
      va_end(argp);
      ReportEvent(windowsEventLog, toEventType(level), 0, 0, NULL,
                  ARRAY_COUNT(strings), 0, strings, NULL);
      goto done;
#define NEED_DONE
    }
#elif defined(__MSDOS__)
    if (dosLogFile != -1) {
      char buffer[0X100];
      int size;
      va_start(argp, format);
      size = vsnprintf(buffer, sizeof(buffer), format, argp);
      va_end(argp);
      write(dosLogFile, buffer, size);
      write(dosLogFile, "\n", 1);
      goto done;
#define NEED_DONE
    }
#endif /* write system log */

    level = printLevel;
  }

#ifdef NEED_DONE
done:
#endif /* NEED_DONE */

  if (level <= printLevel) {
    FILE *stream = stderr;
    if (printPrefix) fprintf(stream, "%s: ", printPrefix);
    va_start(argp, format);
    vfprintf(stream, format, argp);
    va_end(argp);
    fprintf(stream, "\n");
    fflush(stream);
  }

  errno = reason;
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

#ifdef __MINGW32__
void
LogWindowsSocketError(const char *action) {
  DWORD error = WSAGetLastError();
  LogWindowsCodeError(error, action);
}
#endif /* __MINGW32__ */
#endif /* WINDOWS */

void
LogBytes (int level, const char *description, const unsigned char *data, unsigned int length) {
  if (length) {
    char buffer[(length * 3) + 1];
    char *out = buffer;
    const unsigned char *in = data;

    while (length--) out += sprintf(out, " %2.2X", *in++);
    LogPrint(level, "%s:%s", description, buffer);
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

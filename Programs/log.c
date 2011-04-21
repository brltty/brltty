/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
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
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
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

#endif /* system log internal definitions */

static int printLevel = LOG_NOTICE;
static int logLevel = LOG_NOTICE;
static const char *logPrefix = NULL;

static FILE *logFile = NULL;

void
closeLogFile (void) {
  if (logFile) {
    fclose(logFile);
    logFile = NULL;
  }
}

void
openLogFile (const char *path) {
  closeLogFile();
  logFile = fopen(path, "w");
}

static void
writeLogRecord (const char *record) {
  if (logFile) {
    {
      struct timeval now;
      struct tm description;
      char buffer[0X20];
      int length;

      gettimeofday(&now, NULL);
      localtime_r(&now.tv_sec, &description);
      length = strftime(buffer, sizeof(buffer), "%Y-%m-%d@%H:%M:%S", &description);
      fprintf(logFile, "%.*s.%03ld ", length, buffer, now.tv_usec/1000);
    }

    fputs(record, logFile);
    fputc('\n', logFile);
    fflush(logFile);
  }
}

void
openSystemLog (void) {
#if defined(HAVE_SYSLOG_H)
  if (!syslogOpened) {
    int flags = LOG_PID;
    openlog(PACKAGE_NAME, flags, LOG_DAEMON);
    syslogOpened = 1;
  }
#elif defined(WINDOWS)
  if (windowsEventLog == INVALID_HANDLE_VALUE) {
    windowsEventLog = RegisterEventSource(NULL, PACKAGE_NAME);
  }
#elif defined(__MSDOS__)
  openLogFile(PACKAGE_NAME ".log");
#endif /* open system log */
}

void
closeSystemLog (void) {
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
  closeLogFile();
#endif /* close system log */
}

int
setLogLevel (int newLevel) {
  int oldLevel = logLevel;
  logLevel = newLevel;
  return oldLevel;
}

const char *
setLogPrefix (const char *newPrefix) {
  const char *oldPrefix = logPrefix;
  logPrefix = newPrefix;
  return oldPrefix;
}

int
setPrintLevel (int newLevel) {
  int oldLevel = printLevel;
  printLevel = newLevel;
  return oldLevel;
}

int
setPrintOff (void) {
  return setPrintLevel(-1);
}

void
logData (int level, LogDataFormatter *formatLogData, const void *data) {
  int write = level <= logLevel;
  int print = level <= printLevel;

  if (write || print) {
    int reason = errno;
    char buffer[0X100];
    const char *record = formatLogData(buffer, sizeof(buffer), data);

    if (*record) {
      if (write) {
        writeLogRecord(record);

#if defined(HAVE_SYSLOG_H)
        if (syslogOpened) syslog(level, "%s", record);
#elif defined(WINDOWS)
        if (windowsEventLog != INVALID_HANDLE_VALUE) {
          const char *strings[] = {record};
          ReportEvent(windowsEventLog, toEventType(level), 0, 0, NULL,
                      ARRAY_COUNT(strings), 0, strings, NULL);
        }
#elif defined(__MSDOS__)

#endif /* write system log */
      }

      if (print) {
        FILE *stream = stderr;

        if (logPrefix) {
          fputs(logPrefix, stream);
          fputs(": ", stream);
        }

        fputs(record, stream);
        fputc('\n', stream);
        fflush(stream);
      }
    }

    errno = reason;
  }
}

typedef struct {
  const char *format;
  va_list arguments;
} LogMessageData;

static const char *
formatLogMessageData (char *buffer, size_t size, const void *data) {
  const LogMessageData *msg = data;
  vsnprintf(buffer, size, msg->format, msg->arguments);
  return buffer;
}

void
logMessage (int level, const char *format, ...) {
  LogMessageData msg = {
    .format = format
  };;

  va_start(msg.arguments, format);
  logData(level, formatLogMessageData, &msg);
  va_end(msg.arguments);
}

typedef struct {
  const char *description;
  const void *data;
  size_t length;
} LogBytesData;

static const char *
formatLogBytesData (char *buffer, size_t size, const void *data) {
  const LogBytesData *bytes = data;
  size_t length = bytes->length;
  const unsigned char *in = bytes->data;
  char *out = buffer;

  {
    size_t count = snprintf(out, size, "%s:", bytes->description);
    out += count;
    size -= count;
  }

  while (length-- && (size > 3)) {
    size_t count = snprintf(out, size, " %2.2X", *in++);
    out += count;
    size -= count;
  }

  return buffer;
}

void
logBytes (int level, const char *description, const void *data, size_t length) {
  const LogBytesData bytes = {
    .description = description,
    .data = data,
    .length = length
  };

  logData(level, formatLogBytesData, &bytes);
}

void
logSystemError (const char *action) {
  logMessage(LOG_ERR, "%s error %d: %s.", action, errno, strerror(errno));
}
void
logMallocError (void) {
  logSystemError("malloc");
}

#ifdef WINDOWS
void
logWindowsError (DWORD error, const char *action) {
  char *message;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (char *)&message, 0, NULL);

  {
    char *end = strpbrk(message, "\r\n");
    if (end) *end = 0;
  }

  logMessage(LOG_ERR, "%s error %ld: %s", action, error, message);
  LocalFree(message);
}

void
logWindowsSystemError (const char *action) {
  DWORD error = GetLastError();
  logWindowsError(error, action);
}

#ifdef __MINGW32__
void
logWindowsSocketError (const char *action) {
  DWORD error = WSAGetLastError();
  logWindowsError(error, action);
}
#endif /* __MINGW32__ */
#endif /* WINDOWS */

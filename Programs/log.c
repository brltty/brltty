/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef __MINGW32__
/* MinGW defines localtime_r() in <pthread.h> */
#include <pthread.h>
#endif /* __MINGW32__ */

#ifdef __ANDROID__
#include <android/log.h>
#endif /* __ANDROID__ */

#include "log.h"
#include "timing.h"

const char *const logLevelNames[] = {
  "emergency", "alert", "critical", "error",
  "warning", "notice", "information", "debug"
};
const unsigned int logLevelCount = ARRAY_COUNT(logLevelNames);

unsigned char systemLogLevel = LOG_NOTICE;
unsigned char stderrLogLevel = LOG_NOTICE;

typedef struct {
  const char *name;
  const char *prefix;
} LogCategoryEntry;

static const LogCategoryEntry logCategoryTable[LOG_CATEGORY_COUNT] = {
  [LOG_CATEGORY_INDEX(GENERIC_INPUT)] = {
    .name = "ingio",
    .prefix = "generic input"
  },

  [LOG_CATEGORY_INDEX(INPUT_PACKETS)] = {
    .name = "inpkts",
    .prefix = "input packet"
  },

  [LOG_CATEGORY_INDEX(OUTPUT_PACKETS)] = {
    .name = "outpkts",
    .prefix = "output packet"
  },

  [LOG_CATEGORY_INDEX(BRAILLE_KEY_EVENTS)] = {
    .name = "brlkeys",
    .prefix = "braille key"
  },

  [LOG_CATEGORY_INDEX(KEYBOARD_KEY_EVENTS)] = {
    .name = "kbdkeys",
    .prefix = "keyboard key"
  },

  [LOG_CATEGORY_INDEX(CURSOR_TRACKING)] = {
    .name = "csrtrk",
    .prefix = "cursor tracking"
  },

  [LOG_CATEGORY_INDEX(CURSOR_ROUTING)] = {
    .name = "csrrtg",
    .prefix = "cursor routing"
  },
};

unsigned char categoryLogLevel = LOG_WARNING;
unsigned char logCategoryFlags[LOG_CATEGORY_COUNT];

#if defined(WINDOWS)
static HANDLE windowsEventLog = INVALID_HANDLE_VALUE;

static WORD
toWindowsEventType (int level) {
  if (level <= LOG_ERR) return EVENTLOG_ERROR_TYPE;
  if (level <= LOG_WARNING) return EVENTLOG_WARNING_TYPE;
  return EVENTLOG_INFORMATION_TYPE;
}

#elif defined(__MSDOS__)

#elif defined(__ANDROID__)
static int
toAndroidLogPriority (int level) {
  switch (level) {
    case LOG_EMERG:   return ANDROID_LOG_FATAL;
    case LOG_ALERT:   return ANDROID_LOG_FATAL;
    case LOG_CRIT:    return ANDROID_LOG_FATAL;
    case LOG_ERR:     return ANDROID_LOG_ERROR;
    case LOG_WARNING: return ANDROID_LOG_WARN;
    case LOG_NOTICE:  return ANDROID_LOG_INFO;
    case LOG_INFO:    return ANDROID_LOG_INFO;
    case LOG_DEBUG:   return ANDROID_LOG_DEBUG;
    default:          return ANDROID_LOG_UNKNOWN;
  }
}

#elif defined(HAVE_SYSLOG_H)
static int syslogOpened = 0;
#endif /* system log internal definitions */

static const char *logPrefix = NULL;
static FILE *logFile = NULL;

int
enableLogCategory (const char *name) {
  const LogCategoryEntry *ctg = logCategoryTable;
  const LogCategoryEntry *end = ctg + LOG_CATEGORY_COUNT;

  while (ctg < end) {
    if (ctg->name) {
      if (strcasecmp(name, ctg->name) == 0) {
        logCategoryFlags[ctg - logCategoryTable] = 1;
        return 1;
      }
    }

    ctg += 1;
  }

  return 0;
}

const char *
setLogPrefix (const char *newPrefix) {
  const char *oldPrefix = logPrefix;
  logPrefix = newPrefix;
  return oldPrefix;
}

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
      TimeValue now;
      char buffer[0X20];
      size_t length;
      unsigned int milliseconds;

      getCurrentTime(&now);
      length = formatSeconds(buffer, sizeof(buffer), "%Y-%m-%d@%H:%M:%S", now.seconds);
      milliseconds = now.nanoseconds / NSECS_PER_MSEC;

      fprintf(logFile, "%.*s.%03u ", (int)length, buffer, milliseconds);
    }

    fputs(record, logFile);
    fputc('\n', logFile);
    fflush(logFile);
  }
}

void
openSystemLog (void) {
#if defined(WINDOWS)
  if (windowsEventLog == INVALID_HANDLE_VALUE) {
    windowsEventLog = RegisterEventSource(NULL, PACKAGE_NAME);
  }

#elif defined(__MSDOS__)
  openLogFile(PACKAGE_NAME ".log");

#elif defined(__ANDROID__)

#elif defined(HAVE_SYSLOG_H)
  if (!syslogOpened) {
    openlog(PACKAGE_NAME, LOG_PID, LOG_DAEMON);
    syslogOpened = 1;
  }
#endif /* open system log */
}

void
closeSystemLog (void) {
#if defined(WINDOWS)
  if (windowsEventLog != INVALID_HANDLE_VALUE) {
    DeregisterEventSource(windowsEventLog);
    windowsEventLog = INVALID_HANDLE_VALUE;
  }

#elif defined(__MSDOS__)
  closeLogFile();

#elif defined(__ANDROID__)

#elif defined(HAVE_SYSLOG_H)
  if (syslogOpened) {
    closelog();
    syslogOpened = 0;
  }
#endif /* close system log */
}

void
logData (int level, LogDataFormatter *formatLogData, const void *data) {
  const char *prefix = NULL;

  if (level & LOG_FLG_CATEGORY) {
    int category = level & LOG_MSK_CATEGORY;

    if (!logCategoryFlags[category]) return;
    level = categoryLogLevel;

    {
      const LogCategoryEntry *ctg = &logCategoryTable[category];

      prefix = ctg->prefix;
    }
  }

  {
    int write = level <= systemLogLevel;
    int print = level <= stderrLogLevel;

    if (write || print) {
      int oldErrno = errno;
      char record[0X1000];

      STR_BEGIN(record, sizeof(record));
      if (prefix) STR_PRINTF("%s: ", prefix);
      {
        size_t sublength = formatLogData(STR_NEXT, STR_LEFT, data);
        STR_ADJUST(sublength);
      }
      STR_END

      if (write) {
        writeLogRecord(record);

#if defined(WINDOWS)
        if (windowsEventLog != INVALID_HANDLE_VALUE) {
          const char *strings[] = {record};
          ReportEvent(windowsEventLog, toWindowsEventType(level), 0, 0, NULL,
                      ARRAY_COUNT(strings), 0, strings, NULL);
        }

#elif defined(__MSDOS__)

#elif defined(__ANDROID__)
        __android_log_write(toAndroidLogPriority(level), PACKAGE_NAME, record);

#elif defined(HAVE_SYSLOG_H)
        if (syslogOpened) syslog(level, "%s", record);
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

      errno = oldErrno;
    }
  }
}

typedef struct {
  const char *format;
  va_list *arguments;
} LogMessageData;

static size_t
formatLogMessageData (char *buffer, size_t size, const void *data) {
  const LogMessageData *msg = data;
  int length = vsnprintf(buffer, size, msg->format, *msg->arguments);

  if (length < 0) return 0;
  if (length < size) return length;
  return size;
}

void
vlogMessage (int level, const char *format, va_list *arguments) {
  const LogMessageData msg = {
    .format = format,
    .arguments = arguments
  };

  logData(level, formatLogMessageData, &msg);
}

void
logMessage (int level, const char *format, ...) {
  va_list arguments;

  va_start(arguments, format);
  vlogMessage(level, format, &arguments);
}

typedef struct {
  const char *description;
  const void *data;
  size_t length;
} LogBytesData;

static size_t
formatLogBytesData (char *buffer, size_t size, const void *data) {
  const LogBytesData *bytes = data;
  const unsigned char *byte = bytes->data;
  const unsigned char *end = byte + bytes->length;
  size_t length;

  STR_BEGIN(buffer, size);
  STR_PRINTF("%s:", bytes->description);
  while (byte < end) STR_PRINTF(" %2.2X", *byte++);
  length = STR_LENGTH;
  STR_END
  return length;
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

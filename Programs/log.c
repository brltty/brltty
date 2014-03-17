/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#ifdef _POSIX_THREAD_SAFE_FUNCTIONS
#define lockStream(stream) flockfile((stream))
#define unlockStream(stream) funlockfile((stream))
#else /* _POSIX_THREAD_SAFE_FUNCTIONS */
#define lockStream(stream)
#define unlockStream(stream)
#endif /* _POSIX_THREAD_SAFE_FUNCTIONS */

#ifdef __MINGW32__
/* MinGW defines localtime_r() in <pthread.h> */
#include <pthread.h>
#endif /* __MINGW32__ */

#ifdef __ANDROID__
#include <android/log.h>
#endif /* __ANDROID__ */

#include "log.h"
#include "timing.h"
#include "addresses.h"

const char *const logLevelNames[] = {
  "emergency", "alert", "critical", "error",
  "warning", "notice", "information", "debug"
};
const unsigned int logLevelCount = ARRAY_COUNT(logLevelNames);

unsigned char systemLogLevel = LOG_NOTICE;
unsigned char stderrLogLevel = LOG_NOTICE;

typedef struct {
  const char *name;
  const char *title;
  const char *prefix;
} LogCategoryEntry;

static const LogCategoryEntry logCategoryTable[LOG_CATEGORY_COUNT] = {
  [LOG_CATEGORY_INDEX(GENERIC_INPUT)] = {
    .name = "ingio",
    .title = strtext("Generic Input"),
    .prefix = "generic input"
  },

  [LOG_CATEGORY_INDEX(INPUT_PACKETS)] = {
    .name = "inpkts",
    .title = strtext("Input Packets"),
    .prefix = "input packet"
  },

  [LOG_CATEGORY_INDEX(OUTPUT_PACKETS)] = {
    .name = "outpkts",
    .title = strtext("Output Packets"),
    .prefix = "output packet"
  },

  [LOG_CATEGORY_INDEX(BRAILLE_KEYS)] = {
    .name = "brlkeys",
    .title = strtext("Braille Key Events"),
    .prefix = "braille key"
  },

  [LOG_CATEGORY_INDEX(KEYBOARD_KEYS)] = {
    .name = "kbdkeys",
    .title = strtext("Keyboard Key Events"),
    .prefix = "keyboard key"
  },

  [LOG_CATEGORY_INDEX(CURSOR_TRACKING)] = {
    .name = "csrtrk",
    .title = strtext("Cursor Tracking"),
    .prefix = "cursor tracking"
  },

  [LOG_CATEGORY_INDEX(CURSOR_ROUTING)] = {
    .name = "csrrtg",
    .title = strtext("Cursor Routing"),
    .prefix = "cursor routing"
  },

  [LOG_CATEGORY_INDEX(UPDATE_EVENTS)] = {
    .name = "update",
    .title = strtext("Update Events"),
    .prefix = "update"
  },

  [LOG_CATEGORY_INDEX(SPEECH_EVENTS)] = {
    .name = "speech",
    .title = strtext("Speech Events"),
    .prefix = "speech"
  },

  [LOG_CATEGORY_INDEX(ASYNC_EVENTS)] = {
    .name = "async",
    .title = strtext("Async Events"),
    .prefix = "async"
  },

  [LOG_CATEGORY_INDEX(SERVER_EVENTS)] = {
    .name = "server",
    .title = strtext("Server Events"),
    .prefix = "server"
  },

  [LOG_CATEGORY_INDEX(SERIAL_IO)] = {
    .name = "serial",
    .title = strtext("Serial I/O"),
    .prefix = "serial"
  },

  [LOG_CATEGORY_INDEX(USB_IO)] = {
    .name = "usb",
    .title = strtext("USB I/O"),
    .prefix = "USB"
  },

  [LOG_CATEGORY_INDEX(BLUETOOTH_IO)] = {
    .name = "bluetooth",
    .title = strtext("Bluetooth I/O"),
    .prefix = "Bluetooth"
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

typedef struct LogPrefixEntryStruct LogPrefixEntry;
static LogPrefixEntry *logPrefixStack = NULL;
static FILE *logFile = NULL;

struct LogPrefixEntryStruct {
  LogPrefixEntry *previous;
  char *prefix;
};

static inline const LogCategoryEntry *
getLogCategoryEntry (LogCategoryIndex index) {
  return ((index >= 0) && (index < LOG_CATEGORY_COUNT))? &logCategoryTable[index]: NULL;
}

const char *
getLogCategoryName (LogCategoryIndex index) {
  const LogCategoryEntry *ctg = getLogCategoryEntry(index);

  return (ctg && ctg->name)? ctg->name: "";
}

const char *
getLogCategoryTitle (LogCategoryIndex index) {
  const LogCategoryEntry *ctg = getLogCategoryEntry(index);

  return (ctg && ctg->title)? ctg->title: "";
}

static void
setLogCategory (const LogCategoryEntry *ctg, int state) {
  logCategoryFlags[ctg - logCategoryTable] = state;
}

void
disableAllLogCategories (void) {
  const LogCategoryEntry *ctg = logCategoryTable;
  const LogCategoryEntry *end = ctg + LOG_CATEGORY_COUNT;

  while (ctg < end) setLogCategory(ctg++, 0);
}

int
enableLogCategory (const char *name) {
  const LogCategoryEntry *ctg = logCategoryTable;
  const LogCategoryEntry *end = ctg + LOG_CATEGORY_COUNT;

  while (ctg < end) {
    if (ctg->name) {
      if (strcasecmp(name, ctg->name) == 0) {
        setLogCategory(ctg, 1);
        return 1;
      }
    }

    ctg += 1;
  }

  return 0;
}

int
pushLogPrefix (const char *prefix) {
  LogPrefixEntry *entry;

  if ((entry = malloc(sizeof(*entry)))) {
    memset(entry, 0, sizeof(*entry));
    if (!prefix) prefix = "";

    if ((entry->prefix = strdup(prefix))) {
      entry->previous = logPrefixStack;
      logPrefixStack = entry;
      return 1;
    }

    free(entry);
  }

  return 0;
}

int
popLogPrefix (void) {
  if (logPrefixStack) {
    LogPrefixEntry *entry = logPrefixStack;
    logPrefixStack = entry->previous;
    entry->previous = NULL;

    free(entry->prefix);
    free(entry);

    return 1;
  }

  return 0;
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
    lockStream(logFile);

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
    unlockStream(logFile);
  }
}

void
openSystemLog (void) {
#if defined(WINDOWS)
  if (windowsEventLog == INVALID_HANDLE_VALUE) {
    windowsEventLog = RegisterEventSource(NULL, PACKAGE_TARNAME);
  }

#elif defined(__MSDOS__)
  openLogFile(PACKAGE_TARNAME ".log");

#elif defined(__ANDROID__)

#elif defined(HAVE_SYSLOG_H)
  if (!syslogOpened) {
    openlog(PACKAGE_TARNAME, LOG_PID, LOG_DAEMON);
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
        __android_log_write(toAndroidLogPriority(level), PACKAGE_TARNAME, record);

#elif defined(HAVE_SYSLOG_H)
        if (syslogOpened) syslog(level, "%s", record);
#endif /* write system log */
      }

      if (print) {
        FILE *stream = stderr;
        lockStream(stream);

        if (logPrefixStack) {
          const char *prefix = logPrefixStack->prefix;

          if (*prefix) {
            fputs(prefix, stream);
            fputs(": ", stream);
          }
        }

        fputs(record, stream);
        fputc('\n', stream);
        fflush(stream);
        unlockStream(stream);
      }

      errno = oldErrno;
    }
  }
}

static size_t
formatLogArguments (char *buffer, size_t size, const char *format, va_list *arguments) {
  int length = vsnprintf(buffer, size, format, *arguments);

  if (length < 0) return 0;
  if (length < size) return length;
  return size;
}

typedef struct {
  const char *format;
  va_list *arguments;
} LogMessageData;

static size_t
formatLogMessageData (char *buffer, size_t size, const void *data) {
  const LogMessageData *msg = data;

  return formatLogArguments(buffer, size, msg->format, msg->arguments);
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
  va_end(arguments);
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
  if (bytes->description) STR_PRINTF("%s: ", bytes->description);

  while (byte < end) {
    if (byte != bytes->data) STR_PRINTF(" ");
    STR_PRINTF("%2.2X", *byte++);
  }

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

typedef struct {
  void *address;
  const char *format;
  va_list *arguments;
} LogSymbolData;

static size_t
formatLogSymbolData (char *buffer, size_t size, const void *data) {
  const LogSymbolData *symbol = data;
  size_t length;

  ptrdiff_t offset = 0;
  const char *name = getAddressName(symbol->address, &offset);

  STR_BEGIN(buffer, size);

  length = formatLogArguments(STR_NEXT, STR_LEFT, symbol->format, symbol->arguments);
  STR_ADJUST(length);
  STR_PRINTF(": ");

  if (name && *name) {
    STR_PRINTF("%s", name);
    if (offset) STR_PRINTF("+%#tX", offset);
  } else {
    STR_PRINTF("%p", symbol->address);
  }

  length = STR_LENGTH;
  STR_END;
  return length;
}

void
logSymbol (int level, void *address, const char *format, ...) {
  va_list arguments;
  va_start(arguments, format);

  {
    const LogSymbolData symbol = {
      .address = address,
      .format = format,
      .arguments = &arguments
    };

    logData(level, formatLogSymbolData, &symbol);
  }

  va_end(arguments);
}

void
logActionError (int error, const char *action) {
  logMessage(LOG_ERR, "%s error %d: %s.", action, error, strerror(error));
}

void
logSystemError (const char *action) {
  logActionError(errno, action);
}

void
logMallocError (void) {
  logSystemError("malloc");
}

void
logUnsupportedFeature (const char *name) {
  logMessage(LOG_WARNING, "feature not supported: %s", name);
}

void
logUnsupportedOperation (const char *name) {
  errno = ENOSYS;
  logSystemError(name);
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

  logMessage(LOG_ERR, "%s error %d: %s", action, (int)error, message);
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

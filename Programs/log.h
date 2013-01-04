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

#ifndef BRLTTY_INCLUDED_LOG
#define BRLTTY_INCLUDED_LOG

#include "prologue.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern unsigned char systemLogLevel;
extern unsigned char stderrLogLevel;

#if defined(HAVE_SYSLOG_H)
#include <syslog.h>
#else /* no system log */
typedef enum {
  LOG_EMERG,
  LOG_ALERT,
  LOG_CRIT,
  LOG_ERR,
  LOG_WARNING,
  LOG_NOTICE,
  LOG_INFO,
  LOG_DEBUG
} SyslogLevel;
#endif /* system log external definitions */

extern const char *const logLevelNames[];
extern const unsigned int logLevelCount;

#define LOG_FLG_CATEGORY 0X80
#define LOG_MSK_CATEGORY (LOG_FLG_CATEGORY - 1)

#define LOG_CATEGORY_INDEX(name) LOG_CTG_##name
#define LOG_CATEGORY(name) (LOG_FLG_CATEGORY | LOG_CATEGORY_INDEX(name))

typedef enum {
  LOG_CATEGORY_INDEX(GENERIC_INPUT),

  LOG_CATEGORY_INDEX(INPUT_PACKETS),
  LOG_CATEGORY_INDEX(OUTPUT_PACKETS),

  LOG_CATEGORY_INDEX(BRAILLE_KEY_EVENTS),
  LOG_CATEGORY_INDEX(KEYBOARD_KEY_EVENTS),

  LOG_CATEGORY_INDEX(CURSOR_TRACKING),
  LOG_CATEGORY_INDEX(CURSOR_ROUTING),

  LOG_CATEGORY_COUNT /* must be last */
} LogCategoryIndex;

extern int enableLogCategory (const char *name);
extern unsigned char categoryLogLevel;

extern unsigned char logCategoryFlags[LOG_CATEGORY_COUNT];
#define LOG_CATEGORY_FLAG(name) logCategoryFlags[LOG_CATEGORY_INDEX(name)]

extern void openLogFile (const char *path);
extern void closeLogFile (void);

extern void openSystemLog (void);
extern void closeSystemLog (void);

extern const char *setLogPrefix (const char *newPrefix);

typedef size_t LogDataFormatter (char *buffer, size_t size, const void *data);
extern void logData (int level, LogDataFormatter *formatLogData, const void *data);

extern void logMessage (int level, const char *format, ...) PRINTF(2, 3);
extern void vlogMessage (int level, const char *format, va_list *arguments);

extern void logBytes (int level, const char *description, const void *data, size_t length);

extern void logSystemError (const char *action);
extern void logMallocError (void);

#ifdef WINDOWS
extern void logWindowsError (DWORD code, const char *action);
extern void logWindowsSystemError (const char *action);

#ifdef __MINGW32__
extern void logWindowsSocketError (const char *action);
#endif /* __MINGW32__ */
#endif /* WINDOWS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_LOG */

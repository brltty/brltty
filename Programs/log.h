/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

extern void openLogFile (const char *path);
extern void closeLogFile (void);

extern void openSystemLog (void);
extern void closeSystemLog (void);

extern const char *setLogPrefix (const char *newPrefix);

typedef const char *LogDataFormatter (char *buffer, size_t size, const void *data);
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

extern unsigned char systemLogLevel;
extern unsigned char stderrLogLevel;

extern int enableLogCategory (const char *name);
extern unsigned char categoryLogLevel;

typedef enum {
  LOG_CTG_GENERIC_INPUT,
  LOG_CTG_INPUT_PACKETS,
  LOG_CTG_OUTPUT_PACKETS,
  LOG_CTG_BRAILLE_KEY_EVENTS,
  LOG_CTG_KEYBOARD_KEY_EVENTS,
  LOG_CTG_CURSOR_TRACKING,
  LOG_CTG_CURSOR_ROUTING
} LogCategory;

extern unsigned char logCategoryFlags[];
#define LOG_CATEGORY_FLAG(category) logCategoryFlags[LOG_CTG_##category]

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_LOG */

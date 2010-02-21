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

#ifndef BRLTTY_INCLUDED_LOG
#define BRLTTY_INCLUDED_LOG

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "prologue.h"

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

extern void LogOpen(int toConsole);
extern void LogClose(void);
extern void LogPrint (int level, const char *format, ...) PRINTF(2, 3);
extern void LogError (const char *action);

#ifdef WINDOWS
extern void LogWindowsCodeError (DWORD code, const char *action);
extern void LogWindowsError (const char *action);

#ifdef __MINGW32__
extern void LogWindowsSocketError (const char *action);
#endif /* __MINGW32__ */
#endif /* WINDOWS */

extern void LogBytes (int level, const char *description, const unsigned char *data, unsigned int length);
extern int setLogLevel (int level);
extern const char *setPrintPrefix (const char *prefix);
extern int setPrintLevel (int level);
extern int setPrintOff (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_LOG */

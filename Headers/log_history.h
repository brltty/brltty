/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2017 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_LOG_HISTORY
#define BRLTTY_INCLUDED_LOG_HISTORY

#include "timing.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  LEO_NOLOG  = 0X01,
  LEO_SQUASH = 0X02,
} LogEntryOptions;

typedef struct LogEntryStruct LogEntry;
extern const LogEntry *getPreviousLogEntry (const LogEntry *entry);
extern const char *getLogEntryText (const LogEntry *entry);
extern const TimeValue *getLogEntryTime (const LogEntry *entry);
extern unsigned int getLogEntryCount (const LogEntry *entry);

extern int pushLogEntry (LogEntry **head, const char *text, LogEntryOptions options);
extern int popLogEntry (LogEntry **head);

extern const LogEntry *getNewestLogMessage (void);
extern void pushLogMessage (const char *message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_LOG_HISTORY */

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

#include "log.h"
#include "cmd_braille.h"
#include "cmd_queue.h"
#include "async_alarm.h"
#include "prefs.h"
#include "ktb.h"
#include "brl.h"
#include "brldefs.h"
#include "brltty.h"

int
enqueueKeyEvent (unsigned char set, unsigned char key, int press) {
#ifdef ENABLE_API
  if (apiStarted) {
    if (api_handleKeyEvent(set, key, press) == EOF) {
      return 1;
    }
  }
#endif /* ENABLE_API */

  if (brl.keyTable) {
    switch (prefs.brailleOrientation) {
      case BRL_ORIENTATION_ROTATED:
        if (brl.rotateKey) brl.rotateKey(&brl, &set, &key);
        break;

      default:
      case BRL_ORIENTATION_NORMAL:
        break;
    }

    processKeyEvent(brl.keyTable, getCurrentCommandContext(), set, key, press);
  }

  return 1;
}

int
enqueueKey (unsigned char set, unsigned char key) {
  if (enqueueKeyEvent(set, key, 1))
    if (enqueueKeyEvent(set, key, 0))
      return 1;

  return 0;
}

int
enqueueKeys (uint32_t bits, unsigned char set, unsigned char key) {
  unsigned char stack[0X20];
  unsigned char count = 0;

  while (bits) {
    if (bits & 0X1) {
      if (!enqueueKeyEvent(set, key, 1)) return 0;
      stack[count++] = key;
    }

    bits >>= 1;
    key += 1;
  }

  while (count)
    if (!enqueueKeyEvent(set, stack[--count], 0))
      return 0;

  return 1;
}

int
enqueueUpdatedKeys (uint32_t new, uint32_t *old, unsigned char set, unsigned char key) {
  uint32_t bit = 0X1;
  unsigned char stack[0X20];
  unsigned char count = 0;

  while (*old != new) {
    if ((new & bit) && !(*old & bit)) {
      stack[count++] = key;
      *old |= bit;
    } else if (!(new & bit) && (*old & bit)) {
      if (!enqueueKeyEvent(set, key, 0)) return 0;
      *old &= ~bit;
    }

    key += 1;
    bit <<= 1;
  }

  while (count)
    if (!enqueueKeyEvent(set, stack[--count], 1))
      return 0;

  return 1;
}

int
enqueueXtScanCode (
  unsigned char key, unsigned char escape,
  unsigned char set00, unsigned char setE0, unsigned char setE1
) {
  unsigned char set;

  switch (escape) {
    case 0X00: set = set00; break;
    case 0XE0: set = setE0; break;
    case 0XE1: set = setE1; break;

    default:
      logMessage(LOG_WARNING, "unsupported XT scan code: %02X %02X", escape, key);
      return 0;
  }

  return enqueueKey(set, key);
}

static int
readCommand (void) {
  int command = readBrailleCommand(&brl, getCurrentCommandContext());

  if (command != EOF) {
    switch (command & BRL_MSK_CMD) {
      case BRL_CMD_OFFLINE:
        if (!isOffline) {
          logMessage(LOG_DEBUG, "braille display offline");
          isOffline = 1;
        }
        return 0;

      default:
        break;
    }
  }

  if (isOffline) {
    logMessage(LOG_DEBUG, "braille display online");
    isOffline = 0;
  }

  if (command == EOF) return 0;
  enqueueCommand(command);
  return 1;
}

static void setPollAlarm (int delay, void *data);
static AsyncHandle pollAlarm = NULL;

static void
handlePollAlarm (const AsyncAlarmResult *result) {
  int delay = 40;

  asyncDiscardHandle(pollAlarm);
  pollAlarm = NULL;

  if (!isSuspended) {
#ifdef ENABLE_API
    apiClaimDriver();
#endif /* ENABLE_API */

    if (readCommand()) delay = 0;

#ifdef ENABLE_API
    apiReleaseDriver();
#endif /* ENABLE_API */
  }

#ifdef ENABLE_API
  else if (apiStarted) {
    switch (readBrailleCommand(&brl, KTB_CTX_DEFAULT)) {
      case BRL_CMD_RESTARTBRL:
        restartBrailleDriver();
        break;

      default:
        delay = 0;
      case EOF:
        break;
    }
  }

  if (apiStarted) {
    if (!api_flush(&brl)) restartRequired = 1;
  }
#endif /* ENABLE_API */

  setPollAlarm(delay, result->data);
}

static void
setPollAlarm (int delay, void *data) {
  if (!pollAlarm) {
   asyncSetAlarmIn(&pollAlarm, delay, handlePollAlarm, data);
  }
}

void
startBrailleCommands (void) {
  setPollAlarm(0, NULL);
}

void
stopBrailleCommands (void) {
  if (pollAlarm) {
    asyncCancelRequest(pollAlarm);
    pollAlarm = NULL;
  }
}

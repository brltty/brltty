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
#include <string.h>

#include "log.h"
#include "cmd_queue.h"
#include "queue.h"
#include "async_alarm.h"
#include "ktbdefs.h"
#include "scr.h"
#include "brl.h"
#include "ktb.h"
#include "prefs.h"
#include "brltty.h"

typedef struct CommandHandlerLevelStruct CommandHandlerLevel;
static CommandHandlerLevel *commandHandlerStack = NULL;

struct CommandHandlerLevelStruct {
  CommandHandlerLevel *previousLevel;
  CommandHandler *handleCommand;
  void *data;
};

int
pushCommandHandler (CommandHandler *handler, void *data) {
  CommandHandlerLevel *chl;

  if ((chl = malloc(sizeof(*chl)))) {
    memset(chl, 0, sizeof(*chl));
    chl->handleCommand = handler;
    chl->data = data;

    chl->previousLevel = commandHandlerStack;
    commandHandlerStack = chl;
    return 1;
  } else {
    logMallocError();
  }

  return 0;
}

typedef struct {
  int command;
} CommandQueueItem;

static void
deallocateCommandQueueItem (void *item, void *data) {
  CommandQueueItem *cmd = item;

  free(cmd);
}

static Queue *
createCommandQueue (void *data) {
  return newQueue(deallocateCommandQueueItem, NULL);
}

static Queue *
getCommandQueue (int create) {
  static Queue *commands = NULL;

  return getProgramQueue(&commands, "command-queue", create,
                         createCommandQueue, NULL);
}

static int
dequeueCommand (Queue *queue) {
  CommandQueueItem *item;

  while ((item = dequeueItem(queue))) {
    int command = item->command;
    free(item);

#ifdef ENABLE_API
    if (apiStarted) {
      if ((command = api_handleCommand(command)) == EOF) {
        continue;
      }
    }
#endif /* ENABLE_API */

    return command;
  }

  return EOF;
}

static void setExecuteCommandAlarm (void *data);

static void
handleExecuteCommandAlarm (const AsyncAlarmResult *result) {
  Queue *queue = getCommandQueue(0);

  if (queue) {
    int command = dequeueCommand(queue);

    if (command != EOF) {
      const CommandHandlerLevel *chl = commandHandlerStack;

      while (chl) {
        if (chl->handleCommand(command, chl->data)) break;
        chl = chl->previousLevel;
      }
    }

    if (getQueueSize(queue) > 0) setExecuteCommandAlarm(result->data);
  }
}

static void
setExecuteCommandAlarm (void *data) {
  asyncSetAlarmIn(NULL, 0, handleExecuteCommandAlarm, data);
}

int
enqueueCommand (int command) {
  if (command != EOF) {
    Queue *queue = getCommandQueue(1);

    if (queue) {
      CommandQueueItem *item = malloc(sizeof(CommandQueueItem));

      if (item) {
        item->command = command;

        if (enqueueItem(queue, item)) {
          if (getQueueSize(queue) == 1) setExecuteCommandAlarm(NULL);
          return 1;
        }

        free(item);
      }
    }
  }

  return 0;
}

typedef struct {
  unsigned char set;
  unsigned char key;
  unsigned press:1;
} KeyEvent;

static const int keyReleaseTimeout = 0;
static TimePeriod keyReleasePeriod;
static KeyEvent *keyReleaseEvent = NULL;

static void
deallocateKeyEvent (KeyEvent *event) {
  free(event);
}

static void
deallocateKeyEventQueueItem (void *item, void *data) {
  KeyEvent *event = item;

  deallocateKeyEvent(event);
}

static Queue *
createKeyEventQueue (void *data) {
  return newQueue(deallocateKeyEventQueueItem, NULL);
}

static Queue *
getKeyEventQueue (int create) {
  static Queue *events = NULL;

  return getProgramQueue(&events, "key-event-queue", create,
                         createKeyEventQueue, NULL);
}

static int
addKeyEvent (KeyEvent *event) {
  Queue *queue = getKeyEventQueue(1);

  if (queue) {
    if (enqueueItem(queue, event)) {
      return 1;
    }
  }

  return 0;
}

static int
dequeueKeyEvent (unsigned char *set, unsigned char *key, int *press) {
  Queue *queue = getKeyEventQueue(0);

  if (keyReleaseEvent) {
    if (afterTimePeriod(&keyReleasePeriod, NULL)) {
      if (!addKeyEvent(keyReleaseEvent)) return 0;
      keyReleaseEvent = NULL;
    }
  }

  if (queue) {
    KeyEvent *event;

    while ((event = dequeueItem(queue))) {
#ifdef ENABLE_API
      if (apiStarted) {
        if ((api_handleKeyEvent(event->set, event->key, event->press)) == EOF) {
          deallocateKeyEvent(event);
	  continue;
	}
      }
#endif /* ENABLE_API */

      *set = event->set;
      *key = event->key;
      *press = event->press;
      deallocateKeyEvent(event);
      return 1;
    }
  }

  return 0;
}

int
enqueueKeyEvent (unsigned char set, unsigned char key, int press) {
  if (keyReleaseEvent) {
    if (press && (set == keyReleaseEvent->set) && (key == keyReleaseEvent->key)) {
      if (!afterTimePeriod(&keyReleasePeriod, NULL)) {
        deallocateKeyEvent(keyReleaseEvent);
        keyReleaseEvent = NULL;
        return 1;
      }
    }

    {
      KeyEvent *event = keyReleaseEvent;
      keyReleaseEvent = NULL;

      if (!addKeyEvent(event)) {
        deallocateKeyEvent(event);
        return 0;
      }
    }
  }

  {
    KeyEvent *event;

    if ((event = malloc(sizeof(*event)))) {
      event->set = set;
      event->key = key;
      event->press = press;

      if (keyReleaseTimeout && !press) {
        keyReleaseEvent = event;
        startTimePeriod(&keyReleasePeriod, keyReleaseTimeout);
        return 1;
      }

      if (addKeyEvent(event)) return 1;
      deallocateKeyEvent(event);
    } else {
      logMallocError();
    }
  }

  return 0;
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
readNextCommand (void) {
  KeyTableCommandContext context = getScreenCommandContext();
  int command = readBrailleCommand(&brl, context);

  unsigned char set;
  unsigned char key;
  int press;

  while (dequeueKeyEvent(&set, &key, &press)) {
    if (brl.keyTable) {
      switch (prefs.brailleOrientation) {
        case BRL_ORIENTATION_ROTATED:
          if (brl.rotateKey) brl.rotateKey(&brl, &set, &key);
          break;

        default:
        case BRL_ORIENTATION_NORMAL:
          break;
      }

      processKeyEvent(brl.keyTable, context, set, key, press);
    }
  }

  if (command == EOF) return 0;
  enqueueCommand(command);
  return 1;
}

static void setReadCommandAlarm (int delay, void *data);
static AsyncHandle readCommandAlarm = NULL;

static void
handleReadCommandAlarm (const AsyncAlarmResult *result) {
  int delay = 40;

  asyncDiscardHandle(readCommandAlarm);
  readCommandAlarm = NULL;

  if (!isSuspended) {
    apiClaimDriver();

    if (readNextCommand()) {
      delay = 0;
      resetUpdateAlarm(10);
    }

    apiReleaseDriver();
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

  setReadCommandAlarm(delay, result->data);
}

static void
setReadCommandAlarm (int delay, void *data) {
  if (!readCommandAlarm) {
   asyncSetAlarmIn(&readCommandAlarm, delay, handleReadCommandAlarm, data);
  }
}

void
startBrailleCommands (void) {
  setReadCommandAlarm(0, NULL);
}

void
stopBrailleCommands (void) {
  if (readCommandAlarm) {
    asyncCancelRequest(readCommandAlarm);
    readCommandAlarm = NULL;
  }

  {
    Queue *queues[] = {
      getKeyEventQueue(0),
      getCommandQueue(0)
    };

    Queue **queue = queues;
    Queue **end = queue + ARRAY_COUNT(queues);

    while (queue < end) {
      if (*queue) deleteElements(*queue);
      queue += 1;
    }
  }
}

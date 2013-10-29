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
#include "prefs.h"
#include "api_control.h"
#include "ktbdefs.h"
#include "scr.h"
#include "brltty.h"

#define LOG_LEVEL LOG_DEBUG

typedef struct CommandEnvironmentStruct CommandEnvironment;
typedef struct CommandHandlerLevelStruct CommandHandlerLevel;

struct CommandHandlerLevelStruct {
  CommandHandlerLevel *previousLevel;
  const char *levelName;

  CommandHandler *handleCommand;
  void *handlerData;
  KeyTableCommandContext commandContext;
};

struct CommandEnvironmentStruct {
  CommandEnvironment *previousEnvironment;
  const char *environmentName;

  CommandHandlerLevel *handlerStack;
  CommandPreprocessor *preprocessCommand;
  CommandPostprocessor *postprocessCommand;
  unsigned handlingCommand:1;
};

static CommandEnvironment *commandEnvironmentStack = NULL;

static CommandHandlerLevel **
getCommandHandlerTop (void) {
  return &commandEnvironmentStack->handlerStack;
}

KeyTableCommandContext
getCurrentCommandContext (void) {
  const CommandHandlerLevel *chl = *getCommandHandlerTop();
  KeyTableCommandContext context = chl? chl->commandContext: KTB_CTX_DEFAULT;

  if (context == KTB_CTX_DEFAULT) context = getScreenCommandContext();
  return context;
}

int
handleCommand (int command) {
  {
    int real = command;

    if (prefs.skipIdenticalLines) {
      switch (command & BRL_MSK_CMD) {
        case BRL_CMD_LNUP:
          real = BRL_CMD_PRDIFLN;
          break;

        case BRL_CMD_LNDN:
          real = BRL_CMD_NXDIFLN;
          break;

        case BRL_CMD_PRDIFLN:
          real = BRL_CMD_LNUP;
          break;

        case BRL_CMD_NXDIFLN:
          real = BRL_CMD_LNDN;
          break;

        default:
          break;
      }
    }

    if (real == command) {
      logCommand(command);
    } else {
      real |= (command & ~BRL_MSK_CMD);
      logTransformedCommand(command, real);
      command = real;
    }
  }

  {
    const CommandEnvironment *env = commandEnvironmentStack;
    const CommandHandlerLevel *chl = env->handlerStack;

    while (chl) {
      if (chl->handleCommand(command, chl->handlerData)) return 1;
      chl = chl->previousLevel;
    }
  }

  logMessage(LOG_WARNING, "%s: %04X", gettext("unhandled command"), command);
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

static void setCommandAlarm (void *data);
static AsyncHandle commandAlarm = NULL;

static void
handleCommandAlarm (const AsyncAlarmCallbackParameters *parameters) {
  Queue *queue = getCommandQueue(0);

  asyncDiscardHandle(commandAlarm);
  commandAlarm = NULL;

  if (queue) {
    int command = dequeueCommand(queue);

    if (command != EOF) {
      CommandEnvironment *env = commandEnvironmentStack;
      void *state;
      int handled;

      env->handlingCommand = 1;
      state = env->preprocessCommand? env->preprocessCommand(): NULL;
      handled = handleCommand(command);
      if (env->postprocessCommand) env->postprocessCommand(state, command, handled);
      env->handlingCommand = 0;
    }
  }

  setCommandAlarm(parameters->data);
}

static void
setCommandAlarm (void *data) {
  if (!commandAlarm) {
    const CommandEnvironment *env = commandEnvironmentStack;

    if (env && !env->handlingCommand) {
      Queue *queue = getCommandQueue(0);

      if (queue && (getQueueSize(queue) > 0)) {
        asyncSetAlarmIn(&commandAlarm, 0, handleCommandAlarm, data);
        logMessage(LOG_LEVEL, "set command alarm");
      }
    }
  }
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
          setCommandAlarm(NULL);
          return 1;
        }

        free(item);
      }
    }
  }

  return 0;
}

int
pushCommandHandler (
  const char *name,
  KeyTableCommandContext context,
  CommandHandler *handler, void *data
) {
  CommandHandlerLevel *chl;

  if ((chl = malloc(sizeof(*chl)))) {
    memset(chl, 0, sizeof(*chl));
    chl->levelName = name;
    chl->handleCommand = handler;
    chl->handlerData = data;
    chl->commandContext = context;

    {
      CommandHandlerLevel **top = getCommandHandlerTop();

      chl->previousLevel = *top;
      *top = chl;
    }

    logMessage(LOG_LEVEL, "pushed command handler: %s", chl->levelName);
    return 1;
  } else {
    logMallocError();
  }

  return 0;
}

int
popCommandHandler (void) {
  CommandHandlerLevel **top = getCommandHandlerTop();
  CommandHandlerLevel *chl = *top;

  if (!chl) return 0;
  *top = chl->previousLevel;

  logMessage(LOG_LEVEL, "popped command handler: %s", chl->levelName);
  free(chl);
  return 1;
}

int
pushCommandEnvironment (
  const char *name,
  CommandPreprocessor *preprocessCommand,
  CommandPostprocessor *postprocessCommand
) {
  CommandEnvironment *env;

  if ((env = malloc(sizeof(*env)))) {
    memset(env, 0, sizeof(*env));
    env->environmentName = name;
    env->handlerStack = NULL;
    env->preprocessCommand = preprocessCommand;
    env->postprocessCommand = postprocessCommand;
    env->handlingCommand = 0;

    env->previousEnvironment = commandEnvironmentStack;
    commandEnvironmentStack = env;
    setCommandAlarm(NULL);

    logMessage(LOG_LEVEL, "pushed command environment: %s", env->environmentName);
    return 1;
  } else {
    logMallocError();
  }

  return 0;
}

int
popCommandEnvironment (void) {
  CommandEnvironment *env = commandEnvironmentStack;

  if (!env) return 0;
  while (popCommandHandler());
  commandEnvironmentStack = env->previousEnvironment;

  if (commandAlarm) {
    const CommandEnvironment *env = commandEnvironmentStack;

    if (!env || env->handlingCommand) {
      asyncCancelRequest(commandAlarm);
      commandAlarm = NULL;
      logMessage(LOG_LEVEL, "cancelled command alarm");
    }
  }

  logMessage(LOG_LEVEL, "popped command environment: %s", env->environmentName);
  free(env);
  return 1;
}

int
beginCommandHandling (void) {
  commandEnvironmentStack = NULL;
  return pushCommandEnvironment("initial", NULL, NULL);
}

void
endCommandHandling (void) {
  while (popCommandEnvironment());
}

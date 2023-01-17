/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "scr_terminal.h"
#include "msg_queue.h"
#include "utf8.h"
#include "hostcmd.h"
#include "parse.h"
#include "file.h"
#include "program.h"
#include "async_handle.h"
#include "async_io.h"
#include "embed.h"

typedef enum {
  PARM_DIRECTORY,
  PARM_EMULATOR,
  PARM_GROUP,
  PARM_HOME,
  PARM_PATH,
  PARM_SHELL,
  PARM_USER,
} ScreenParameters;

#define SCRPARMS "directory", "emulator", "group", "home", "path", "shell", "user"
#include "scr_driver.h"

static char *directoryParameter = NULL;
static char *emulatorParameter = NULL;
static char *groupParameter = NULL;
static char *homeParameter = NULL;
static char *pathParameter = NULL;
static char *shellParameter = NULL;
static char *userParameter = NULL;

static void
setParameter (char **variable, char **parameters, ScreenParameters parameter) {
  char *value = parameters[parameter];
  if (value && !*value) value = NULL;
  *variable = value;
}

static int
processParameters_TerminalEmulatorScreen (char **parameters) {
  setParameter(&directoryParameter, parameters, PARM_DIRECTORY);
  setParameter(&emulatorParameter, parameters, PARM_EMULATOR);
  setParameter(&groupParameter, parameters, PARM_GROUP);
  setParameter(&homeParameter, parameters, PARM_HOME);
  setParameter(&pathParameter, parameters, PARM_PATH);
  setParameter(&shellParameter, parameters, PARM_SHELL);
  setParameter(&userParameter, parameters, PARM_USER);
  return 1;
}

static void
handleException (const char *cause) {
  logMessage(LOG_CATEGORY(SCREEN_DRIVER), "stopping: %s", cause);
  brlttyInterrupt(WAIT_STOP);
}

static AsyncHandle emulatorMonitorHandle = NULL;
static FILE *emulatorStream = NULL;
static char *emulatorStreamBuffer = NULL;
static size_t emulatorStreamBufferSize = 0;

static const char *problemText = NULL;
static ScreenSegmentHeader *screenSegment = NULL;
static ScreenSegmentHeader *cachedSegment = NULL;

static int haveTerminalMessageQueue = 0;
static int terminalMessageQueue;
static int haveSegmentUpdatedHandler = 0;
static int haveEmulatorExitingHandler = 0;

static int
sendTerminalMessage (MessageType type, const void *content, size_t length) {
  if (!haveTerminalMessageQueue) return 0;
  return sendMessage(terminalMessageQueue, type, content, length, 0);
}

static void
messageHandler_segmentUpdated (const MessageHandlerParameters *parameters) {
  mainScreenUpdated();
}

static void
messageHandler_emulatorExiting (const MessageHandlerParameters *parameters) {
  handleException("emulator exiting");
}

static void
enableMessages (key_t key) {
  haveTerminalMessageQueue = getMessageQueue(&terminalMessageQueue, key);

  if (haveTerminalMessageQueue) {
    haveSegmentUpdatedHandler = startMessageReceiver(
      "screen-segment-updated-receiver",
      terminalMessageQueue, TERM_MSG_SEGMENT_UPDATED,
      0, messageHandler_segmentUpdated, NULL
    );

    haveEmulatorExitingHandler = startMessageReceiver(
      "terminal-emulator-exiting-receiver",
      terminalMessageQueue, TERM_MSG_EMULATOR_EXITING,
      0, messageHandler_emulatorExiting, NULL
    );
  }
}

static int
accessSegmentForPath (const char *path) {
  key_t key;

  if (makeTerminalKey(&key, path)) {
    if ((screenSegment = getScreenSegmentForKey(key))) {
      problemText = gettext("no screen cache");
      enableMessages(key);
      return 1;
    } else {
      problemText = gettext("screen not accessible");
    }
  }

  return 0;
}

static void
destruct_TerminalEmulatorScreen (void) {
  brlttyDisableInterrupt();

  if (emulatorMonitorHandle) {
    asyncCancelRequest(emulatorMonitorHandle);
    emulatorMonitorHandle = NULL;
  }

  if (emulatorStream) {
    fclose(emulatorStream);
    emulatorStream = NULL;
  }

  if (emulatorStreamBuffer) {
    free(emulatorStreamBuffer);
    emulatorStreamBuffer = NULL;
  }

  if (screenSegment) {
    detachScreenSegment(screenSegment);
    screenSegment = NULL;
  }

  if (cachedSegment) {
    free(cachedSegment);
    cachedSegment = NULL;
  }
}

static void
driverDirectiveHandler_path (const char *const *operands) {
  const char *path = operands[0];

  if (path) {
    if (!screenSegment) {
      accessSegmentForPath(path);
    }
  }
}

typedef void DriverDirectiveHandler (const char *const *operands);

typedef struct {
  const char *name;
  DriverDirectiveHandler *handler;
} DriverDirectiveEntry;

#define DRIVER_DIRECTIVE_ENTRY(directive) { \
  .name = #directive, \
  .handler = driverDirectiveHandler_ ## directive, \
}

static const DriverDirectiveEntry driverDirectiveTable[] = {
  DRIVER_DIRECTIVE_ENTRY(path),
};

static int
handleDriverDirective (const char *line) {
  char buffer[strlen(line) + 1];
  strcpy(buffer, line);

  const char *operandTable[9];
  unsigned int operandCount = 0;

  {
    char *string = buffer;

    while (operandCount < (ARRAY_COUNT(operandTable) - 1)) {
      static const char delimiters[] = " ";
      char *next = strtok(string, delimiters);
      if (!next) break;

      operandTable[operandCount++] = next;
      string = NULL;
    }

    operandTable[operandCount] = NULL;
  }

  if (operandCount > 0) {
    const char **operands = operandTable;
    const char *name = *operands++;

    const DriverDirectiveEntry *entry = driverDirectiveTable;
    const DriverDirectiveEntry *end = entry + ARRAY_COUNT(driverDirectiveTable);

    while (entry < end) {
      if (strcasecmp(name, entry->name) == 0) {
        entry->handler(operands);
        return 1;
      }

      entry += 1;
    }
  }

  return 0;
}

ASYNC_MONITOR_CALLBACK(emEmulatorMonitor) {
  const char *line;

  {
    int ok = readLine(
      emulatorStream, &emulatorStreamBuffer,
      &emulatorStreamBufferSize, NULL
    );

    if (!ok) {
      const char *cause =
        ferror(emulatorStream)? "emulator stream error":
        feof(emulatorStream)? "end of emulator stream":
        "emulator monitor failure";

      handleException(cause);
      return 0;
    }

    line = emulatorStreamBuffer;
  }

  if (!handleDriverDirective(line)) {
    logMessage(LOG_NOTICE, "%s", line);
  }

  return 1;
}

static char *
makeDefaultEmulatorPath (void) {
  typedef char *PathMaker (const char *name);

  static PathMaker *pathMakers[] = {
    makeProgramPath,
    makeCommandPath,
  };

  PathMaker **pathMaker = pathMakers;
  PathMaker **end = pathMaker + ARRAY_COUNT(pathMakers);

  while (pathMaker < end) {
    char *path = (*pathMaker)("brltty-pty");

    if (path) {
      if (testProgramPath(path)) {
        logMessage(LOG_CATEGORY(SCREEN_DRIVER),
          "default terminal emulator: %s", path
        );

        return path;
      }

      free(path);
    }

    pathMaker += 1;
  }

  logMessage(LOG_WARNING, "default terminal emulator not found");
  return NULL;
}

static int
startEmulator (void) {
  char *emulator = emulatorParameter;

  if (!emulator) {
    if (!(emulator = makeDefaultEmulatorPath())) {
      return 0;
    }
  }

  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
    "terminal emulator command: %s", emulator
  );

  const char *arguments[13];
  unsigned int argumentCount = 0;

  arguments[argumentCount++] = emulator;
  arguments[argumentCount++] = "--driver-directives";

  if (userParameter) {
    arguments[argumentCount++] = "--user";
    arguments[argumentCount++] = userParameter;
  }

  if (groupParameter) {
    arguments[argumentCount++] = "--group";
    arguments[argumentCount++] = groupParameter;
  }

  if (directoryParameter) {
    arguments[argumentCount++] = "--working-directory";
    arguments[argumentCount++] = directoryParameter;
  }

  if (homeParameter) {
    arguments[argumentCount++] = "--home-directory";
    arguments[argumentCount++] = homeParameter;
  }

  arguments[argumentCount++] = "--";
  if (shellParameter) arguments[argumentCount++] = shellParameter;
  arguments[argumentCount++] = NULL;

  HostCommandOptions options;
  initializeHostCommandOptions(&options);
  options.asynchronous = 1;
  options.standardError = &emulatorStream;

  int exitStatus = runHostCommand(arguments, &options);
  if (emulator != emulatorParameter) free(emulator);
  emulator = NULL;

  if (!exitStatus) {
    detachStandardStreams();

    if (asyncMonitorFileInput(&emulatorMonitorHandle, fileno(emulatorStream), emEmulatorMonitor, NULL)) {
      return 1;
    }
  }

  return 0;
}

static int
construct_TerminalEmulatorScreen (void) {
  brlttyEnableInterrupt();

  emulatorMonitorHandle = NULL;
  emulatorStream = NULL;
  emulatorStreamBuffer = NULL;
  emulatorStreamBufferSize = 0;

  problemText = gettext("screen not available");
  screenSegment = NULL;
  cachedSegment = NULL;

  haveTerminalMessageQueue = 0;
  haveSegmentUpdatedHandler = 0;
  haveEmulatorExitingHandler = 0;

  if (pathParameter) {
    if (accessSegmentForPath(pathParameter)) return 1;
  } else if (startEmulator()) {
    problemText = gettext("screen not attached");
    return 1;
  } else {
    problemText = gettext("no screen emulator");
  }

  handleException("driver construction failure");
//destruct_TerminalEmulatorScreen();
  return 0;
}

static int
poll_TerminalEmulatorScreen (void) {
  return !haveSegmentUpdatedHandler;
}

static int
refresh_TerminalEmulatorScreen (void) {
  if (!screenSegment) return 0;
  size_t size = screenSegment->segmentSize;

  if (cachedSegment) {
    if (cachedSegment->segmentSize != size) {
      logMessage(LOG_CATEGORY(SCREEN_DRIVER), "deallocating old screen cache");
      free(cachedSegment);
      cachedSegment = NULL;
    }
  }

  if (!cachedSegment) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER), "allocating new screen cache");

    if (!(cachedSegment = malloc(size))) {
      logMallocError();
      return 0;
    }
  }

  memcpy(cachedSegment, screenSegment, size);
  return 1;
}

static ScreenSegmentHeader *
getSegment (void) {
  ScreenSegmentHeader *segment = cachedSegment;
  if (!segment) segment = screenSegment;
  return segment;
}

static int
currentVirtualTerminal_TerminalEmulatorScreen (void) {
  ScreenSegmentHeader *segment = getSegment();
  return segment? segment->screenNumber: 0;
}

static void
describe_TerminalEmulatorScreen (ScreenDescription *description) {
  ScreenSegmentHeader *segment = getSegment();

  if (!segment) {
    description->unreadable = problemText;
    description->cols = strlen(description->unreadable);
    description->rows = 1;
    description->posx = 0;
    description->posy = 0;
  } else {
    description->number = currentVirtualTerminal_TerminalEmulatorScreen();
    description->rows = segment->screenHeight;
    description->cols = segment->screenWidth;
    description->posy = segment->cursorRow;
    description->posx = segment->cursorColumn;
  }
}

static void
setScreenAttribute (ScreenAttributes *attributes, ScreenAttributes attribute, unsigned char level, int bright) {
  if (level >= 0X20) {
    *attributes |= attribute;
    if (bright && (level >= 0XD0)) *attributes |= SCR_ATTR_FG_BRIGHT;
  }
}

static int
readCharacters_TerminalEmulatorScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  ScreenSegmentHeader *segment = getSegment();

  if (!segment) {
    setScreenMessage(box, buffer, problemText);
    return 1;
  }

  if (validateScreenBox(box, segment->screenWidth, segment->screenHeight)) {
    ScreenCharacter *target = buffer;

    for (unsigned int row=0; row<box->height; row+=1) {
      const ScreenSegmentCharacter *source =
        getScreenCharacter(segment, box->top + row, box->left, NULL);

      for (unsigned int column=0; column<box->width; column+=1) {
        target->text = source->text;

        {
          ScreenAttributes *attributes = &target->attributes;
          *attributes = 0;
          if (source->blink) *attributes |= SCR_ATTR_BLINK;

          setScreenAttribute(attributes, SCR_ATTR_FG_RED,   source->foreground.red,   1);
          setScreenAttribute(attributes, SCR_ATTR_FG_GREEN, source->foreground.green, 1);
          setScreenAttribute(attributes, SCR_ATTR_FG_BLUE,  source->foreground.blue,  1);

          setScreenAttribute(attributes, SCR_ATTR_BG_RED,   source->background.red,   0);
          setScreenAttribute(attributes, SCR_ATTR_BG_GREEN, source->background.green, 0);
          setScreenAttribute(attributes, SCR_ATTR_BG_BLUE,  source->background.blue,  0);
        }

        source += 1;
        target += 1;
      }
    }

    return 1;
  }

  return 0;
}

static int
insertKey_TerminalEmulatorScreen (ScreenKey key) {
  setScreenKeyModifiers(&key, 0);

  wchar_t character = key & SCR_KEY_CHAR_MASK;
  Utf8Buffer utf8;
  size_t length = convertWcharToUtf8(character, utf8);

  return sendTerminalMessage(TERM_MSG_INPUT_TEXT, utf8, length);
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.currentVirtualTerminal = currentVirtualTerminal_TerminalEmulatorScreen;
  main->base.describe = describe_TerminalEmulatorScreen;
  main->base.readCharacters = readCharacters_TerminalEmulatorScreen;
  main->base.insertKey = insertKey_TerminalEmulatorScreen;
  main->base.poll = poll_TerminalEmulatorScreen;
  main->base.refresh = refresh_TerminalEmulatorScreen;

  main->processParameters = processParameters_TerminalEmulatorScreen;
  main->construct = construct_TerminalEmulatorScreen;
  main->destruct = destruct_TerminalEmulatorScreen;
}

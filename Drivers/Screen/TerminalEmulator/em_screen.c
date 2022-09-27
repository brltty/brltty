/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2022 by The BRLTTY Developers.
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
  PARM_EMULATOR,
} ScreenParameters;

#define SCRPARMS "emulator"
#include "scr_driver.h"

static char *emulatorCommand = NULL;

static int
processParameters_TerminalEmulatorScreen (char **parameters) {
  {
    char *command = parameters[PARM_EMULATOR];
    if (command && !*command) command = NULL;
    emulatorCommand = command;
  }

  return 1;
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
enableMessages (key_t key) {
  haveTerminalMessageQueue = getMessageQueue(&terminalMessageQueue, key);

  if (haveTerminalMessageQueue) {
    haveSegmentUpdatedHandler = startMessageReceiver(
      "screen-segment-updated-receiver",
      terminalMessageQueue, TERMINAL_MESSAGE_SEGMENT_UPDATED,
      0, messageHandler_segmentUpdated, NULL
    );
  }
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
endSession (const char *reason) {
  logMessage(LOG_CATEGORY(SCREEN_DRIVER), "session ending: %s", reason);
  brlttyInterrupt(WAIT_STOP);
}

static void
driverDirectiveHandler_path (const char *const *operands) {
  const char *path = operands[0];

  if (path) {
    if (!screenSegment) {
      key_t key;

      if (makeTerminalKey(&key, path)) {
        if ((screenSegment = getScreenSegmentForKey(key))) {
          problemText = gettext("no screen cache");
          enableMessages(key);
        } else {
          problemText = gettext("screen not accessible");
        }
      }
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
      const char *reason =
        ferror(emulatorStream)? "emulator stream error":
        feof(emulatorStream)? "end of emulator stream":
        "emulator monitor failure";

      endSession(reason);
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
  char *path = NULL;
  char *directory = NULL;

  if (changeStringSetting(&directory, COMMANDS_DIRECTORY)) {
    if (fixInstallPath(&directory)) {
      if ((path = makePath(directory, "brltty-pty"))) {
        logMessage(LOG_CATEGORY(SCREEN_DRIVER),
          "default terminal emulator: %s", path
        );
      }
    }
  }

  if (directory) free(directory);
  return path;
}

static int
startEmulator (void) {
  char *command = emulatorCommand;

  if (!command) {
    if (!(command = makeDefaultEmulatorPath())) {
      return 0;
    };
  }

  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
    "terminal emulator command: %s", command
  );

  const char *const arguments[] = {
    command, "--driver-directives"
  };

  const HostCommandOptions options = {
    .standardError = &emulatorStream,
    .asynchronous = 1,
  };

  int exitStatus = runHostCommand(arguments, &options);
  if (command != emulatorCommand) free(command);

  if (!exitStatus) {
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

  if (startEmulator()) {
    problemText = gettext("screen not attached");
    return 1;
  } else {
    problemText = gettext("no screen emulator");
  }

  destruct_TerminalEmulatorScreen();
  endSession("driver construction failure");
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

static int
currentVirtualTerminal_TerminalEmulatorScreen (void) {
  return 1;
}

static ScreenSegmentHeader *
getSegment (void) {
  ScreenSegmentHeader *segment = cachedSegment;
  if (!segment) segment = screenSegment;
  return segment;
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
    const ScreenSegmentCharacter *source = getScreenStart(segment);
    source += (box->top * segment->screenWidth) + box->left;
    unsigned int sourceRowIncrement = segment->screenWidth - box->width;

    for (unsigned int row=0; row<box->height; row+=1) {
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

      source += sourceRowIncrement;
    }

    return 1;
  }

  return 0;
}

static int
insertKey_TerminalEmulatorScreen (ScreenKey key) {
  setScreenKeyModifiers(&key, 0);

  wchar_t character = key & SCR_KEY_CHAR_MASK;
  const char *sequence = NULL;
  Utf8Buffer utf8;

  if (isSpecialKey(key)) {
    #define KEY(key, seq) case SCR_KEY_##key: sequence = seq; break;
    switch (character) {
      KEY(ENTER, "\r")
      KEY(TAB, "\t")
      KEY(BACKSPACE, "\x7f")
      KEY(ESCAPE, "\x1b")

      KEY(CURSOR_LEFT, "\x1b[D")
      KEY(CURSOR_RIGHT, "\x1b[C")
      KEY(CURSOR_UP, "\x1b[A")
      KEY(CURSOR_DOWN, "\x1b[B")

      KEY(PAGE_UP, "\x1b[5~")
      KEY(PAGE_DOWN, "\x1b[6~")

      KEY(HOME, "\x1b[1~")
      KEY(END, "\x1b[4~")

      KEY(INSERT, "\x1b[2~")
      KEY(DELETE, "\x1b[3~")

      KEY(F1, "\x1BOP")
      KEY(F2, "\x1BOQ")
      KEY(F3, "\x1BOR")
      KEY(F4, "\x1BOS")
      KEY(F5, "\x1B[15~")
      KEY(F6, "\x1B[17~")
      KEY(F7, "\x1B[18~")
      KEY(F8, "\x1B[19~")
      KEY(F9, "\x1B[20~")
      KEY(F10, "\x1B[21~")
      KEY(F11, "\x1B[23~")
      KEY(F12, "\x1B[24~")

      default:
        logMessage(LOG_WARNING, "unsupported key: %04X", key);
        return 0;
    }
    #undef KEY
  } else {
    convertWcharToUtf8(character, utf8);
    sequence = utf8;
  }

  return sendTerminalMessage(TERMINAL_MESSAGE_INPUT_TEXT, sequence, strlen(sequence));
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

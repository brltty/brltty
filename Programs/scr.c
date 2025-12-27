/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
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

/* scr.cc - The screen reading library
 *
 * Note: Although C++, this code requires no standard C++ library.
 * This is important as BRLTTY *must not* rely on too many
 * run-time shared libraries, nor be a huge executable.
 */

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "strfmt.h"
#include "unicode.h"
#include "scr.h"
#include "scr_utils.h"
#include "scr_real.h"
#include "driver.h"
#include "color.h"
#include "prefs.h"

MainScreen mainScreen;
BaseScreen *currentScreen = NULL;

int
isMainScreen (void) {
  return currentScreen == &mainScreen.base;
}

const char *const *
getScreenParameters (const ScreenDriver *driver) {
  return driver->parameters;
}

static void
initializeScreen (void) {
  screen->initialize(&mainScreen);
  currentScreen = &mainScreen.base;
  currentScreen->onForeground();
}

void
setNoScreen (void) {
  screen = &noScreen;
  initializeScreen();
}

int
constructScreenDriver (char **parameters) {
  initializeScreen();

  if (mainScreen.processParameters(parameters)) {
    if (mainScreen.construct()) {
      return 1;
    } else {
      logMessage(LOG_DEBUG, "screen driver initialization failed: %s",
                 screen->definition.code);
    }

    mainScreen.releaseParameters();
  }

  return 0;
}

void
destructScreenDriver (void) {
  mainScreen.destruct();
  mainScreen.releaseParameters();
}


int
pollScreen (void) {
  return currentScreen->poll();
}

int
refreshScreen (void) {
  return currentScreen->refresh();
}

static int detectSoftCursor (ScreenDescription *description);

void
describeScreen (ScreenDescription *description) {
  describeScreenObject(description, currentScreen);
  if (description->unreadable) description->quality = SCQ_NONE;
  if (prefs.softCursorDetection) detectSoftCursor(description);
}

static int
sameBackgroundColor (const ScreenColor *a, const ScreenColor *b) {
  if (a->usingRGB != b->usingRGB) return 0;

  if (a->usingRGB) {
    return memcmp(&a->background, &b->background, sizeof(RGBColor)) == 0;
  } else {
    // Compare only background bits (4-6) of VGA attributes
    return ((a->vgaAttributes ^ b->vgaAttributes) & 0x70) == 0;
  }
}

// Maximum number of distinct background colors to track
#define SOFT_CURSOR_MAX_COLORS 16

typedef struct {
  ScreenColor color;
  short x, y;
} SoftCursorCandidate;

static int
detectSoftCursor (ScreenDescription *description) {
  // Only search for soft cursor if hardware cursor is at screen edge
  // (column 0 or last column), which suggests the application is using
  // a visual cursor instead of positioning the hardware cursor properly.
  if ((description->posx != 0) && (description->posx != description->cols - 1)) {
    return 0;
  }

  // Read the entire screen
  unsigned int count = description->cols * description->rows;
  ScreenCharacter *buffer = malloc(count * sizeof(*buffer));
  if (!buffer) return 0;

  int result = 0;

  if (readScreen(0, 0, description->cols, description->rows, buffer)) {
    // Track candidates (seen once) and discarded (seen multiple times)
    SoftCursorCandidate candidates[SOFT_CURSOR_MAX_COLORS];
    ScreenColor discarded[SOFT_CURSOR_MAX_COLORS];
    int candidateCount = 0;
    int discardedCount = 0;

    for (unsigned int i = 0; i < count; i++) {
      const ScreenColor *bg = &buffer[i].color;
      int found = 0;

      // Check discarded first (most common case)
      for (int j = 0; j < discardedCount; j++) {
        if (sameBackgroundColor(bg, &discarded[j])) {
          found = 1;
          break;
        }
      }
      if (found) continue;

      // Check if it's in candidates
      found = 0;
      for (int j = 0; j < candidateCount; j++) {
        if (sameBackgroundColor(bg, &candidates[j].color)) {
          // Move to discarded
          if (discardedCount < SOFT_CURSOR_MAX_COLORS) {
            discarded[discardedCount++] = candidates[j].color;
          }
          // Remove from candidates by shifting
          for (int k = j; k < candidateCount - 1; k++) {
            candidates[k] = candidates[k + 1];
          }
          candidateCount--;
          found = 1;
          break;
        }
      }
      if (found) continue;

      // New color - add to candidates if space available
      if (candidateCount < SOFT_CURSOR_MAX_COLORS) {
        candidates[candidateCount].color = *bg;
        candidates[candidateCount].x = i % description->cols;
        candidates[candidateCount].y = i / description->cols;
        candidateCount++;
      }
    }

    // If exactly one candidate remains, that's the soft cursor
    if (candidateCount == 1) {
      description->posx = candidates[0].x;
      description->posy = candidates[0].y;
      result = 1;
    }
  }

  free(buffer);
  return result;
}

int
readScreen (short left, short top, short width, short height, ScreenCharacter *buffer) {
  const ScreenBox box = {
    .left = left,
    .top = top,
    .width = width,
    .height = height,
  };

  unsigned int count = box.width * box.height;
  clearScreenCharacters(buffer, count);
  if (!currentScreen->readCharacters(&box, buffer)) return 0;

  ScreenCharacter *character = buffer;
  const ScreenCharacter *end = character + count;

  while (character < end) {
    uint32_t text = character->text;

    if (!text || (text > UNICODE_LAST_CHARACTER)) {
      // This is not a valid Unicode character - return the replacement character.

      size_t index = character - buffer;
      unsigned int column = box.left + (index % box.width);
      unsigned int row = box.top + (index / box.width);

      logMessage(LOG_ERR,
        "invalid character U+%04"PRIX32 " on screen at [%u,%u]",
        text, column, row
      );

      character->text = UNICODE_REPLACEMENT_CHARACTER;
    }

    character += 1;
  }

  return 1;
}

int
readScreenText (short left, short top, short width, short height, wchar_t *buffer) {
  unsigned int count = width * height;
  ScreenCharacter characters[count];
  if (!readScreen(left, top, width, height, characters)) return 0;

  for (int i=0; i<count; i+=1) {
    buffer[i] = characters[i].text;
  }

  return 1;
}

int
sameScreenColors (const ScreenColor *color1, const ScreenColor *color2) {
  return memcmp(color1, color2, sizeof(*color1)) == 0;
}

unsigned char
getScreenColorAttributes (const ScreenColor *color) {
  if (!color->usingRGB) return color->vgaAttributes;

  static ScreenColor cachedColor = {.usingRGB=0};
  static unsigned char cachedAttributes = 0;

  if (!sameScreenColors(color, &cachedColor))  {
    ScreenColor c = *color;
    toVGAScreenColor(&c);

    cachedColor = *color;
    cachedAttributes = c.vgaAttributes;
  }

  return cachedAttributes;
}

STR_BEGIN_FORMATTER(formatScreenColor, const ScreenColor *color)
  const char *on = " on ";

  const char *styleNames[8];
  unsigned int styleCount = 0;

  if (color->usingRGB) {
    ColorNameBuffer foreground;
    rgbColorToName(foreground, sizeof(foreground), color->foreground);

    ColorNameBuffer background;
    rgbColorToName(background, sizeof(background), color->background);

    STR_PRINTF("%s%s%s", foreground, on, background);
    if (color->isBlinking) styleNames[styleCount++] = "blink";
    if (color->isBold) styleNames[styleCount++] = "bold";
    if (color->isItalic) styleNames[styleCount++] = "italic";
    if (color->hasUnderline) styleNames[styleCount++] = "underline";
    if (color->hasStrikeThrough) styleNames[styleCount++] = "strike";
  } else {
    unsigned char attributes = color->vgaAttributes;
    const char *foreground = vgaColorName(vgaGetForegroundColor(attributes));
    const char *background = vgaColorName(vgaGetBackgroundColor(attributes));

    STR_PRINTF("%s%s%s", foreground, on, background);
    if (attributes & VGA_BIT_BLINK) styleNames[styleCount++] = "blink";
  }

  if (styleCount > 0) {
    const char *delimiter = " (";

    for (unsigned int i=0; i<styleCount; i+=1) {
      STR_PRINTF("%s%s", delimiter, styleNames[i]);
      delimiter = ", ";
    }

    STR_PRINTF(")");
  }
STR_END_FORMATTER

int
insertScreenKey (ScreenKey key) {
  logMessage(LOG_CATEGORY(SCREEN_DRIVER), "insert key: 0X%04X", key);
  return currentScreen->insertKey(key);
}

ScreenPasteMode
getScreenPasteMode (void) {
  return currentScreen->getPasteMode();
}

int
routeScreenCursor (int column, int row, int screen) {
  return currentScreen->routeCursor(column, row, screen);
}

int
highlightScreenRegion (int left, int right, int top, int bottom) {
  return currentScreen->highlightRegion(left, right, top, bottom);
}

int
unhighlightScreenRegion (void) {
  return currentScreen->unhighlightRegion();
}

int
getScreenPointer (int *column, int *row) {
  return currentScreen->getPointer(column, row);
}

int
clearScreenTextSelection (void) {
  return currentScreen->clearSelection();
}

int
setScreenTextSelection (int startColumn, int startRow, int endColumn, int endRow) {
  if ((endRow < startRow) || ((endRow == startRow) && (endColumn < startColumn))) {
    int temp;

    temp = endColumn;
    endColumn = startColumn;
    startColumn = temp;

    temp = endRow;
    endRow = startRow;
    startRow = temp;
  }

  return currentScreen->setSelection(startColumn, startRow, endColumn, endRow);
}

int
currentVirtualTerminal (void) {
  return currentScreen->currentVirtualTerminal();
}

int
selectScreenVirtualTerminal (int vt) {
  return currentScreen->selectVirtualTerminal(vt);
}

int
switchScreenVirtualTerminal (int vt) {
  return currentScreen->switchVirtualTerminal(vt);
}

int
nextScreenVirtualTerminal (void) {
  return currentScreen->nextVirtualTerminal();
}

int
previousScreenVirtualTerminal (void) {
  return currentScreen->previousVirtualTerminal();
}

int
userVirtualTerminal (int number) {
  return mainScreen.userVirtualTerminal(number);
}

int
handleScreenCommands (int command, void *data) {
  return currentScreen->handleCommand(command);
}

KeyTableCommandContext
getScreenCommandContext (void) {
  return currentScreen->getCommandContext();
}

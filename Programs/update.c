/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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
#include "parameters.h"
#include "update.h"
#include "async_alarm.h"
#include "timing.h"
#include "unicode.h"
#include "charset.h"
#include "ttb.h"
#include "atb.h"
#include "brl_dots.h"
#include "spk.h"
#include "scr.h"
#include "scr_utils.h"
#include "prefs.h"
#include "status.h"
#include "blink.h"
#include "routing.h"
#include "api_control.h"
#include "brltty.h"

static int oldwinx;
static int oldwiny;

static int
checkPointer (void) {
  int moved = 0;
  int column, row;

  if (prefs.windowFollowsPointer && getScreenPointer(&column, &row)) {
    if (column != ses->ptrx) {
      if (ses->ptrx >= 0) moved = 1;
      ses->ptrx = column;
    }

    if (row != ses->ptry) {
      if (ses->ptry >= 0) moved = 1;
      ses->ptry = row;
    }

    if (moved) {
      if (column < ses->winx) {
        ses->winx = column;
      } else if (column >= (int)(ses->winx + textCount)) {
        ses->winx = column + 1 - textCount;
      }

      if (row < ses->winy) {
        ses->winy = row;
      } else if (row >= (int)(ses->winy + brl.textRows)) {
        ses->winy = row + 1 - brl.textRows;
      }
    }
  } else {
    ses->ptrx = ses->ptry = -1;
  }

  return moved;
}

static void
highlightWindow (void) {
  if (prefs.highlightWindow) {
    int left = 0;
    int right;
    int top = 0;
    int bottom;

    if (prefs.showAttributes) {
      right = left;
      bottom = top;
    } else {
      right = textCount - 1;
      bottom = brl.textRows - 1;
    }

    highlightScreenRegion(ses->winx+left, ses->winx+right, ses->winy+top, ses->winy+bottom);
  }
}

static const unsigned char cursorStyles[] = {
  [csUnderline] = (BRL_DOT_7 | BRL_DOT_8),
  [csBlock] = (BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6 | BRL_DOT_7 | BRL_DOT_8),
  [csLowerLeftDot] = (BRL_DOT_7),
  [csLowerRightDot] = (BRL_DOT_8)
};

unsigned char
getCursorDots (void) {
  return cursorStyles[prefs.cursorStyle];
}

static int
getCursorPosition (int x, int y) {
  int position = -1;

#ifdef ENABLE_CONTRACTED_BRAILLE
  if (isContracted) {
    int uncontractedOffset = getUncontractedCursorOffset(x, y);

    if (uncontractedOffset < contractedLength) {
      while (uncontractedOffset >= 0) {
        int contractedOffset = contractedOffsets[uncontractedOffset];
        if (contractedOffset != CTB_NO_OFFSET) {
          position = ((contractedOffset / textCount) * brl.textColumns) + textStart + (contractedOffset % textCount);
          break;
        }

        uncontractedOffset -= 1;
      }
    }
  } else
#endif /* ENABLE_CONTRACTED_BRAILLE */

  {
    if ((x >= ses->winx) && (x < (int)(ses->winx + textCount)) &&
        (y >= ses->winy) && (y < (int)(ses->winy + brl.textRows)) &&
        (x < scr.cols) && (y < scr.rows))
      position = ((y - ses->winy) * brl.textColumns) + textStart + x - ses->winx;
  }

  return position;
}

static void
overlayAttributesUnderline (unsigned char *cell, unsigned char attributes) {
  unsigned char dots;

  switch (attributes) {
    case SCR_COLOUR_FG_DARK_GREY | SCR_COLOUR_BG_BLACK:
    case SCR_COLOUR_FG_LIGHT_GREY | SCR_COLOUR_BG_BLACK:
    case SCR_COLOUR_FG_LIGHT_GREY | SCR_COLOUR_BG_BLUE:
    case SCR_COLOUR_FG_BLACK | SCR_COLOUR_BG_CYAN:
      return;

    case SCR_COLOUR_FG_BLACK | SCR_COLOUR_BG_LIGHT_GREY:
      dots = BRL_DOT_7 | BRL_DOT_8;
      break;

    case SCR_COLOUR_FG_WHITE | SCR_COLOUR_BG_BLACK:
    default:
      dots = BRL_DOT_8;
      break;
  }

  {
    BlinkDescriptor *blink = &attributesUnderlineBlinkDescriptor;

    requireBlinkDescriptor(blink);
    if (isBlinkVisible(blink)) *cell |= dots;
  }
}

static int
writeStatusCells (void) {
  if (braille->writeStatus) {
    const unsigned char *fields = prefs.statusFields;
    unsigned int length = getStatusFieldsLength(fields);

    if (length > 0) {
      unsigned char cells[length];        /* status cell buffer */
      unsigned int count = brl.statusColumns * brl.statusRows;

      memset(cells, 0, length);
      renderStatusFields(fields, cells);

      if (count > length) {
        unsigned char buffer[count];
        memcpy(buffer, cells, length);
        memset(&buffer[length], 0, count-length);
        if (!braille->writeStatus(&brl, buffer)) return 0;
      } else if (!braille->writeStatus(&brl, cells)) {
        return 0;
      }
    } else if (!clearStatusCells(&brl)) {
      return 0;
    }
  }

  return 1;
}

static int
showInfo (void) {
  const char *mode = "info";
  const size_t size = brl.textColumns * brl.textRows;
  char text[size + 1];

  if (!setStatusText(&brl, mode)) return 0;

  /* Here we must be careful. Some displays (e.g. Braille Lite 18)
   * are very small, and others (e.g. Bookworm) are even smaller.
   */
  if (size < 21) {
    wchar_t characters[size];
    size_t length;

    const int cellCount = 5;
    unsigned char cells[cellCount];
    char prefix[cellCount];

    {
      static const unsigned char fields[] = {
        sfCursorAndWindowColumn, sfCursorAndWindowRow, sfStateDots, sfEnd
      };

      memset(cells, 0, cellCount);
      renderStatusFields(fields, cells);
    }

    memset(prefix, 'x', cellCount);
    length = snprintf(text, sizeof(text), "%.*s %02d %c%c%c%c%c%c",
                      cellCount, prefix,
                      scr.number,
                      ses->trackCursor? 't': ' ',
                      prefs.showCursor? (prefs.blinkingCursor? 'B': 'v'):
                                        (prefs.blinkingCursor? 'b': ' '),
                      ses->displayMode? 'a': 't',
                      isFrozenScreen()? 'f': ' ',
                      prefs.textStyle? '6': '8',
                      prefs.blinkingCapitals? 'B': ' ');
    if (length > size) length = size;

    {
      unsigned int i;

      for (i=0; i<length; i+=1) {
        if (i < cellCount) {
          characters[i] = UNICODE_BRAILLE_ROW | cells[i];
        } else {
          wint_t character = convertCharToWchar(text[i]);
          if (character == WEOF) character = WC_C('?');
          characters[i] = character;
        }
      }
    }

    return writeBrailleCharacters(mode, characters, length);
  }

  STR_BEGIN(text, sizeof(text));
  STR_PRINTF("%02d:%02d %02d:%02d %02d %c%c%c%c%c%c",
             SCR_COLUMN_NUMBER(ses->winx), SCR_ROW_NUMBER(ses->winy),
             SCR_COLUMN_NUMBER(scr.posx), SCR_ROW_NUMBER(scr.posy),
             scr.number,
             ses->trackCursor? 't': ' ',
             prefs.showCursor? (prefs.blinkingCursor? 'B': 'v'):
                               (prefs.blinkingCursor? 'b': ' '),
             ses->displayMode? 'a': 't',
             isFrozenScreen()? 'f': ' ',
             prefs.textStyle? '6': '8',
             prefs.blinkingCapitals? 'B': ' ');

  {
    TimeFormattingData fmt;

    getTimeFormattingData(&fmt);
    STR_ADJUST(formatBrailleTime(STR_NEXT, STR_LEFT, &fmt));

    if (prefs.showSeconds) {
      scheduleUpdateIn("info clock second", millisecondsTillNextSecond(&fmt.value));
    } else {
      scheduleUpdateIn("info clock minute", millisecondsTillNextMinute(&fmt.value));
    }
  }

  STR_END;
  return writeBrailleText(mode, text);
}

#ifdef ENABLE_SPEECH_SUPPORT
static int wasAutospeaking;

static void
doAutospeak (int force) {
  static int oldScreen = -1;
  static int oldX = -1;
  static int oldY = -1;
  static int oldWidth = 0;
  static ScreenCharacter *oldCharacters = NULL;
  static size_t oldSize = 0;
  static int cursorAssumedStable = 0;

  int newScreen = scr.number;
  int newX = scr.posx;
  int newY = scr.posy;
  int newWidth = scr.cols;
  ScreenCharacter newCharacters[newWidth];

  readScreen(0, ses->winy, newWidth, 1, newCharacters);

  if (!spk.track.isActive) {
    const ScreenCharacter *characters = newCharacters;
    int column = 0;
    int count = newWidth;
    const char *reason = NULL;

    if (force) {
      reason = "current line";
    } else if (!oldCharacters) {
      reason = "initial line";
      count = 0;
    } else if ((newScreen != oldScreen) || (ses->winy != oldwiny) || (newWidth != oldWidth)) {
      if (!prefs.autospeakSelectedLine) count = 0;
      reason = "line selected";
    } else {
      int onScreen = (newX >= 0) && (newX < newWidth);

      if (!isSameRow(newCharacters, oldCharacters, newWidth, isSameText)) {
        if ((newY == ses->winy) && (newY == oldY) && onScreen) {
          /* Sometimes the cursor moves after the screen content has been
           * updated. Make sure we don't race ahead of such a cursor move
           * before assuming that it is actually stable.
           */
	  if ((newX == oldX) && !cursorAssumedStable) {
	    scheduleUpdate("autospeak cursor stability check");
	    cursorAssumedStable = 1;
	    return;
	  }

          if ((newX == oldX) &&
              isSameRow(newCharacters, oldCharacters, newX, isSameText)) {
            int oldLength = oldWidth;
            int newLength = newWidth;
            int x = newX;

            while (oldLength > oldX) {
              if (!iswspace(oldCharacters[oldLength-1].text)) break;
              oldLength -= 1;
            }
            if (oldLength < oldWidth) oldLength += 1;

            while (newLength > newX) {
              if (!iswspace(newCharacters[newLength-1].text)) break;
              newLength -= 1;
            }
            if (newLength < newWidth) newLength += 1;

            while (1) {
              int done = 1;

              if (x < newLength) {
                if (isSameRow(newCharacters+x, oldCharacters+oldX, newWidth-x, isSameText)) {
                  column = newX;
                  count = prefs.autospeakInsertedCharacters? (x - newX): 0;
                  reason = "characters inserted after cursor";
                  goto autospeak;
                }

                done = 0;
              }

              if (x < oldLength) {
                if (isSameRow(newCharacters+newX, oldCharacters+x, oldWidth-x, isSameText)) {
                  characters = oldCharacters;
                  column = oldX;
                  count = prefs.autospeakDeletedCharacters? (x - oldX): 0;
                  reason = "characters deleted after cursor";
                  goto autospeak;
                }

                done = 0;
              }

              if (done) break;
              x += 1;
            }
          }

          if (oldX < 0) oldX = 0;
          if ((newX > oldX) &&
              isSameRow(newCharacters, oldCharacters, oldX, isSameText) &&
              isSameRow(newCharacters+newX, oldCharacters+oldX, newWidth-newX, isSameText)) {
            column = oldX;
            count = newX - oldX;

            if (prefs.autospeakCompletedWords) {
              int last = column + count - 1;

              if (iswspace(characters[last].text)) {
                int first = column;

                while (first > 0) {
                  if (iswspace(characters[--first].text)) {
                    first += 1;
                    break;
                  }
                }

                if (first < column) {
                  while (last >= first) {
                    if (!iswspace(characters[last].text)) break;
                    last -= 1;
                  }

                  if (last > first) {
                    column = first;
                    count = last - first + 1;
                    reason = "word inserted";
                    goto autospeak;
                  }
                }
              }
            }

            if (!prefs.autospeakInsertedCharacters) count = 0;
            reason = "characters inserted before cursor";
            goto autospeak;
          }

          if (oldX >= oldWidth) oldX = oldWidth - 1;
          if ((newX < oldX) &&
              isSameRow(newCharacters, oldCharacters, newX, isSameText) &&
              isSameRow(newCharacters+newX, oldCharacters+oldX, oldWidth-oldX, isSameText)) {
            characters = oldCharacters;
            column = newX;
            count = prefs.autospeakDeletedCharacters? (oldX - newX): 0;
            reason = "characters deleted before cursor";
            goto autospeak;
          }
        }

        while (newCharacters[column].text == oldCharacters[column].text) ++column;
        while (newCharacters[count-1].text == oldCharacters[count-1].text) --count;
        count -= column;
        if (!prefs.autospeakReplacedCharacters) count = 0;
        reason = "characters replaced";
      } else if ((newY == ses->winy) && ((newX != oldX) || (newY != oldY)) && onScreen) {
        column = newX;
        count = prefs.autospeakSelectedCharacter? 1: 0;
        reason = "character selected";

        if (prefs.autospeakCompletedWords) {
          if ((newX > oldX) && (column >= 2)) {
            int length = newWidth;

            while (length > 0) {
              if (!iswspace(characters[--length].text)) {
                length += 1;
                break;
              }
            }

            if ((length + 1) == column) {
              int first = length - 1;

              while (first > 0) {
                if (iswspace(characters[--first].text)) {
                  first += 1;
                  break;
                }
              }

              if ((length -= first) > 1) {
                column = first;
                count = length;
                reason = "word appended";
                goto autospeak;
              }
            }
          }
        }
      } else {
        count = 0;
      }
    }

  autospeak:
    if (count) {
      characters += column;

      if (!reason) reason = "unknown reason";
      logMessage(LOG_CATEGORY(SPEECH_EVENTS), "autospeak: %s: %d", reason, count);

      speakCharacters(characters, count, 0);
    }
  }

  {
    size_t newSize = newWidth * sizeof(*oldCharacters);

    if (newSize > oldSize) {
      ScreenCharacter *newBuffer = malloc(newSize);

      if (!newBuffer) {
        logMallocError();
        return;
      }

      if (!oldCharacters) {
        registerProgramMemory("autospeak-buffer", &oldCharacters);
      }

      oldCharacters = newBuffer;
      oldSize = newSize;
    }

    memcpy(oldCharacters, newCharacters, newSize);
  }

  oldScreen = newScreen;
  oldX = newX;
  oldY = newY;
  oldWidth = newWidth;
  cursorAssumedStable = 0;
}
#endif /* ENABLE_SPEECH_SUPPORT */

static void
doUpdate (void) {
  int pointerMoved = 0;

  logMessage(LOG_CATEGORY(UPDATE_EVENTS), "starting");
  unrequireAllBlinkDescriptors();
  refreshScreen();

  {
    const SessionEntry *previousSession = ses;

    updateSessionAttributes();
    if (ses != previousSession) apiFlush();
  }

  if (scr.unreadable) {
    logMessage(LOG_CATEGORY(UPDATE_EVENTS), "screen unreadable: %s", scr.unreadable);
  } else {
    logMessage(LOG_CATEGORY(UPDATE_EVENTS), "screen: #%d %dx%d [%d,%d]",
               scr.number, scr.cols, scr.rows, scr.posx, scr.posy);
  }

  if (opt_releaseDevice) {
    if (scr.unreadable) {
      if (canBraille()) {
        logMessage(LOG_DEBUG, "suspending braille driver");
        writeStatusCells();
        writeBrailleText("wrn", scr.unreadable);
        apiSuspend();
        isSuspended = 1;
        logMessage(LOG_DEBUG, "braille driver suspended");
      }
    } else {
      if (isSuspended) {
        logMessage(LOG_DEBUG, "resuming braille driver");

        if (apiResume()) {
          logMessage(LOG_DEBUG, "braille driver resumed");
          isSuspended = 0;
        } else {
          logMessage(LOG_DEBUG, "braille driver not resumed");
        }
      }
    }
  }

  if (ses->trackCursor) {
#ifdef ENABLE_SPEECH_SUPPORT
    if (!spk.track.isActive)
#endif /* ENABLE_SPEECH_SUPPORT */
    {
      /* If cursor moves while blinking is on */
      if (prefs.blinkingCursor) {
        if (scr.posy != ses->trky) {
          /* turn off cursor to see what's under it while changing lines */
          setBlinkState(&screenCursorBlinkDescriptor, 0);
        } else if (scr.posx != ses->trkx) {
          /* turn on cursor to see it moving on the line */
          setBlinkState(&screenCursorBlinkDescriptor, 1);
        }
      }

      /* If the cursor moves in cursor tracking mode: */
      if (!isRouting() && (scr.posx != ses->trkx || scr.posy != ses->trky)) {
        int oldx = ses->winx;
        int oldy = ses->winy;

        trackCursor(0);
        logMessage(LOG_CATEGORY(CURSOR_TRACKING),
                   "scr=%u csr=[%u,%u]->[%u,%u] win=[%u,%u]->[%u,%u]",
                   scr.number,
                   ses->trkx, ses->trky, scr.posx, scr.posy,
                   oldx, oldy, ses->winx, ses->winy);

        ses->spkx = ses->trkx = scr.posx;
        ses->spky = ses->trky = scr.posy;
      } else if (checkPointer()) {
        pointerMoved = 1;
      }
    }
  }

#ifdef ENABLE_SPEECH_SUPPORT
  if (spk.canAutospeak) {
    int isAutospeaking = autospeak();

    if (isAutospeaking) {
      doAutospeak(!wasAutospeaking);
    } else if (wasAutospeaking) {
      muteSpeech("autospeak disabled");
    }

    wasAutospeaking = isAutospeaking;
  }
#endif /* ENABLE_SPEECH_SUPPORT */

  /* There are a few things to take care of if the display has moved. */
  if ((ses->winx != oldwinx) || (ses->winy != oldwiny)) {
    if (!pointerMoved) highlightWindow();

    /* Attributes are blinking.
     * We could check to see if we changed screen, but that doesn't
     * really matter... this is mainly for when you are hunting up/down
     * for the line with attributes.
     */
    setBlinkState(&attributesUnderlineBlinkDescriptor, 1);
    /* problem: this still doesn't help when the braille window is
     * stationnary and the attributes themselves are moving
     * (example: tin).
     */

    if ((ses->spky < ses->winy) || (ses->spky >= (ses->winy + brl.textRows))) ses->spky = ses->winy;
    if ((ses->spkx < ses->winx) || (ses->spkx >= (ses->winx + textCount))) ses->spkx = ses->winx;

    oldwinx = ses->winx;
    oldwiny = ses->winy;
  }

  if (!brl.isOffline && canBraille()) {
    apiClaimDriver();

    if (infoMode) {
      if (!showInfo()) restartRequired = 1;
    } else {
      const unsigned int windowLength = brl.textColumns * brl.textRows;
      const unsigned int textLength = textCount * brl.textRows;
      wchar_t textBuffer[windowLength];

      memset(brl.buffer, 0, windowLength);
      wmemset(textBuffer, WC_C(' '), windowLength);

#ifdef ENABLE_CONTRACTED_BRAILLE
      isContracted = 0;

      if (isContracting()) {
        while (1) {
          int inputLength = scr.cols - ses->winx;
          ScreenCharacter inputCharacters[inputLength];
          wchar_t inputText[inputLength];

          int outputLength = textLength;
          unsigned char outputBuffer[outputLength];

          readScreen(ses->winx, ses->winy, inputLength, 1, inputCharacters);

          {
            int i;
            for (i=0; i<inputLength; ++i) {
              inputText[i] = inputCharacters[i].text;
            }
          }

          contractText(contractionTable,
                       inputText, &inputLength,
                       outputBuffer, &outputLength,
                       contractedOffsets, getContractedCursor());

          {
            int inputEnd = inputLength;

            if (contractedTrack) {
              if (outputLength == textLength) {
                int inputIndex = inputEnd;
                while (inputIndex) {
                  int offset = contractedOffsets[--inputIndex];
                  if (offset != CTB_NO_OFFSET) {
                    if (offset != outputLength) break;
                    inputEnd = inputIndex;
                  }
                }
              }

              if (scr.posx >= (ses->winx + inputEnd)) {
                int offset = 0;
                int length = scr.cols - ses->winx;
                int onspace = 0;

                while (offset < length) {
                  if ((iswspace(inputCharacters[offset].text) != 0) != onspace) {
                    if (onspace) break;
                    onspace = 1;
                  }
                  ++offset;
                }

                if ((offset += ses->winx) > scr.posx) {
                  ses->winx = (ses->winx + scr.posx) / 2;
                } else {
                  ses->winx = offset;
                }

                continue;
              }
            }
          }

          contractedStart = ses->winx;
          contractedLength = inputLength;
          contractedTrack = 0;
          isContracted = 1;

          if (ses->displayMode || prefs.showAttributes) {
            int inputOffset;
            int outputOffset = 0;
            unsigned char attributes = 0;
            unsigned char attributesBuffer[outputLength];

            for (inputOffset=0; inputOffset<contractedLength; ++inputOffset) {
              int offset = contractedOffsets[inputOffset];

              if (offset != CTB_NO_OFFSET) {
                while (outputOffset < offset) attributesBuffer[outputOffset++] = attributes;
                attributes = 0;
              }

              attributes |= inputCharacters[inputOffset].attributes;
            }

            while (outputOffset < outputLength) attributesBuffer[outputOffset++] = attributes;

            if (ses->displayMode) {
              for (outputOffset=0; outputOffset<outputLength; ++outputOffset) {
                outputBuffer[outputOffset] = convertAttributesToDots(attributesTable, attributesBuffer[outputOffset]);
              }
            } else {
              unsigned int i;

              for (i=0; i<outputLength; i+=1) {
                overlayAttributesUnderline(&outputBuffer[i], attributesBuffer[i]);
              }
            }
          }

          fillDotsRegion(textBuffer, brl.buffer,
                         textStart, textCount, brl.textColumns, brl.textRows,
                         outputBuffer, outputLength);
          break;
        }
      }

      if (!isContracted)
#endif /* ENABLE_CONTRACTED_BRAILLE */
      {
        int windowColumns = MIN(textCount, scr.cols-ses->winx);
        ScreenCharacter characters[textLength];

        readScreen(ses->winx, ses->winy, windowColumns, brl.textRows, characters);
        if (windowColumns < textCount) {
          /* We got a rectangular piece of text with readScreen but the display
           * is in an off-right position with some cells at the end blank
           * so we'll insert these cells and blank them.
           */
          int i;

          for (i=brl.textRows-1; i>0; i--) {
            memmove(characters + (i * textCount),
                    characters + (i * windowColumns),
                    windowColumns * sizeof(*characters));
          }

          for (i=0; i<brl.textRows; i++) {
            clearScreenCharacters(characters + (i * textCount) + windowColumns,
                                  textCount-windowColumns);
          }
        }

        /* convert to dots using the current translation table */
        if (ses->displayMode) {
          int row;

          for (row=0; row<brl.textRows; row+=1) {
            const ScreenCharacter *source = &characters[row * textCount];
            unsigned int start = (row * brl.textColumns) + textStart;
            unsigned char *target = &brl.buffer[start];
            wchar_t *text = &textBuffer[start];
            int column;

            for (column=0; column<textCount; column+=1) {
              text[column] = UNICODE_BRAILLE_ROW | (target[column] = convertAttributesToDots(attributesTable, source[column].attributes));
            }
          }
        } else {
          unsigned int row;

          for (row=0; row<brl.textRows; row+=1) {
            const ScreenCharacter *source = &characters[row * textCount];
            unsigned int start = (row * brl.textColumns) + textStart;
            unsigned char *target = &brl.buffer[start];
            wchar_t *text = &textBuffer[start];
            unsigned int column;

            for (column=0; column<textCount; column+=1) {
              const ScreenCharacter *character = &source[column];
              unsigned char *dots = &target[column];

              *dots = convertCharacterToDots(textTable, character->text);

              if (iswupper(character->text)) {
                BlinkDescriptor *blink = &uppercaseLettersBlinkDescriptor;

                requireBlinkDescriptor(blink);
                if (!isBlinkVisible(blink)) *dots = 0;
              }

              if (prefs.textStyle) *dots &= ~(BRL_DOT_7 | BRL_DOT_8);
              if (prefs.showAttributes) overlayAttributesUnderline(dots, character->attributes);

              text[column] = character->text;
            }
          }
        }
      }

      if ((brl.cursor = getCursorPosition(scr.posx, scr.posy)) >= 0) {
        if (showCursor()) {
          BlinkDescriptor *blink = &screenCursorBlinkDescriptor;

          requireBlinkDescriptor(blink);
          if (isBlinkVisible(blink)) brl.buffer[brl.cursor] |= getCursorDots();
        }
      }

      if (prefs.showSpeechCursor) {
        int position = getCursorPosition(ses->spkx, ses->spky);

        if (position >= 0) {
          if (position != brl.cursor) {
            BlinkDescriptor *blink = &speechCursorBlinkDescriptor;

            requireBlinkDescriptor(blink);
            if (isBlinkVisible(blink)) brl.buffer[position] |= cursorStyles[prefs.speechCursorStyle];
          }
        }
      }

      if (statusCount > 0) {
        const unsigned char *fields = prefs.statusFields;
        unsigned int length = getStatusFieldsLength(fields);

        if (length > 0) {
          unsigned char cells[length];
          memset(cells, 0, length);
          renderStatusFields(fields, cells);
          fillDotsRegion(textBuffer, brl.buffer,
                         statusStart, statusCount, brl.textColumns, brl.textRows,
                         cells, length);
        }

        fillStatusSeparator(textBuffer, brl.buffer);
      }

      if (!(writeStatusCells() && braille->writeWindow(&brl, textBuffer))) restartRequired = 1;
    }

    apiReleaseDriver();
  }

  resetAllBlinkDescriptors();
  logMessage(LOG_CATEGORY(UPDATE_EVENTS), "finished");
}

static void setUpdateAlarm (void);
static AsyncHandle updateAlarm;
static int updateSuspendCount;

static TimeValue updateTime;
static TimeValue earliestTime;

static void
enforceEarliestTime (void) {
  if (compareTimeValues(&updateTime, &earliestTime) < 0) {
    updateTime = earliestTime;
  }
}

static void
setUpdateDelay (int delay) {
  getMonotonicTime(&earliestTime);
  adjustTimeValue(&earliestTime, delay);
  enforceEarliestTime();
}

static void
setUpdateTime (int delay, const TimeValue *from, int ifEarlier) {
  TimeValue time;

  if (from) {
    time = *from;
  } else {
    getMonotonicTime(&time);
  }

  adjustTimeValue(&time, delay);

  if (!ifEarlier || (millisecondsBetween(&updateTime, &time) < 0)) {
    updateTime = time;
    enforceEarliestTime();
  }
}

void
scheduleUpdateIn (const char *reason, int delay) {
  setUpdateTime(delay, NULL, 1);
  if (updateAlarm) asyncResetAlarmTo(updateAlarm, &updateTime);
  logMessage(LOG_CATEGORY(UPDATE_EVENTS), "scheduled: %s", reason);
}

void
scheduleUpdate (const char *reason) {
  scheduleUpdateIn(reason, 0);
}

ASYNC_ALARM_CALLBACK(handleUpdateAlarm) {
  asyncDiscardHandle(updateAlarm);
  updateAlarm = NULL;

  suspendUpdates();
  setUpdateTime((pollScreen()? SCREEN_UPDATE_POLL_INTERVAL: (SECS_PER_DAY * MSECS_PER_SEC)),
                parameters->now, 0);

  doUpdate();
  setUpdateDelay(MAX((brl.writeDelay + 1), UPDATE_SCHEDULE_DELAY));
  brl.writeDelay = 0;

  resumeUpdates(0);
}

static void
setUpdateAlarm (void) {
  if (!updateSuspendCount && !updateAlarm) {
    asyncSetAlarmTo(&updateAlarm, &updateTime, handleUpdateAlarm, NULL);
  }
}

void
beginUpdates (void) {
  logMessage(LOG_CATEGORY(UPDATE_EVENTS), "begin");

  setUpdateDelay(0);
  setUpdateTime(0, NULL, 0);
  updateAlarm = NULL;
  updateSuspendCount = 0;

  oldwinx = -1;
  oldwiny = -1;

#ifdef ENABLE_SPEECH_SUPPORT
  wasAutospeaking = 0;
#endif /* ENABLE_SPEECH_SUPPORT */
}

void
suspendUpdates (void) {
  if (updateAlarm) {
    asyncCancelRequest(updateAlarm);
    updateAlarm = NULL;
  }

  updateSuspendCount += 1;
  logMessage(LOG_CATEGORY(UPDATE_EVENTS), "suspend: %u", updateSuspendCount);
}

void
resumeUpdates (int refresh) {
  if (!--updateSuspendCount) {
    setUpdateAlarm();
    if (refresh) scheduleUpdate("updates resumed");
  }

  logMessage(LOG_CATEGORY(UPDATE_EVENTS), "resume: %u", updateSuspendCount);
}

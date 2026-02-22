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

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "alert.h"
#include "message.h"
#include "cmd_queue.h"
#include "cmd_utils.h"
#include "cmd_clipboard.h"
#include "clipboard.h"
#include "copysmart.h"
#include "brl_cmds.h"
#include "scr.h"
#include "routing.h"
#include "file.h"
#include "datafile.h"
#include "utf8.h"
#include "ascii.h"
#include "core.h"

typedef struct {
  ClipboardObject *clipboard;

  struct {
    int column;
    int row;
    size_t offset;
  } begin;
} ClipboardCommandData;

static wchar_t *
cpbReadScreen (ClipboardCommandData *ccd, size_t *length, int fromColumn, int fromRow, int toColumn, int toRow) {
  wchar_t *newBuffer = NULL;
  int columns = toColumn - fromColumn + 1;
  int rows = toRow - fromRow + 1;

  if ((columns >= 1) && (rows >= 1)) {
    wchar_t fromBuffer[rows * columns];

    if (readScreenText(fromColumn, fromRow, columns, rows, fromBuffer)) {
      wchar_t toBuffer[rows * (columns + 1)];
      wchar_t *toAddress = toBuffer;
      const wchar_t *fromAddress = fromBuffer;

      for (int row=fromRow; row<=toRow; row+=1) {
        for (int column=fromColumn; column<=toColumn; column+=1) {
          wchar_t character = *fromAddress++;
          if (iswcntrl(character) || iswspace(character)) character = WC_C(' ');
          *toAddress++ = character;
        }

        if (row != toRow) *toAddress++ = WC_C('\r');
      }

      /* make a new permanent buffer of just the right size */
      {
        size_t newLength = toAddress - toBuffer;

        if ((newBuffer = allocateCharacters(newLength))) {
          wmemcpy(newBuffer, toBuffer, (*length = newLength));
        }
      }
    }
  }

  return newBuffer;
}

static void
cpbBeginOperation (ClipboardCommandData *ccd, int column, int row, int append) {
  ccd->begin.column = column;
  ccd->begin.row = row;
  ccd->begin.offset = append? getClipboardContentLength(ccd->clipboard): 0;
  alert(ALERT_CLIPBOARD_BEGIN);
}

static int
cpbEndOperation (ClipboardCommandData *ccd, const wchar_t *characters, size_t length, int insertCR) {
  lockMainClipboard();
  int copied = copyClipboardContent(ccd->clipboard, characters, length, ccd->begin.offset, insertCR);
  unlockMainClipboard();

  if (!copied) return 0;
  alert(ALERT_CLIPBOARD_END);
  onMainClipboardUpdated();
  return 1;
}

static int
cpbRectangularCopy (ClipboardCommandData *ccd, int column, int row) {
  if (row < ccd->begin.row) return 0;
  if (column < ccd->begin.column) return 0;

  int copied = 0;
  size_t length;
  wchar_t *buffer = cpbReadScreen(ccd, &length, ccd->begin.column, ccd->begin.row, column, row);

  if (buffer) {
    {
      const wchar_t *from = buffer;
      const wchar_t *end = from + length;
      wchar_t *to = buffer;
      int spaces = 0;

      while (from != end) {
        wchar_t character = *from++;

        switch (character) {
          case WC_C(' '):
            spaces += 1;
            continue;

          case WC_C('\r'):
            spaces = 0;
            break;

          default:
            break;
        }

        while (spaces) {
          *to++ = WC_C(' ');
          spaces -= 1;
        }

        *to++ = character;
      }

      length = to - buffer;
    }

    if (cpbEndOperation(ccd, buffer, length, 1)) copied = 1;
    free(buffer);
  }

  return copied;
}

static int
cpbLinearCopy (ClipboardCommandData *ccd, int column, int row) {
  if (row < ccd->begin.row) return 0;
  if ((row == ccd->begin.row) && (column < ccd->begin.column)) return 0;

  int copied = 0;
  ScreenDescription screen;
  describeScreen(&screen);

  {
    int rightColumn = screen.cols - 1;
    size_t length;
    wchar_t *buffer = cpbReadScreen(ccd, &length, 0, ccd->begin.row, rightColumn, row);

    if (buffer) {
      if (column < rightColumn) {
        wchar_t *start = buffer + length;

        while (start != buffer) {
          if (*--start == WC_C('\r')) {
            start += 1;
            break;
          }
        }

        {
          int adjustment = (column + 1) - (buffer + length - start);
          if (adjustment < 0) length += adjustment;
        }
      }

      if (ccd->begin.column) {
        wchar_t *start = wmemchr(buffer, WC_C('\r'), length);
        if (!start) start = buffer + length;
        if ((start - buffer) > ccd->begin.column) start = buffer + ccd->begin.column;
        if (start != buffer) wmemmove(buffer, start, (length -= start - buffer));
      }

      {
        const wchar_t *from = buffer;
        const wchar_t *end = from + length;
        wchar_t *to = buffer;
        int spaces = 0;
        int newlines = 0;

        while (from != end) {
          wchar_t character = *from++;

          switch (character) {
            case WC_C(' '):
              spaces += 1;
              continue;

            case WC_C('\r'):
              newlines += 1;
              continue;

            default:
              break;
          }

          if (newlines) {
            if ((newlines > 1) || (spaces > 0)) spaces = 1;
            newlines = 0;
          }

          while (spaces) {
            *to++ = WC_C(' ');
            spaces -= 1;
          }

          *to++ = character;
        }

        if (spaces || newlines) *to++ = WC_C(' ');
        length = to - buffer;
      }

      if (cpbEndOperation(ccd, buffer, length, 0)) copied = 1;
      free(buffer);
    }
  }

  return copied;
}

static int
pasteCharacters (const wchar_t *characters, size_t count) {
  const wchar_t *character = characters;
  const wchar_t *end = character + count;

  while (character < end) {
    if (!insertScreenKey(*character++)) return 0;
  }

  return 1;
}

static int
cpbPaste (ClipboardCommandData *ccd, unsigned int index, int useAlternateMode) {
  if (!isMainScreen()) return 0;
  if (isRouting()) return 0;

  if (!prefs.alternatePasteModeEnabled) useAlternateMode = 0;
  int bracketed;

  {
    ScreenPasteMode mode = getScreenPasteMode();

    switch (mode) {
      case SPM_UNKNOWN:
        bracketed = useAlternateMode;
        break;

      case SPM_BRACKETED:
        bracketed = !useAlternateMode;
        break;

      default:
        logMessage(LOG_WARNING, "unexpected screen paste mode: %d", mode);
        /* fall through */
      case SPM_PLAIN:
        bracketed = 0;
        break;
    }
  }

  int pasted = 0;
  lockMainClipboard();

  const wchar_t *characters;
  size_t length;

  if (index) {
    characters = getClipboardHistory(ccd->clipboard, index-1, &length);
  } else {
    characters = getClipboardContent(ccd->clipboard, &length);
  }

  if (characters) {
    while (length > 0) {
      size_t last = length - 1;
      if (!iswspace(characters[last])) break;
      length = last;
    }

    if (length > 0) {
      if (bracketed) {
        static const wchar_t sequence[] = {
          ASCII_ESC, WC_C('['), WC_C('2'), WC_C('0'), WC_C('0'), WC_C('~')
        };

        if (!pasteCharacters(sequence, ARRAY_COUNT(sequence))) {
          goto PASTE_FAILED;
        }
      }

      if (!pasteCharacters(characters, length)) {
        goto PASTE_FAILED;
      }

      if (bracketed) {
        static const wchar_t sequence[] = {
          ASCII_ESC, WC_C('['), WC_C('2'), WC_C('0'), WC_C('1'), WC_C('~')
        };

        if (!pasteCharacters(sequence, ARRAY_COUNT(sequence))) {
          goto PASTE_FAILED;
        }
      }
    }

    pasted = 1;
  }

PASTE_FAILED:
  unlockMainClipboard();
  return pasted;
}

static FILE *
cpbOpenFile (const char *mode) {
  const char *file = "clipboard";
  char *path = makeUpdatablePath(file);

  if (path) {
    FILE *stream = openDataFile(path, mode, 0);

    free(path);
    path = NULL;

    if (stream) return stream;
  }

  return NULL;
}

static int
cpbSave (ClipboardCommandData *ccd) {
  int ok = 0;

  lockMainClipboard();
    size_t length;
    const wchar_t *characters = getClipboardContent(ccd->clipboard, &length);

    if (length > 0) {
      FILE *stream = cpbOpenFile("w");

      if (stream) {
        if (writeUtf8Characters(stream, characters, length)) {
          ok = 1;
        }

        if (fclose(stream) == EOF) {
          logSystemError("fclose");
          ok = 0;
        }
      }
    }
  unlockMainClipboard();

  return ok;
}

static int
cpbRestore (ClipboardCommandData *ccd) {
  int ok = 0;
  FILE *stream = cpbOpenFile("r");

  if (stream) {
    int wasUpdated = 0;

    lockMainClipboard();
    {
      int isClear = 0;

      if (isClipboardEmpty(ccd->clipboard)) {
        isClear = 1;
      } else if (clearClipboardContent(ccd->clipboard)) {
        isClear = 1;
        wasUpdated = 1;
      }

      if (isClear) {
        ok = 1;

        size_t size = 0X1000;
        char buffer[size];
        size_t length = 0;

        do {
          size_t count = fread(&buffer[length], 1, (size - length), stream);
          int done = (length += count) < size;

          if (ferror(stream)) {
            logSystemError("fread");
            ok = 0;
          } else {
            const char *next = buffer;
            size_t left = length;

            while (left > 0) {
              const char *start = next;
              wint_t wi = convertUtf8ToWchar(&next, &left);

              if (wi == WEOF) {
                length = next - start;

                if (left > 0) {
                  logBytes(LOG_ERR, "invalid UTF-8 character", start, length);
                  ok = 0;
                  break;
                }

                memmove(buffer, start, length);
              } else {
                wchar_t wc = wi;

                if (appendClipboardContent(ccd->clipboard, &wc, 1)) {
                  wasUpdated = 1;
                } else {
                  ok = 0;
                  break;
                }
              }
            }
          }

          if (done) break;
        } while (ok);
      }
    }
    unlockMainClipboard();

    if (fclose(stream) == EOF) {
      logSystemError("fclose");
      ok = 0;
    }

    if (wasUpdated) onMainClipboardUpdated();
  }

  return ok;
}

static int
findCharacters (const wchar_t **address, size_t *length, const wchar_t *characters, size_t count) {
  const wchar_t *ptr = *address;
  size_t len = *length;

  while (count <= len) {
    const wchar_t *next = wmemchr(ptr, *characters, len);
    if (!next) break;

    len -= next - ptr;
    if (wmemcmp((ptr = next), characters, count) == 0) {
      *address = ptr;
      *length = len;
      return 1;
    }

    ++ptr, --len;
  }

  return 0;
}

static wchar_t *
cpbLinearize (
  ClipboardCommandData *ccd,
  int *linearLen, int *targetOffset
) {
  int column = ccd->begin.column;
  int row = ccd->begin.row;

  int scanRadius = 5;
  int startRow = row - scanRadius;
  if (startRow < 0) startRow = 0;

  int endRow = row + scanRadius;
  if (endRow >= scr.rows) endRow = scr.rows - 1;

  int numRows = endRow - startRow + 1;
  int cols = scr.cols;
  int totalCells = numRows * cols;

  wchar_t *buf = allocateCharacters(totalCells);
  if (!buf) return NULL;

  if (!readScreenText(0, startRow, cols, numRows, buf)) {
    free(buf);
    return NULL;
  }

  /* collapse multiple spaces into one in place,
   * tracking the target offset */
  int rawTarget = ((row - startRow) * cols) + column;
  int inSpace = 0;

  *linearLen = 0;
  *targetOffset = -1;

  for (int i = 0; i < totalCells; i += 1) {
    wchar_t ch = buf[i];

    if (ch == WC_C(' ')) {
      if (inSpace) {
        if (i == rawTarget) *targetOffset = *linearLen - 1;
        continue;
      }

      inSpace = 1;
    } else {
      inSpace = 0;
    }

    if (i == rawTarget) *targetOffset = *linearLen;
    buf[*linearLen] = ch;
    *linearLen += 1;
  }

  if (*targetOffset < 0) {
    free(buf);
    return NULL;
  }

  return buf;
}

static int
handleClipboardCommands (int command, void *data) {
  ClipboardCommandData *ccd = data;

  switch (command & BRL_MSK_CMD) {
    {
      int useAlternateMode;

    case BRL_CMD_PASTE:
      useAlternateMode = 0;
      goto doPaste;

    case BRL_CMD_PASTE_ALTMODE:
      useAlternateMode = 1;
      goto doPaste;

    doPaste:
      if (!cpbPaste(ccd, 0, useAlternateMode)) alert(ALERT_COMMAND_REJECTED);
      break;
    }

    case BRL_CMD_CLIP_SAVE:
      alert(cpbSave(ccd)? ALERT_COMMAND_DONE: ALERT_COMMAND_REJECTED);
      break;

    case BRL_CMD_CLIP_RESTORE:
      alert(cpbRestore(ccd)? ALERT_COMMAND_DONE: ALERT_COMMAND_REJECTED);
      break;

    case BRL_CMD_COPY_SMART:
      goto doSmartCopy;

    {
      int increment;

      const wchar_t *cpbBuffer;
      size_t cpbLength;

    case BRL_CMD_PRSEARCH:
      increment = -1;
      goto doSearch;

    case BRL_CMD_NXSEARCH:
      increment = 1;
      goto doSearch;

    doSearch:
      lockMainClipboard();
        if ((cpbBuffer = getClipboardContent(ccd->clipboard, &cpbLength))) {
          int found = 0;
          size_t count = cpbLength;

          if (count <= scr.cols) {
            int line = ses->winy;
            wchar_t buffer[scr.cols];
            wchar_t characters[count];

            {
              unsigned int i;
              for (i=0; i<count; i+=1) characters[i] = towlower(cpbBuffer[i]);
            }

            while ((line >= 0) && (line <= (int)(scr.rows - brl.textRows))) {
              const wchar_t *address = buffer;
              size_t length = scr.cols;
              readScreenText(0, line, length, 1, buffer);

              {
                for (size_t i=0; i<length; i+=1) buffer[i] = towlower(buffer[i]);
              }

              if (line == ses->winy) {
                if (increment < 0) {
                  int end = ses->winx + count - 1;
                  if (end < length) length = end;
                } else {
                  int start = ses->winx + textCount;
                  if (start > length) start = length;
                  address += start;
                  length -= start;
                }
              }

              if (findCharacters(&address, &length, characters, count)) {
                if (increment < 0) {
                  while (findCharacters(&address, &length, characters, count)) {
                    ++address, --length;
                  }
                }

                ses->winy = line;
                ses->winx = (address - buffer) / textCount * textCount;
                found = 1;
                break;
              }

              line += increment;
            }
          }

          if (!found) alert(ALERT_BOUNCE);
        } else {
          alert(ALERT_COMMAND_REJECTED);
        }
      unlockMainClipboard();

      break;
    }

    default: {
      int arg1 = BRL_CODE_GET(ARG, command);
      int arg2 = BRL_CODE_GET(EXT, command);

      switch (command & BRL_MSK_BLK) {
        {
          int append;
          int column, row;

        case BRL_CMD_BLK(CLIP_NEW):
          append = 0;
          goto doClipBegin;

        case BRL_CMD_BLK(CLIP_ADD):
          append = 1;
          goto doClipBegin;

        doClipBegin:
          if (getCharacterCoordinates(arg1, &row, &column, NULL, 0)) {
            cpbBeginOperation(ccd, column, row, append);
            break;
          }

          alert(ALERT_COMMAND_REJECTED);
          break;
        }

        {
          int append;
          int column, row;

        case BRL_CMD_BLK(COPY_SMART_NEW):
          append = 0;
          goto doSmartCopyBegin;

        case BRL_CMD_BLK(COPY_SMART_ADD):
          append = 1;
          goto doSmartCopyBegin;

        doSmartCopyBegin:
          if (getCharacterCoordinates(arg1, &row, &column, NULL, 0)) {
            cpbBeginOperation(ccd, column, row, append);
          } else {
            alert(ALERT_COMMAND_REJECTED);
            break;
          }

        doSmartCopy: {
          int linearLen, targetOffset;
          wchar_t *buf = cpbLinearize(ccd, &linearLen, &targetOffset);

          if (buf) {
            int matchOffset, matchLength;

            if (cpbSmartMatch(buf, linearLen, targetOffset, &matchOffset, &matchLength)) {
              cpbEndOperation(ccd, buf + matchOffset, matchLength, 0);
            } else {
              alert(ALERT_COMMAND_REJECTED);
            }

            free(buf);
          } else {
            alert(ALERT_COMMAND_REJECTED);
          }

          break;
        }
        }

        case BRL_CMD_BLK(COPY_RECT): {
          int column, row;

          if (getCharacterCoordinates(arg1, &row, NULL, &column, 1)) {
            if (cpbRectangularCopy(ccd, column, row)) {
              break;
            }
          }

          alert(ALERT_COMMAND_REJECTED);
          break;
        }

        case BRL_CMD_BLK(COPY_LINE): {
          int column, row;

          if (getCharacterCoordinates(arg1, &row, NULL, &column, 1)) {
            if (cpbLinearCopy(ccd, column, row)) {
              break;
            }
          }

          alert(ALERT_COMMAND_REJECTED);
          break;
        }

        {
          int append;

        case BRL_CMD_BLK(CLIP_COPY):
          append = 0;
          goto doClipCopy;

        case BRL_CMD_BLK(CLIP_APPEND):
          append = 1;
          goto doClipCopy;

        doClipCopy:
          if (arg2 > arg1) {
            int column1, row1;

            if (getCharacterCoordinates(arg1, &row1, &column1, NULL, 0)) {
              int column2, row2;

              if (getCharacterCoordinates(arg2, &row2, NULL, &column2, 1)) {
                cpbBeginOperation(ccd, column1, row1, append);
                if (cpbLinearCopy(ccd, column2, row2)) break;
              }
            }
          }

          alert(ALERT_COMMAND_REJECTED);
          break;
        }

        {
          int useAlternateMode;

        case BRL_CMD_BLK(PASTE_HISTORY):
          useAlternateMode = 0;
          goto doPasteHistory;

        case BRL_CMD_BLK(PASTE_HISTORY_ALTMODE):
          useAlternateMode = 1;
          goto doPasteHistory;

        doPasteHistory:
          if (!cpbPaste(ccd, arg1, useAlternateMode)) alert(ALERT_COMMAND_REJECTED);
          break;
        }

        default:
          return 0;
      }

      break;
    }
  }

  return 1;
}

static void
destroyClipboardCommandData (void *data) {
  ClipboardCommandData *ccd = data;
  free(ccd);
}

int
addClipboardCommands (void) {
  ClipboardCommandData *ccd;

  if ((ccd = malloc(sizeof(*ccd)))) {
    memset(ccd, 0, sizeof(*ccd));
    ccd->clipboard = getMainClipboard();

    ccd->begin.column = 0;
    ccd->begin.row = 0;
    ccd->begin.offset = 0;

    if (pushCommandHandler("clipboard", KTB_CTX_DEFAULT,
                           handleClipboardCommands, destroyClipboardCommandData, ccd)) {
      return 1;
    }

    free(ccd);
  } else {
    logMallocError();
  }

  return 0;
}

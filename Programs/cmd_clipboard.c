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
#include <ctype.h>

#include "log.h"
#include "alert.h"
#include "message.h"
#include "cmd_queue.h"
#include "cmd_utils.h"
#include "cmd_clipboard.h"
#include "clipboard.h"
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

static int
isURLChar (wchar_t ch) {
  if (iswalnum(ch)) return 1;

  switch (ch) {
    /* unreserved */
    case WC_C('-'): case WC_C('_'): case WC_C('.'): case WC_C('~'):
    /* reserved (gen-delims + sub-delims) */
    case WC_C(':'): case WC_C('/'): case WC_C('?'): case WC_C('#'):
    case WC_C('['): case WC_C(']'): case WC_C('@'):
    case WC_C('!'): case WC_C('$'): case WC_C('&'): case WC_C('\''):
    case WC_C('('): case WC_C(')'): case WC_C('*'): case WC_C('+'):
    case WC_C(','): case WC_C(';'): case WC_C('='): case WC_C('%'):
      return 1;

    default:
      return 0;
  }
}

static int
isEmailLocalChar (wchar_t ch) {
  if (iswalnum(ch)) return 1;

  switch (ch) {
    case WC_C('.'): case WC_C('_'): case WC_C('%'):
    case WC_C('+'): case WC_C('-'):
      return 1;

    default:
      return 0;
  }
}

static int
isEmailDomainChar (wchar_t ch) {
  if (iswalnum(ch)) return 1;

  switch (ch) {
    case WC_C('.'): case WC_C('-'):
      return 1;

    default:
      return 0;
  }
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
tryURLCopy (ClipboardCommandData *ccd, const wchar_t *buf, int len, int target) {
  /* scan backward and forward for URL characters */
  int start = target;
  while (start > 0 && isURLChar(buf[start - 1])) start -= 1;

  int end = target;
  while (end < len - 1 && isURLChar(buf[end + 1])) end += 1;

  int count = end - start + 1;
  const wchar_t *text = &buf[start];

  /* strip trailing punctuation that is typically not part of URLs */
  while (count > 0) {
    wchar_t last = text[count - 1];

    if (last == WC_C('.') || last == WC_C(',') || last == WC_C(';')) {
      count -= 1;
    } else if (last == WC_C(')') && !wmemchr(text, WC_C('('), count)) {
      count -= 1;
    } else {
      break;
    }
  }

  if (count < 1) return 0;

  /* validate: must contain "://" with a valid scheme before it */
  for (int i = 0; i < count - 2; i += 1) {
    if (text[i] == WC_C(':') &&
        text[i + 1] == WC_C('/') &&
        text[i + 2] == WC_C('/')) {
      if (i > 0 && iswalpha(text[0])) {
        int valid = 1;

        for (int j = 1; j < i; j += 1) {
          wchar_t sc = text[j];

          if (!iswalnum(sc) &&
              sc != WC_C('+') &&
              sc != WC_C('-') &&
              sc != WC_C('.')) {
            valid = 0;
            break;
          }
        }

        if (valid) return cpbEndOperation(ccd, text, count, 0);
      }

      break;
    }
  }

  return 0;
}

static int
tryEmailCopy (ClipboardCommandData *ccd, const wchar_t *buf, int len, int target) {
  /* target must be part of an email address */
  if (buf[target] != WC_C('@') &&
      !isEmailLocalChar(buf[target]) &&
      !isEmailDomainChar(buf[target])) return 0;

  /* find the '@' sign: scan outward from the target */
  int at = -1;

  if (buf[target] == WC_C('@')) {
    at = target;
  } else {
    /* scan left through local-part characters */
    for (int i = target - 1; i >= 0; i -= 1) {
      if (buf[i] == WC_C('@')) { at = i; break; }
      if (!isEmailLocalChar(buf[i])) break;
    }

    /* if not found, scan right through domain characters */
    if (at < 0) {
      for (int i = target + 1; i < len; i += 1) {
        if (buf[i] == WC_C('@')) { at = i; break; }
        if (!isEmailDomainChar(buf[i])) break;
      }
    }
  }

  if (at < 1 || at >= len - 1) return 0;

  /* scan backward from '@' for the local part */
  int start = at - 1;
  while (start > 0 && isEmailLocalChar(buf[start - 1])) start -= 1;
  if (!iswalnum(buf[start])) return 0;

  /* scan forward from '@' for the domain part */
  int end = at + 1;
  while (end < len - 1 && isEmailDomainChar(buf[end + 1])) end += 1;

  /* strip trailing dots and hyphens from domain */
  while (end > at + 1 && (buf[end] == WC_C('.') || buf[end] == WC_C('-'))) end -= 1;
  if (!iswalnum(buf[end])) return 0;

  /* domain must contain at least one dot */
  {
    int hasDot = 0;

    for (int i = at + 1; i <= end; i += 1) {
      if (buf[i] == WC_C('.')) { hasDot = 1; break; }
    }

    if (!hasDot) return 0;
  }

  int count = end - start + 1;
  return cpbEndOperation(ccd, &buf[start], count, 0);
}

static int
hasKnownTLD (const wchar_t *text, int len) {
  static const char *const knownTLDs[] = {
    "com", "org", "net", "edu", "gov",
    "mil", "int",
    "io", "dev", "app", "info", "biz",
    "name", "pro", "coop", "museum",
    NULL
  };

  /* find the last dot */
  int lastDot = -1;
  for (int i = len - 1; i >= 0; i -= 1) {
    if (text[i] == WC_C('.')) { lastDot = i; break; }
  }

  if (lastDot < 0 || lastDot >= len - 1) return 0;

  const wchar_t *tld = &text[lastDot + 1];
  int tldLen = len - lastDot - 1;

  /* two-letter TLDs are country codes - accept them all */
  if (tldLen == 2 && iswalpha(tld[0]) && iswalpha(tld[1])) return 1;

  for (const char *const *t = knownTLDs; *t; t += 1) {
    int knownLen = strlen(*t);
    if (knownLen != tldLen) continue;

    int match = 1;
    for (int i = 0; i < knownLen; i += 1) {
      if (towlower(tld[i]) != (*t)[i]) { match = 0; break; }
    }

    if (match) return 1;
  }

  return 0;
}

static int
tryHostnameCopy (ClipboardCommandData *ccd, const wchar_t *buf, int len, int target) {
  if (!isEmailDomainChar(buf[target])) return 0;

  /* scan backward and forward for hostname characters */
  int start = target;
  while (start > 0 && isEmailDomainChar(buf[start - 1])) start -= 1;

  int end = target;
  while (end < len - 1 && isEmailDomainChar(buf[end + 1])) end += 1;

  /* strip leading/trailing dots and hyphens */
  while (start <= end && (buf[start] == WC_C('.') || buf[start] == WC_C('-'))) start += 1;
  while (end >= start && (buf[end] == WC_C('.') || buf[end] == WC_C('-'))) end -= 1;

  int count = end - start + 1;
  const wchar_t *text = &buf[start];

  if (count < 1) return 0;

  /* must contain at least one dot */
  {
    int hasDot = 0;
    for (int i = 0; i < count; i += 1) {
      if (text[i] == WC_C('.')) { hasDot = 1; break; }
    }
    if (!hasDot) return 0;
  }

  /* labels between dots must be non-empty */
  {
    int prevDot = -1;
    for (int i = 0; i <= count; i += 1) {
      if (i == count || text[i] == WC_C('.')) {
        if (i - prevDot <= 1) return 0;
        prevDot = i;
      }
    }
  }

  /* require www. prefix or a known TLD */
  {
    int hasWWW = 0;

    if (count >= 4) {
      if (towlower(text[0]) == WC_C('w') &&
          towlower(text[1]) == WC_C('w') &&
          towlower(text[2]) == WC_C('w') &&
          text[3] == WC_C('.')) {
        hasWWW = 1;
      }
    }

    if (!hasWWW && !hasKnownTLD(text, count)) return 0;
  }

  return cpbEndOperation(ccd, text, count, 0);
}

static int
cpbSmartCopy (ClipboardCommandData *ccd) {
  int linearLen;
  int targetOffset;

  wchar_t *buf = cpbLinearize(ccd, &linearLen, &targetOffset);
  if (!buf) return 0;

  typedef int TryFunction (ClipboardCommandData *ccd, const wchar_t *characters, int count, int target);

  static TryFunction *const tryFunctions[] = {
    tryURLCopy,
    tryEmailCopy,
    tryHostnameCopy,
  };

  TryFunction *const *try = tryFunctions;
  TryFunction *const *end = try + ARRAY_COUNT(tryFunctions);

  while (try < end) {
    if ((*try)(ccd, buf, linearLen, targetOffset)) break;
    try += 1;
  }

  free(buf);
  return try < end;
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
      if (!cpbSmartCopy(ccd)) alert(ALERT_COMMAND_REJECTED);
      break;

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

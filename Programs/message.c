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

#include <string.h>

#include "embed.h"
#include "log.h"
#include "log_history.h"
#include "alert.h"
#include "message.h"
#include "defaults.h"
#include "async_wait.h"
#include "async_task.h"
#include "utf8.h"
#include "unicode.h"
#include "brl_utils.h"
#include "brl_cmds.h"
#include "clipboard.h"
#include "spk.h"
#include "ktb_types.h"
#include "ctb.h"
#include "update.h"
#include "cmd_queue.h"
#include "api_control.h"
#include "core.h"

int messageHoldTimeout = DEFAULT_MESSAGE_HOLD_TIMEOUT;

typedef struct {
  const char *mode;
  MessageOptions options;

  unsigned char presented:1;
  unsigned char deallocate:1;

  char text[0];
} MessageParameters;

typedef struct {
  const wchar_t *start;
  size_t length;
} MessageSegment;

typedef struct {
  const MessageParameters *parameters;

  int timeout;
  unsigned char endWait:1;
  unsigned char hold:1;
  unsigned char touch:1;

  struct {
    const MessageSegment *first;
    const MessageSegment *current;
    const MessageSegment *last;
  } segments;

  struct {
    ClipboardObject *main;
    const wchar_t *start;
    size_t offset;
  } clipboard;

  struct {
    const wchar_t *message;
    const wchar_t *characters;
    int *toMessageOffset;
  } braille;
} MessageData;

ASYNC_CONDITION_TESTER(testEndMessageWait) {
  const MessageData *mgd = data;
  return mgd->endWait;
}

static const wchar_t *
toMessageCharacter (int arg, int end, MessageData *mgd) {
  if (arg < 0) return NULL;
  int column = arg % brl.textColumns;
  int row = arg / brl.textColumns;
  if (row >= brl.textRows) return NULL;

  if ((column -= textStart) < 0) return NULL;
  if (column >= textCount) return NULL;

  const MessageSegment *segment = mgd->segments.current;
  size_t offset = (row * textCount) + column;

  if (offset >= segment->length) {
    if (!end) return NULL;
    offset = segment->length - 1;
  }

  const wchar_t *character = segment->start + offset;
  offset = character - mgd->braille.characters;

  if (mgd->braille.characters != mgd->braille.message) {
    const int *toMessageOffset = mgd->braille.toMessageOffset;

    if (toMessageOffset) {
      if (end) {
        while (toMessageOffset[++offset] == CTB_NO_OFFSET);
        // we're now at the start of the next contraction
      } else {
        while (toMessageOffset[offset] == CTB_NO_OFFSET) offset -= 1;
        // we're now at the start of the current contraction
      }

      offset = toMessageOffset[offset];
      if (end) offset -= 1;
    }
  }

  return mgd->braille.message + offset;
}

static void
beginClipboardCopy (const wchar_t *start, int append, MessageData *mgd) {
  mgd->clipboard.start = start;
  mgd->clipboard.offset = append? getClipboardContentLength(mgd->clipboard.main): 0;
  alert(ALERT_CLIPBOARD_BEGIN);
}

static int
endClipboardCopy (ClipboardObject *clipboard, const wchar_t *characters, size_t length, int insertCR, MessageData *mgd) {
  lockMainClipboard();
  int copied = copyClipboardContent(clipboard, characters, length, mgd->clipboard.offset, insertCR);
  unlockMainClipboard();

  if (!copied) return 0;
  alert(ALERT_CLIPBOARD_END);
  onMainClipboardUpdated();
  return 1;
}

static int
handleMessageCommands (int command, void *data) {
  MessageData *mgd = data;

  switch (command & BRL_MSK_CMD) {
    case BRL_CMD_LNUP:
    case BRL_CMD_PRDIFLN:
    case BRL_CMD_PRNBWIN:
    case BRL_CMD_FWINLTSKIP:
    case BRL_CMD_FWINLT: {
      if (mgd->segments.current > mgd->segments.first) {
        mgd->segments.current -= 1;
        mgd->endWait = 1;
      } else {
        alert(ALERT_BOUNCE);
      }

      mgd->hold = 1;
      return 1;
    }

    case BRL_CMD_LNDN:
    case BRL_CMD_NXDIFLN:
    case BRL_CMD_NXNBWIN:
    case BRL_CMD_FWINRTSKIP:
    case BRL_CMD_FWINRT: {
      if ((mgd->hold = mgd->segments.current < mgd->segments.last)) {
        mgd->segments.current += 1;
      }

      break;
    }

    default: {
      int arg1 = BRL_CODE_GET(ARG, command);
      int arg2 = BRL_CODE_GET(EXT, command);

      switch (command & BRL_MSK_BLK) {
        case BRL_CMD_BLK(TOUCH_AT):
          if ((mgd->touch = arg1 != BRL_MSK_ARG)) return 1;
          mgd->timeout = 1000;
          break;

        {
          int append;

        case BRL_CMD_BLK(CLIP_COPY):
          append = 0;
          goto doClipCopy;

        case BRL_CMD_BLK(CLIP_APPEND):
          append = 1;
          goto doClipCopy;

        doClipCopy:
          {
            const wchar_t *first = toMessageCharacter(arg1, 0, mgd);

            if (first) {
              const wchar_t *last = toMessageCharacter(arg2, 1, mgd);

              if (last) {
                beginClipboardCopy(first, append, mgd);
                size_t count = last - first + 1;

                if (endClipboardCopy(mgd->clipboard.main, first, count, 0, mgd)) {
                  mgd->hold = 1;
                  return 1;
                }
              }
            }
          }

          alert(ALERT_COMMAND_REJECTED);
          return 1;
        }

        {
          int append;

        case BRL_CMD_BLK(CLIP_NEW):
          append = 0;
          goto doClipBegin;

        case BRL_CMD_BLK(CLIP_ADD):
          append = 1;
          goto doClipBegin;

        doClipBegin:
          {
            const wchar_t *first = toMessageCharacter(arg1, 0, mgd);

            if (first) {
              beginClipboardCopy(first, append, mgd);
              mgd->hold = 1;
              return 1;
            }
          }

          alert(ALERT_COMMAND_REJECTED);
          return 1;
        }

        {
          int insertCR;

        case BRL_CMD_BLK(COPY_RECT):
          insertCR = 1;
          goto doClipEnd;

        case BRL_CMD_BLK(COPY_LINE):
          insertCR = 0;
          goto doClipEnd;

        doClipEnd:
          {
            const wchar_t *first = mgd->clipboard.start;

            if (first) {
              const wchar_t *last = toMessageCharacter(arg1, 1, mgd);

              if (last) {
                if (last > first) {
                  size_t count = last - first + 1;

                  if (endClipboardCopy(mgd->clipboard.main, first, count, insertCR, mgd)) {
                    mgd->hold = 1;
                    return 1;
                  }
                }
              }
            }
          }

          alert(ALERT_COMMAND_REJECTED);
          return 1;
        }

        default:
          mgd->hold = 0;
          break;
      }

      break;
    }
  }

  mgd->endWait = 1;
  return 1;
}

int
sayMessage (const char *text) {
#ifdef ENABLE_SPEECH_SUPPORT
  if (isAutospeakActive()) {
    if (!sayString(&spk, text, SAY_OPT_MUTE_FIRST)) {
      return 0;
    }
  }
#endif /* ENABLE_SPEECH_SUPPORT */

  return 1;
}

static const MessageSegment *
makeSegments (MessageSegment *segment, const wchar_t *characters, size_t characterCount, int wordWrap) {
  const size_t windowLength = textCount * brl.textRows;

  const wchar_t *character = characters;
  const wchar_t *const charactersEnd = character + characterCount;

  while (*character) {
    /* strip leading spaces */
    while ((character < charactersEnd) && iswspace(*character)) character += 1;

    const size_t charactersLeft = charactersEnd - character;
    if (!charactersLeft) break;
    segment->start = character;

    if (charactersLeft <= windowLength) {
      segment->length = charactersLeft;
    } else {
      segment->length = windowLength;

      if (wordWrap) {
        const wchar_t *segmentEnd = segment->start + segment->length;

        if (!iswspace(*segmentEnd)) {
          while (--segmentEnd >= segment->start) {
            if (iswspace(*segmentEnd)) break;
          }

          if (segmentEnd > segment->start) {
            segment->length = segmentEnd - segment->start + 1;
          }
        }
      }
    }

    character += segment->length;
    segment += 1;
  }

  return segment - 1;
}

static wchar_t *
makeBraille (const wchar_t *text, size_t textLength, size_t *brailleLength, int **offsetMap) {
  wchar_t *braille = NULL;

  *brailleLength = 0;
  *offsetMap = NULL;

  int cellsSize = MAX(0X10, textLength) * 2;
  int cellsLimit = textLength * 10;

  do {
    unsigned char cells[cellsSize];
    int toCellOffset[textLength + 1];

    int textCount = textLength;
    int cellCount = cellsSize;

    contractText(
      contractionTable, NULL,
      text, &textCount,
      cells, &cellCount,
      toCellOffset, CTB_NO_CURSOR
    );

    // this sholdn't happen
    if (textCount > textLength) break;

    if (textCount == textLength) {
      toCellOffset[textCount] = cellCount;

      int *toTextOffset;
      {
        size_t size = (cellCount + 1) * sizeof(*toTextOffset);

        if (!(toTextOffset = malloc(size))) {
          logMallocError();
          break;
        }
      }

      {
        int textOffset = 0;
        int cellOffset = 0;

        while (textOffset <= textCount) {
          int nextCellOffset = toCellOffset[textOffset];

          if (nextCellOffset != CTB_NO_OFFSET) {
            while (cellOffset < nextCellOffset) {
              toTextOffset[cellOffset++] = CTB_NO_OFFSET;
            }

            toTextOffset[cellOffset++] = textOffset;
          }

          textOffset += 1;
        }
      }

      if ((braille = allocateCharacters(cellCount + 1))) {
        const unsigned char *cell = cells;
        const unsigned char *end = cell + cellCount;
        wchar_t *character = braille;

        while (cell < end) {
          unsigned char dots = *cell++;
          *character++ = dots? (dots | UNICODE_BRAILLE_ROW): WC_C(' ');
        }

        *character = 0;
        *brailleLength = character - braille;
        *offsetMap = toTextOffset;
      } else {
        free(toTextOffset);
      }

      break;
    }
  } while ((cellsSize <<= 1) <= cellsLimit);

  return braille;
}

ASYNC_TASK_CALLBACK(presentMessage) {
  MessageParameters *mgp = data;

  if (!(mgp->options & MSG_SILENT)) {
    sayMessage(mgp->text);
  }

  if (canBraille()) {
    MessageData mgd = {
      .parameters = mgp,
      .hold = 0,
      .touch = 0,

      .clipboard = {
        .main = getMainClipboard(),
        .start = NULL,
        .offset = 0,
      },
    };

    const size_t textLength = countUtf8Characters(mgp->text);
    wchar_t text[textLength + 1];
    makeWcharsFromUtf8(mgp->text, text, ARRAY_COUNT(text));

    mgd.braille.message = text;
    mgd.braille.characters = text;
    mgd.braille.toMessageOffset = NULL;

    wchar_t *characters = text;
    size_t characterCount = textLength;
    int wordWrap = prefs.wordWrap;

    if (isContracting()) {
      size_t brailleLength;
      int *offsetMap;
      wchar_t *braille = makeBraille(text, textLength, &brailleLength, &offsetMap);

      if (braille) {
        characters = braille;
        characterCount = brailleLength;
        wordWrap = 1;

        mgd.braille.characters = braille;
        mgd.braille.toMessageOffset = offsetMap;
      }
    }

    MessageSegment messageSegments[characterCount];
    mgd.segments.current = mgd.segments.first = messageSegments;
    mgd.segments.last = makeSegments(messageSegments, characters, characterCount, wordWrap);
    mgd.clipboard.start = mgd.segments.first->start;

    int apiWasLinked = api.isServerLinked();
    if (apiWasLinked) api.unlinkServer();

    suspendUpdates();
    pushCommandEnvironment("message", NULL, NULL);
    pushCommandHandler("message", KTB_CTX_WAITING,
                       handleMessageCommands, NULL, &mgd);

    while (1) {
      const MessageSegment *segment = mgd.segments.current;
      int isLastSegment = segment == mgd.segments.last;
      brl.cursor = BRL_NO_CURSOR;

      if (!writeBrailleCharacters(mgp->mode, segment->start, segment->length)) {
        mgp->presented = 0;
        break;
      }

      mgd.timeout = messageHoldTimeout - brl.writeDelay;
      drainBrailleOutput(&brl, 0);
      if (!mgd.hold && isLastSegment && (mgp->options & MSG_NODELAY)) break;
      mgd.timeout = MAX(mgd.timeout, 0);

      while (1) {
        int timeout = mgd.timeout;
        mgd.timeout = -1;

        mgd.endWait = 0;
        int timedOut = !asyncAwaitCondition(timeout, testEndMessageWait, &mgd);
        if (mgd.segments.current != segment) break;

        if (mgd.hold || mgd.touch) {
          mgd.timeout = 1000000;
        } else if (timedOut) {
          if (isLastSegment) goto DONE;
          mgd.segments.current += 1;
          mgd.timeout = messageHoldTimeout;
          break;
        } else if (mgd.timeout < 0) {
          goto DONE;
        }
      }
    }

  DONE:
    if (characters != text) free(characters);
    if (mgd.braille.toMessageOffset) free(mgd.braille.toMessageOffset);

    popCommandEnvironment();
    resumeUpdates(1);
    if (apiWasLinked) api.linkServer();
  }

  if (mgp->deallocate) free(mgp);
}

int 
message (const char *mode, const char *text, MessageOptions options) {
  if (options & MSG_LOG) pushLogMessage(text);

  int presented = 0;
  MessageParameters *mgp;
  size_t size = sizeof(*mgp) + strlen(text) + 1;

  if ((mgp = malloc(size))) {
    memset(mgp, 0, size);
    mgp->mode = mode? mode: "";
    mgp->options = options;
    mgp->presented = 1;
    strcpy(mgp->text, text);

    if (mgp->options & MSG_SYNC) {
      mgp->deallocate = 0;
      presentMessage(mgp);
      if (mgp->presented) presented = 1;
    } else {
      mgp->deallocate = 1;
      if (asyncAddTask(NULL, presentMessage, mgp)) return 1;
    }

    free(mgp);
  }

  return presented;
}

void
showMessage (const char *text) {
  message(NULL, text, 0);
}

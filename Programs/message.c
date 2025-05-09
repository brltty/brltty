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
#include "brl_utils.h"
#include "brl_cmds.h"
#include "clipboard.h"
#include "spk.h"
#include "ktb_types.h"
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
  const wchar_t *characters;

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
    int append;
  } clipboard;
} MessageData;

ASYNC_CONDITION_TESTER(testEndMessageWait) {
  const MessageData *mgd = data;
  return mgd->endWait;
}

static const wchar_t *
toMessageCharacter (int arg, int relaxed, MessageData *mgd) {
  if (arg < 0) return NULL;
  int column = arg % brl.textColumns;
  int row = arg / brl.textColumns;
  if (row >= brl.textRows) return NULL;

  if ((column -= textStart) < 0) return NULL;
  if (column >= textCount) return NULL;

  const MessageSegment *segment = mgd->segments.current;
  size_t offset = (row * textCount) + column;

  if (offset >= segment->length) {
    if (!relaxed) return NULL;
    offset = segment->length - 1;
  }

  return segment->start + offset;
}

static int
copyToClipboard (ClipboardObject *clipboard, const wchar_t *characters, size_t count, int append) {
  lockMainClipboard();
  int copied;

  if (append) {
    copied = appendClipboardContent(clipboard, characters, count);
  } else {
    copied = setClipboardContent(clipboard, characters, count);
  }

  unlockMainClipboard();
  return copied;
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
          const wchar_t *first = toMessageCharacter(arg1, 0, mgd);

          if (first) {
            const wchar_t *last = toMessageCharacter(arg2, 1, mgd);

            if (last) {
              size_t count = last - first + 1;

              if (copyToClipboard(mgd->clipboard.main, first, count, append)) {
                alert(ALERT_CLIPBOARD_BEGIN);
                alert(ALERT_CLIPBOARD_END);

                mgd->hold = 1;
                return 1;
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
          const wchar_t *first = toMessageCharacter(arg1, 0, mgd);

          if (first) {
            alert(ALERT_CLIPBOARD_BEGIN);

            mgd->clipboard.start = first;
            mgd->clipboard.append = append;

            mgd->hold = 1;
            return 1;
          }

          alert(ALERT_COMMAND_REJECTED);
          return 1;
        }

        {
        case BRL_CMD_BLK(COPY_LINE):
        case BRL_CMD_BLK(COPY_RECT):
          const wchar_t *first = mgd->clipboard.start;
          int append = mgd->clipboard.append;

          if (first) {
            const wchar_t *last = toMessageCharacter(arg1, 1, mgd);

            if (last) {
              if (last > first) {
                size_t count = last - first + 1;

                if (copyToClipboard(mgd->clipboard.main, first, count, append)) {
                  alert(ALERT_CLIPBOARD_END);

                  mgd->hold = 1;
                  return 1;
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

ASYNC_TASK_CALLBACK(presentMessage) {
  MessageParameters *mgp = data;

#ifdef ENABLE_SPEECH_SUPPORT
  if (!(mgp->options & MSG_SILENT)) {
    if (isAutospeakActive()) {
      sayString(&spk, mgp->text, SAY_OPT_MUTE_FIRST);
    }
  }
#endif /* ENABLE_SPEECH_SUPPORT */

  if (canBraille()) {
    MessageData mgd = {
      .hold = 0,
      .touch = 0,

      .clipboard = {
        .main = getMainClipboard(),
        .start = 0,
        .append = 0,
      },

      .parameters = mgp
    };

    const size_t characterCount = countUtf8Characters(mgp->text);
    wchar_t characters[characterCount + 1];
    makeWcharsFromUtf8(mgp->text, characters, ARRAY_COUNT(characters));
    mgd.characters = characters;

    const size_t brailleSize = textCount * brl.textRows;
    wchar_t brailleBuffer[brailleSize];

    MessageSegment messageSegments[characterCount];

    {
      const wchar_t *character = characters;
      const wchar_t *const end = character + characterCount;

      MessageSegment *segment = messageSegments;
      mgd.segments.current = mgd.segments.first = segment;

      while (*character) {
        /* strip leading spaces */
        while ((character < end) && iswspace(*character)) character += 1;

        const size_t charactersLeft = end - character;
        if (!charactersLeft) break;

        segment->start = character;
        segment->length = MIN(charactersLeft, brailleSize);

        character += segment->length;
        segment += 1;
      }

      mgd.segments.last = segment - 1;
    }

    int apiWasLinked = api.isServerLinked();
    if (apiWasLinked) api.unlinkServer();

    suspendUpdates();
    pushCommandEnvironment("message", NULL, NULL);
    pushCommandHandler("message", KTB_CTX_WAITING,
                       handleMessageCommands, NULL, &mgd);

    while (1) {
      const MessageSegment *segment = mgd.segments.current;
      size_t cellCount = segment->length;
      int isLastSegment = segment == mgd.segments.last;

      wmemcpy(brailleBuffer, segment->start, cellCount);
      brl.cursor = BRL_NO_CURSOR;

      if (!writeBrailleCharacters(mgp->mode, brailleBuffer, cellCount)) {
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

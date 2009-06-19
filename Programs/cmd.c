/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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
#include <search.h>

#include "misc.h"
#include "brldefs.h"
#include "cmd.h"
#include "ttb.h"
#include "charset.h"
#include "queue.h"

const CommandEntry commandTable[] = {
  #include "cmds.auto.h"
  {.name=NULL, .code=EOF, .description=NULL}
};

static int
compareCommandCodes (const void *element1, const void *element2) {
  const CommandEntry *const *cmd1 = element1;
  const CommandEntry *const *cmd2 = element2;

  if ((*cmd1)->code < (*cmd2)->code) return -1;
  if ((*cmd1)->code > (*cmd2)->code) return 1;
  return 0;
}

const CommandEntry *
getCommandEntry (int code) {
  static const CommandEntry **commandEntries = NULL;
  static int commandCount;

  if (!commandEntries) {
    {
      const CommandEntry *cmd = commandTable;
      while (cmd->name) cmd += 1;
      commandCount = cmd - commandTable;
    }

    {
      const CommandEntry **entries = malloc(ARRAY_SIZE(entries, commandCount));

      if (!entries) {
        LogError("malloc");
        return NULL;
      }

      {
        const CommandEntry *cmd = commandTable;
        const CommandEntry **entry = entries;
        while (cmd->name) *entry++ = cmd++;
      }

      qsort(entries, commandCount, sizeof(*entries), compareCommandCodes);
      commandEntries = entries;
    }
  }

  code &= BRL_MSK_CMD;
  {
    int first = 0;
    int last = commandCount - 1;

    while (first <= last) {
      int current = (first + last) / 2;
      const CommandEntry *cmd = commandEntries[current];

      if (code < cmd->code) {
        last = current - 1;
      } else {
        first = current + 1;
      }
    }

    if (last >= 0) {
      const CommandEntry *cmd = commandEntries[last];
      int blk = cmd->code & BRL_MSK_BLK;
      int arg = cmd->code & BRL_MSK_ARG;

      if (blk == (code & BRL_MSK_BLK)) {
        if (arg == (code & BRL_MSK_ARG)) return cmd;

        if (blk) {
          return cmd;
          int next = last + 1;

          if (next < commandCount)
            if (blk != (commandEntries[next]->code & BRL_MSK_BLK))
              return cmd;
        }
      }
    }
  }

  return NULL;
}

void
describeCommand (int command, char *buffer, size_t size, int details) {
  int blk = command & BRL_MSK_BLK;
  int arg = command & BRL_MSK_ARG;
  const CommandEntry *cmd = getCommandEntry(command);

  if (!cmd) {
    snprintf(buffer, size, "unknown: %06X", command);
  } else {
    int length;

    if (details) {
      snprintf(buffer, size, "%s: %n", cmd->name, &length);
      buffer += length, size -= length;
    }

    snprintf(buffer, size, "%s%n", cmd->description, &length);
    if (cmd->isToggle && (command & BRL_FLG_TOGGLE_MASK)) {
      const char *oldVerb = "toggle";
      size_t oldLength = strlen(oldVerb);

      if (strncmp(buffer, oldVerb, oldLength) == 0) {
        const char *newVerb = "set";
        size_t newLength = strlen(newVerb);

        memmove(buffer+newLength, buffer+oldLength, length-oldLength+1);
        memcpy(buffer, newVerb, newLength);

        if (command & BRL_FLG_TOGGLE_ON) {
          char *end = strrchr(buffer, '/');
          if (end) *end = 0;
        } else {
          char *target = strrchr(buffer, ' ');

          if (target) {
            const char *source = strchr(++target, '/');

            if (source) memmove(target, source+1, strlen(source));
          }
        }

        length = strlen(buffer);
      }
    }
    buffer += length, size -= length;

    if (details) {
      if (cmd->isCharacter || cmd->isBase) {
        snprintf(buffer, size, " #%u",
                 arg - (cmd->code & BRL_MSK_ARG) + 1);
      } else if (blk) {
        switch (blk) {
          case BRL_BLK_PASSKEY:
            break;

          case BRL_BLK_PASSDOTS: {
            static const unsigned char dots[] = {BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4, BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8};
            int dot;
            unsigned int number = 0;

            for (dot=0; dot<sizeof(dots); dot+=1) {
              if (arg & dots[dot]) {
                number = (number * 10) + (dot + 1);
              }
            }

            snprintf(buffer, size, " %u", number);
            break;
          }

          default:
            snprintf(buffer, size, " 0X%02X", arg);
            break;
        }
      }
    }
  }
}

typedef struct {
  int command;
} CommandQueueItem;

static Queue *
getCommandQueue (int create) {
  static Queue *commandQueue = NULL;

  if (create && !commandQueue) {
    commandQueue = newQueue(NULL, NULL);
  }

  return commandQueue;
}

int
enqueueCommand (int command) {
  if (command != EOF) {
    Queue *queue = getCommandQueue(1);

    if (queue) {
      CommandQueueItem *item = malloc(sizeof(CommandQueueItem));

      if (item) {
        item->command = command;
        if (enqueueItem(queue, item)) return 1;

        free(item);
      }
    }
  }

  return 0;
}

int
dequeueCommand (void) {
  int command = EOF;
  Queue *queue = getCommandQueue(0);

  if (queue) {
    CommandQueueItem *item = dequeueItem(queue);

    if (item) {
      command = item->command;
      free(item);
    }
  }

  return command;
}

void
resetRepeatState (RepeatState *state) {
  state->command = EOF;
  state->timeout = 0;
  state->started = 0;
}

void
handleRepeatFlags (int *command, RepeatState *state, int panning, int delay, int interval) {
  if (state) {
    if (*command == EOF) {
      if (state->timeout) {
        struct timeval now;
        gettimeofday(&now, NULL);
        if (millisecondsBetween(&state->time, &now) >= state->timeout) {
          *command = state->command;
          state->time = now;
          state->timeout = interval;
          state->started = 1;
        }
      }
    } else {
      int flags = *command & BRL_FLG_REPEAT_MASK;
      *command &= ~BRL_FLG_REPEAT_MASK;

      switch (*command & BRL_MSK_BLK) {
        default:
          switch (*command & BRL_MSK_CMD) {
            case BRL_CMD_FWINLT:
            case BRL_CMD_FWINRT:
              if (panning) break;

            default:
              if (BRL_DELAYED_COMMAND(flags)) *command = BRL_CMD_NOOP;
              flags = 0;

            case BRL_CMD_LNUP:
            case BRL_CMD_LNDN:
            case BRL_CMD_PRDIFLN:
            case BRL_CMD_NXDIFLN:
            case BRL_CMD_CHRLT:
            case BRL_CMD_CHRRT:

            case BRL_CMD_MENU_PREV_ITEM:
            case BRL_CMD_MENU_NEXT_ITEM:
            case BRL_CMD_MENU_PREV_SETTING:
            case BRL_CMD_MENU_NEXT_SETTING:

            case BRL_BLK_PASSKEY + BRL_KEY_BACKSPACE:
            case BRL_BLK_PASSKEY + BRL_KEY_DELETE:
            case BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP:
            case BRL_BLK_PASSKEY + BRL_KEY_PAGE_DOWN:
            case BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP:
            case BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN:
            case BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT:
            case BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT:
              break;
          }

        case BRL_BLK_PASSCHAR:
        case BRL_BLK_PASSDOTS:
          break;
      }

      if (state->started) {
        state->started = 0;

        if (*command == state->command) {
          *command = BRL_CMD_NOOP;
          flags = 0;
        }
      }
      state->command = *command;

      if (flags & BRL_FLG_REPEAT_DELAY) {
        gettimeofday(&state->time, NULL);
        state->timeout = delay;
        if (flags & BRL_FLG_REPEAT_INITIAL) {
          state->started = 1;
        } else {
          *command = BRL_CMD_NOOP;
        }
      } else if (flags & BRL_FLG_REPEAT_INITIAL) {
        gettimeofday(&state->time, NULL);
        state->timeout = interval;
        state->started = 1;
      } else {
        state->timeout = 0;
      }     
    }
  } else if (*command != EOF) {
    if (BRL_DELAYED_COMMAND(*command)) {
      *command = BRL_CMD_NOOP;
    } else {
      *command &= ~BRL_FLG_REPEAT_MASK;
    }
  }
}

static brlapi_keyCode_t
cmdWCharToBrlapi (wchar_t wc) {
  if (iswLatin1(wc)) {
    /* latin1 character */
    return BRLAPI_KEY_TYPE_SYM | wc;
  } else {
    /* unicode character */
    return BRLAPI_KEY_TYPE_SYM | BRLAPI_KEY_SYM_UNICODE | wc;
  }
}

brlapi_keyCode_t
cmdBrlttyToBrlapi (int command, int retainDots) {
  brlapi_keyCode_t code;
  int arg = BRL_ARG_GET(command);
  int blk = command & BRL_MSK_BLK;
  switch (blk) {
  case BRL_BLK_PASSCHAR:
    code = cmdWCharToBrlapi(convertCharToWchar(arg));
    break;
  case BRL_BLK_PASSDOTS:
    if (retainDots) {
      if (arg == (BRLAPI_DOTC >> BRLAPI_KEY_CMD_ARG_SHIFT)) arg = 0;
      goto doDefault;
    }
    code = cmdWCharToBrlapi(convertDotsToCharacter(textTable, arg));
    break;
  case BRL_BLK_PASSKEY:
    switch (arg) {
    case BRL_KEY_ENTER:		code = BRLAPI_KEY_SYM_LINEFEED;	 break;
    case BRL_KEY_TAB:		code = BRLAPI_KEY_SYM_TAB;	 break;
    case BRL_KEY_BACKSPACE:	code = BRLAPI_KEY_SYM_BACKSPACE; break;
    case BRL_KEY_ESCAPE:	code = BRLAPI_KEY_SYM_ESCAPE;	 break;
    case BRL_KEY_CURSOR_LEFT:	code = BRLAPI_KEY_SYM_LEFT;	 break;
    case BRL_KEY_CURSOR_RIGHT:	code = BRLAPI_KEY_SYM_RIGHT;	 break;
    case BRL_KEY_CURSOR_UP:	code = BRLAPI_KEY_SYM_UP;	 break;
    case BRL_KEY_CURSOR_DOWN:	code = BRLAPI_KEY_SYM_DOWN;	 break;
    case BRL_KEY_PAGE_UP:	code = BRLAPI_KEY_SYM_PAGE_UP;	 break;
    case BRL_KEY_PAGE_DOWN:	code = BRLAPI_KEY_SYM_PAGE_DOWN; break;
    case BRL_KEY_HOME:		code = BRLAPI_KEY_SYM_HOME;	 break;
    case BRL_KEY_END:		code = BRLAPI_KEY_SYM_END;	 break;
    case BRL_KEY_INSERT:	code = BRLAPI_KEY_SYM_INSERT;	 break;
    case BRL_KEY_DELETE:	code = BRLAPI_KEY_SYM_DELETE;	 break;
    default: code = BRLAPI_KEY_SYM_FUNCTION + arg - BRL_KEY_FUNCTION; break;
    }
    break;
  default:
  doDefault:
    code = BRLAPI_KEY_TYPE_CMD
         | (blk >> BRL_SHIFT_BLK << BRLAPI_KEY_CMD_BLK_SHIFT)
         | (arg                  << BRLAPI_KEY_CMD_ARG_SHIFT)
         ;
    break;
  }
  if (blk == BRL_BLK_GOTOLINE)
    code = code
    | (command & BRL_FLG_LINE_SCALED	? BRLAPI_KEY_FLG_LINE_SCALED	: 0)
    | (command & BRL_FLG_LINE_TOLEFT	? BRLAPI_KEY_FLG_LINE_TOLEFT	: 0)
      ;
  if (blk == BRL_BLK_PASSCHAR
   || blk == BRL_BLK_PASSKEY
   || blk == BRL_BLK_PASSDOTS)
    code = code
    | (command & BRL_FLG_CHAR_CONTROL	? BRLAPI_KEY_FLG_CONTROL	: 0)
    | (command & BRL_FLG_CHAR_META	? BRLAPI_KEY_FLG_META		: 0)
    | (command & BRL_FLG_CHAR_UPPER	? BRLAPI_KEY_FLG_UPPER		: 0)
    | (command & BRL_FLG_CHAR_SHIFT	? BRLAPI_KEY_FLG_SHIFT		: 0)
      ;
  else
    code = code
    | (command & BRL_FLG_TOGGLE_ON	? BRLAPI_KEY_FLG_TOGGLE_ON	: 0)
    | (command & BRL_FLG_TOGGLE_OFF	? BRLAPI_KEY_FLG_TOGGLE_OFF	: 0)
    | (command & BRL_FLG_ROUTE		? BRLAPI_KEY_FLG_ROUTE		: 0)
      ;
  return code
  | (command & BRL_FLG_REPEAT_INITIAL	? BRLAPI_KEY_FLG_REPEAT_INITIAL	: 0)
  | (command & BRL_FLG_REPEAT_DELAY	? BRLAPI_KEY_FLG_REPEAT_DELAY	: 0)
    ;
}

int
cmdBrlapiToBrltty (brlapi_keyCode_t code) {
  int cmd;
  switch (code & BRLAPI_KEY_TYPE_MASK) {
  case BRLAPI_KEY_TYPE_CMD:
    cmd = BRL_BLK((code&BRLAPI_KEY_CMD_BLK_MASK)>>BRLAPI_KEY_CMD_BLK_SHIFT)
	| BRL_ARG_SET((code&BRLAPI_KEY_CMD_ARG_MASK)>>BRLAPI_KEY_CMD_ARG_SHIFT);
    break;
  case BRLAPI_KEY_TYPE_SYM: {
      unsigned long keysym;
      keysym = code & BRLAPI_KEY_CODE_MASK;
      switch (keysym) {
      case BRLAPI_KEY_SYM_BACKSPACE:	cmd = BRL_BLK_PASSKEY|BRL_KEY_BACKSPACE;	break;
      case BRLAPI_KEY_SYM_TAB:		cmd = BRL_BLK_PASSKEY|BRL_KEY_TAB;	break;
      case BRLAPI_KEY_SYM_LINEFEED:	cmd = BRL_BLK_PASSKEY|BRL_KEY_ENTER;	break;
      case BRLAPI_KEY_SYM_ESCAPE:	cmd = BRL_BLK_PASSKEY|BRL_KEY_ESCAPE;	break;
      case BRLAPI_KEY_SYM_HOME:		cmd = BRL_BLK_PASSKEY|BRL_KEY_HOME;	break;
      case BRLAPI_KEY_SYM_LEFT:		cmd = BRL_BLK_PASSKEY|BRL_KEY_CURSOR_LEFT;	break;
      case BRLAPI_KEY_SYM_UP:		cmd = BRL_BLK_PASSKEY|BRL_KEY_CURSOR_UP;	break;
      case BRLAPI_KEY_SYM_RIGHT:	cmd = BRL_BLK_PASSKEY|BRL_KEY_CURSOR_RIGHT;	break;
      case BRLAPI_KEY_SYM_DOWN:		cmd = BRL_BLK_PASSKEY|BRL_KEY_CURSOR_DOWN;	break;
      case BRLAPI_KEY_SYM_PAGE_UP:	cmd = BRL_BLK_PASSKEY|BRL_KEY_PAGE_UP;	break;
      case BRLAPI_KEY_SYM_PAGE_DOWN:	cmd = BRL_BLK_PASSKEY|BRL_KEY_PAGE_DOWN;	break;
      case BRLAPI_KEY_SYM_END:		cmd = BRL_BLK_PASSKEY|BRL_KEY_END;	break;
      case BRLAPI_KEY_SYM_INSERT:	cmd = BRL_BLK_PASSKEY|BRL_KEY_INSERT;	break;
      case BRLAPI_KEY_SYM_DELETE:	cmd = BRL_BLK_PASSKEY|BRL_KEY_DELETE;	break;
      default:
	if (keysym >= BRLAPI_KEY_SYM_FUNCTION && keysym <= BRLAPI_KEY_SYM_FUNCTION + 34)
	  cmd = BRL_BLK_PASSKEY | (BRL_KEY_FUNCTION + keysym - BRLAPI_KEY_SYM_FUNCTION);
	else if (keysym < 0x100 || (keysym & 0x1F000000) == BRLAPI_KEY_SYM_UNICODE) {
	  int c = convertWcharToChar(keysym & 0xFFFFFF);
	  if (c == EOF)
	    /* Not convertible to current 8bit charset */
	    return EOF;
	  else
	    cmd = BRL_BLK_PASSCHAR | c;
	} else return EOF;
	break;
      }
      break;
    }
  default:
    return EOF;
  }
  return cmd
  | (code & BRLAPI_KEY_FLG_TOGGLE_ON		? BRL_FLG_TOGGLE_ON	: 0)
  | (code & BRLAPI_KEY_FLG_TOGGLE_OFF		? BRL_FLG_TOGGLE_OFF	: 0)
  | (code & BRLAPI_KEY_FLG_ROUTE		? BRL_FLG_ROUTE		: 0)
  | (code & BRLAPI_KEY_FLG_REPEAT_INITIAL	? BRL_FLG_REPEAT_INITIAL: 0)
  | (code & BRLAPI_KEY_FLG_REPEAT_DELAY		? BRL_FLG_REPEAT_DELAY	: 0)
  | (code & BRLAPI_KEY_FLG_LINE_SCALED		? BRL_FLG_LINE_SCALED	: 0)
  | (code & BRLAPI_KEY_FLG_LINE_TOLEFT		? BRL_FLG_LINE_TOLEFT	: 0)
  | (code & BRLAPI_KEY_FLG_CONTROL		? BRL_FLG_CHAR_CONTROL	: 0)
  | (code & BRLAPI_KEY_FLG_META			? BRL_FLG_CHAR_META	: 0)
  | (code & BRLAPI_KEY_FLG_UPPER		? BRL_FLG_CHAR_UPPER	: 0)
  | (code & BRLAPI_KEY_FLG_SHIFT		? BRL_FLG_CHAR_SHIFT	: 0)
    ;
}

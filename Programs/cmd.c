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
#ifdef ENABLE_LEARN_MODE
  #include "cmds.auto.h"
#endif /* ENABLE_LEARN_MODE */
  {EOF, NULL, NULL}
};

static int
isTypedCommand (int code, const int *codes) {
  while (*codes != EOF) {
    if (code == *codes) return 1;
    codes += 1;
  }

  return 0;
}

int
isBaseCommand (const CommandEntry *command) {
  static const int codes[] = {
    BRL_BLK_SWITCHVT,
    BRL_BLK_SETMARK, BRL_BLK_GOTOMARK,
    BRL_BLK_GOTOLINE,
    BRL_BLK_PASSKEY + BRL_KEY_FUNCTION,
    EOF
  };
  return isTypedCommand(command->code, codes);
}

int
isCharacterCommand (const CommandEntry *command) {
  static const int codes[] = {
    BRL_BLK_ROUTE,
    BRL_BLK_CUTBEGIN, BRL_BLK_CUTAPPEND,
    BRL_BLK_CUTRECT, BRL_BLK_CUTLINE,
    BRL_BLK_PRINDENT, BRL_BLK_NXINDENT,
    BRL_BLK_DESCCHAR, BRL_BLK_SETLEFT,
    BRL_BLK_PRDIFCHAR, BRL_BLK_NXDIFCHAR,
    EOF
  };
  return isTypedCommand(command->code, codes);
}

int
isToggleCommand (const CommandEntry *command) {
  const char *prefix = "toggle ";
  return strncasecmp(command->description, prefix, strlen(prefix)) == 0;
}

void
describeCommand (int command, char *buffer, int size) {
  int blk = command & BRL_MSK_BLK;
  int arg = command & BRL_MSK_ARG;
  int cmd = blk | arg;
  const CommandEntry *candidate = NULL;
  const CommandEntry *last = NULL;

  {
    const CommandEntry *current = commandTable;
    while (current->name) {
      if ((current->code & BRL_MSK_BLK) == blk) {
        if (!last || (last->code < current->code)) last = current;
        if (!candidate || (candidate->code < cmd)) candidate = current;
      }

      ++current;
    }
  }
  if (candidate)
    if (candidate->code != cmd)
      if ((blk == 0) || (candidate->code < last->code))
        candidate = NULL;

  if (!candidate) {
    snprintf(buffer, size, "unknown: %06X", command);
  } else if ((candidate == last) && (blk != 0)) {
    unsigned int number;
    switch (blk) {
      default:
        number = cmd - candidate->code + 1;
        break;

      case BRL_BLK_PASSCHAR:
        number = cmd - candidate->code;
        break;

      case BRL_BLK_PASSDOTS: {
        static const unsigned char dots[] = {BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4, BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8};
        int dot;
        number = 0;
        for (dot=0; dot<sizeof(dots); ++dot) {
          if (arg & dots[dot]) {
            number = (number * 10) + (dot + 1);
          }
        }
        break;
      }
    }
    snprintf(buffer, size, "%s[%d]: %s",
             candidate->name, number, candidate->description);
  } else {
    int offset;
    snprintf(buffer, size, "%s: %n%s",
             candidate->name, &offset, candidate->description);

    if ((blk == 0) && (command & BRL_FLG_TOGGLE_MASK)) {
      char *description = buffer + offset;
      const char *oldVerb = "toggle";
      int oldLength = strlen(oldVerb);
      if (strncmp(description, oldVerb, oldLength) == 0) {
        const char *newVerb = "set";
        int newLength = strlen(newVerb);
        memmove(description+newLength, description+oldLength, strlen(description+oldLength)+1);
        memcpy(description, newVerb, newLength);
        if (command & BRL_FLG_TOGGLE_ON) {
          char *end = strrchr(description, '/');
          if (end) *end = 0;
        } else {
          char *target = strrchr(description, ' ');
          if (target) {
            const char *source = strchr(++target, '/');
            if (source) {
              memmove(target, source+1, strlen(source));
            }
          }
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
              if (IS_DELAYED_COMMAND(flags)) *command = BRL_CMD_NOOP;
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
    if (IS_DELAYED_COMMAND(*command)) {
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
  switch (command & BRL_MSK_BLK) {
  case BRL_BLK_PASSCHAR:
    code = cmdWCharToBrlapi(convertCharToWchar(command & BRL_MSK_ARG));
    break;
  case BRL_BLK_PASSDOTS:
    if (retainDots) goto doDefault;
    code = cmdWCharToBrlapi(convertDotsToCharacter(textTable, command & BRL_MSK_ARG));
    break;
  case BRL_BLK_PASSKEY:
    switch (command & BRL_MSK_ARG) {
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
    default: code = BRLAPI_KEY_SYM_FUNCTION + (command & BRL_MSK_ARG) - BRL_KEY_FUNCTION; break;
    }
    break;
  default:
  doDefault:
    code = BRLAPI_KEY_TYPE_CMD
         | (BRL_CODE_GET(BLK, command) << BRLAPI_KEY_CMD_BLK_SHIFT)
         | (BRL_ARG_GET(command)       << BRLAPI_KEY_CMD_ARG_SHIFT)
         ;
    break;
  }
  if ((command & BRL_MSK_BLK) == BRL_BLK_GOTOLINE)
    code = code
    | (command & BRL_FLG_LINE_SCALED	? BRLAPI_KEY_FLG_LINE_SCALED	: 0)
    | (command & BRL_FLG_LINE_TOLEFT	? BRLAPI_KEY_FLG_LINE_TOLEFT	: 0)
      ;
  if ((command & BRL_MSK_BLK) == BRL_BLK_PASSCHAR
   || (command & BRL_MSK_BLK) == BRL_BLK_PASSKEY
   || (command & BRL_MSK_BLK) == BRL_BLK_PASSDOTS)
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

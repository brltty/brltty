/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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
#include "timing.h"
#include "brldefs.h"
#include "cmd.h"
#include "ttb.h"
#include "charset.h"
#include "queue.h"

const CommandEntry commandTable[] = {
  #include "cmds.auto.h"
  {.name=NULL, .code=EOF, .description=NULL}
};

const CommandModifierEntry commandModifierTable_braille[] = {
  {.name="dot1" , .bit=BRL_DOT1},
  {.name="dot2" , .bit=BRL_DOT2},
  {.name="dot3" , .bit=BRL_DOT3},
  {.name="dot4" , .bit=BRL_DOT4},
  {.name="dot5" , .bit=BRL_DOT5},
  {.name="dot6" , .bit=BRL_DOT6},
  {.name="dot7" , .bit=BRL_DOT7},
  {.name="dot8" , .bit=BRL_DOT8},
  {.name="space", .bit=BRL_DOTC},
  {.name=NULL   , .bit=0       }
};

const CommandModifierEntry commandModifierTable_character[] = {
  {.name="upper", .bit=BRL_FLG_CHAR_UPPER},
  {.name=NULL   , .bit=0                 }
};

const CommandModifierEntry commandModifierTable_input[] = {
  {.name="shift"  , .bit=BRL_FLG_CHAR_SHIFT  },
  {.name="control", .bit=BRL_FLG_CHAR_CONTROL},
  {.name="meta"   , .bit=BRL_FLG_CHAR_META   },
  {.name=NULL     , .bit=0                   }
};

const CommandModifierEntry commandModifierTable_keyboard[] = {
  {.name="release", .bit=BRL_FLG_KBD_RELEASE},
  {.name="emul0"  , .bit=BRL_FLG_KBD_EMUL0  },
  {.name="emul1"  , .bit=BRL_FLG_KBD_EMUL1  },
  {.name=NULL     , .bit=0                  }
};

const CommandModifierEntry commandModifierTable_line[] = {
  {.name="scaled", .bit=BRL_FLG_LINE_SCALED},
  {.name="toleft", .bit=BRL_FLG_LINE_TOLEFT},
  {.name=NULL    , .bit=0                  }
};

const CommandModifierEntry commandModifierTable_motion[] = {
  {.name="route", .bit=BRL_FLG_MOTION_ROUTE},
  {.name=NULL   , .bit=0                   }
};

const CommandModifierEntry commandModifierTable_toggle[] = {
  {.name="on" , .bit=BRL_FLG_TOGGLE_ON },
  {.name="off", .bit=BRL_FLG_TOGGLE_OFF},
  {.name=NULL , .bit=0                 }
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
        logMallocError();
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

static size_t
formatCommandModifiers (char *buffer, size_t size, int command, const CommandModifierEntry *modifiers) {
  const CommandModifierEntry *modifier = modifiers;
  size_t length;
  STR_BEGIN(buffer, size);

  while (modifier->name) {
    if (command & modifier->bit) {
      STR_PRINTF(" + %s", modifier->name);
    }

    modifier += 1;
  }

  length = STR_LENGTH;
  STR_END;
  return length;
}

size_t
describeCommand (int command, char *buffer, size_t size, CommandDescriptionOption options) {
  size_t length;
  STR_BEGIN(buffer, size);

  unsigned int arg = BRL_ARG_GET(command);
  unsigned int arg1 = BRL_CODE_GET(ARG, command);
  unsigned int arg2 = BRL_CODE_GET(EXT, command);
  const CommandEntry *cmd = getCommandEntry(command);

  if (!cmd) {
    STR_PRINTF("%s: %06X", gettext("unknown command"), command);
  } else {
    if (options & CDO_IncludeName) {
      STR_PRINTF("%s: ", cmd->name);
    }

    if (cmd->isToggle && (command & BRL_FLG_TOGGLE_MASK)) {
      const char *text = gettext(cmd->description);
      size_t length = strlen(text);
      char buffer[length + 1];
      char *delimiter;

      strcpy(buffer, text);
      delimiter = strchr(buffer, '/');

      if (delimiter) {
        char *source;
        char *target;

        if (command & BRL_FLG_TOGGLE_ON) {
          target = delimiter;
          if (!(source = strchr(target, ' '))) source = target + strlen(target);
        } else if (command & BRL_FLG_TOGGLE_OFF) {
          {
            char oldDelimiter = *delimiter;
            *delimiter = 0;
            target = strrchr(buffer, ' ');
            *delimiter = oldDelimiter;
          }

          target = target? (target + 1): buffer;
          source = delimiter + 1;
        } else {
          goto toggleReady;
        }

        memmove(target, source, (strlen(source) + 1));
      }

    toggleReady:
      STR_PRINTF("%s", buffer);
    } else {
      STR_PRINTF("%s", gettext(cmd->description));
    }

    if (options & CDO_IncludeOperand) {
      if (cmd->isCharacter) {
        STR_PRINTF(" [U+%04" PRIX16 "]", arg);
      }

      if (cmd->isBraille) {
        int none = 1;
        STR_PRINTF(" [");

        {
          static const int dots[] = {
            BRL_DOTC,
            BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4,
            BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8,
            0
          };
          const int *dot = dots;

          while (*dot) {
            if (command & *dot) {
              none = 0;

              if (dot == dots) {
                STR_PRINTF("C");
              } else {
                unsigned int number = dot - dots;
                STR_PRINTF("%u", number);
              }
            }

            dot += 1;
          }
        }

        if (none) STR_PRINTF("%s", gettext("space"));
        STR_PRINTF("]");
      }

      if (cmd->isKeyboard) {
        STR_PRINTF(" [\\X%02" PRIX8 "]", arg1);
      }

      if (cmd->isColumn && !cmd->isRouting && (
           (arg == BRL_MSK_ARG) /* key event processing */
         ||
           ((options & CDO_DefaultOperand) && !arg) /* key table listing */
         )) {
        STR_PRINTF(" %s", gettext("at cursor"));
      } else if (cmd->isColumn || cmd->isRow || cmd->isOffset) {
        STR_PRINTF(" #%u", arg - (cmd->code & BRL_MSK_ARG) + 1);
      } else if (cmd->isRange) {
        STR_PRINTF(" #%u-%u", arg1, arg2);
      }

      if (cmd->isInput) {
        size_t length = formatCommandModifiers(STR_NEXT, STR_LEFT, command, commandModifierTable_input);
        STR_ADJUST(length);
      }

      if (cmd->isCharacter) {
        size_t length = formatCommandModifiers(STR_NEXT, STR_LEFT, command, commandModifierTable_character);
        STR_ADJUST(length);
      }

      if (cmd->isKeyboard) {
        size_t length = formatCommandModifiers(STR_NEXT, STR_LEFT, command, commandModifierTable_keyboard);
        STR_ADJUST(length);
      }
    }

    if (cmd->isMotion) {
      if (cmd->isRow) {
        if (command & BRL_FLG_LINE_TOLEFT) {
          STR_PRINTF(", %s", gettext("left margin"));
        }

        if (command & BRL_FLG_LINE_SCALED) {
          STR_PRINTF(", %s", gettext("normalized position"));
        }
      }

      if (command & BRL_FLG_MOTION_ROUTE) {
        STR_PRINTF(", %s", gettext("drag cursor"));
      }
    }
  }

  length = STR_LENGTH;
  STR_END;
  return length;
}

static size_t
formatCommand (char *buffer, size_t size, int command) {
  size_t length;
  STR_BEGIN(buffer, size);

  STR_PRINTF("%06X (", command);

  {
    size_t length = describeCommand(command, STR_NEXT, STR_LEFT, 
                                    CDO_IncludeName | CDO_IncludeOperand);
    STR_ADJUST(length);
  }

  STR_PRINTF(")");

  length = STR_LENGTH;
  STR_END;
  return length;
}

typedef struct {
  int command;
} LogCommandData;

static size_t
formatLogCommandData (char *buffer, size_t size, const void *data) {
  const LogCommandData *cmd = data;
  size_t length;

  STR_BEGIN(buffer, size);
  STR_PRINTF("command: ");

  {
    size_t sublength = formatCommand(STR_NEXT, STR_LEFT, cmd->command);
    STR_ADJUST(sublength);
  }

  length = STR_LENGTH;
  STR_END
  return length;
}

void
logCommand (int command) {
  const LogCommandData cmd = {
    .command = command
  };

  logData(LOG_DEBUG, formatLogCommandData, &cmd);
}

typedef struct {
  int oldCommand;
  int newCommand;
} LogTransformedCommandData;

static size_t
formatLogTransformedCommandData (char *buffer, size_t size, const void *data) {
  const LogTransformedCommandData *cmd = data;
  size_t length;

  STR_BEGIN(buffer, size);
  STR_PRINTF("command: ");

  {
    size_t sublength = formatCommand(STR_NEXT, STR_LEFT, cmd->oldCommand);
    STR_ADJUST(sublength);
  }

  STR_PRINTF(" -> ");

  {
    size_t sublength = formatCommand(STR_NEXT, STR_LEFT, cmd->newCommand);
    STR_ADJUST(sublength);
  }

  length = STR_LENGTH;
  STR_END
  return length;
}

void
logTransformedCommand (int oldCommand, int newCommand) {
  const LogTransformedCommandData cmd = {
    .oldCommand = oldCommand,
    .newCommand = newCommand
  };

  logData(LOG_DEBUG, formatLogTransformedCommandData, &cmd);
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
        TimeValue now;
        getMonotonicTime(&now);
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

            case BRL_CMD_SPEAK_PREV_CHAR:
            case BRL_CMD_SPEAK_NEXT_CHAR:
            case BRL_CMD_SPEAK_PREV_WORD:
            case BRL_CMD_SPEAK_NEXT_WORD:
            case BRL_CMD_SPEAK_PREV_LINE:
            case BRL_CMD_SPEAK_NEXT_LINE:
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
        getMonotonicTime(&state->time);
        state->timeout = delay;
        if (flags & BRL_FLG_REPEAT_INITIAL) {
          state->started = 1;
        } else {
          *command = BRL_CMD_NOOP;
        }
      } else if (flags & BRL_FLG_REPEAT_INITIAL) {
        getMonotonicTime(&state->time);
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

#ifdef ENABLE_API
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
    code = cmdWCharToBrlapi(arg);
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
    | (command & BRL_FLG_MOTION_ROUTE	? BRLAPI_KEY_FLG_MOTION_ROUTE	: 0)
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
	  wchar_t c = keysym & 0xFFFFFF;
	  cmd = BRL_BLK_PASSCHAR | BRL_ARG_SET(c);
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
  | (code & BRLAPI_KEY_FLG_MOTION_ROUTE		? BRL_FLG_MOTION_ROUTE	: 0)
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
#endif /* ENABLE_API */

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
#include "brl_cmds.h"
#include "cmd.h"
#include "program.h"

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

    registerProgramMemory("sorted-command-table", &commandEntries);
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
        STR_PRINTF(" [U+%04X]", arg);
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
        STR_PRINTF(" [\\X%02X]", arg1);
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

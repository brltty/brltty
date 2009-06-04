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

#include "misc.h"
#include "cmd.h"
#include "config.tab.c"

static char* filename = "stdin";

static int
yyerror (char *problem)  /* Called by yyparse on error */
{
  fprintf(stderr, "%s[%d]: %s\n", filename, lineNumber, problem);
  return 1;
}

/* --------------------------------------------------------------- */

static const char* get_command_description(int code)
{
  int i;
  for(i=0;commandTable[i].name; i++)
    if (commandTable[i].code == code)
	return commandTable[i].description;
  return "";
}

static char* search_help_text(int token)
{
  int i;
  for(i=0;hlptxt[i].txt; i++)
    if (hlptxt[i].cmd == token)
	return hlptxt[i].txt;
  return "";
}

static char* search_symbol(int token)
{
  int i;
  for(i=0;symbols[i].sname; i++)
    if (symbols[i].token == token)
	return symbols[i].sname;
  return "????";
}

static char* search_code(int code, int cmd)
{
  int i;
  for(i=0;symbols[i].sname; i++)
    if (symbols[i].token == code &&
	symbols[i].val == cmd)
	return symbols[i].sname;
  return "???";
}

static char* search_cmd(int cmd)
{
  if ((cmd & BRL_MSK_BLK) == BRL_BLK_PASSKEY) {
    return search_code(VPK, cmd);
  } else {
    return search_code(KEYCODE, cmd);
  }
}

static char* search_toggle(int cmd)
{
  switch (cmd & BRL_FLG_TOGGLE_MASK)
  {
    case BRL_FLG_TOGGLE_ON:
      return search_code(ON, 0);

    case BRL_FLG_TOGGLE_OFF:
      return search_code(OFF, 0);

    case BRL_FLG_TOGGLE_ON | BRL_FLG_TOGGLE_OFF:
      return search_code(CHECK, 0);
  }
  return "";
}

static char* search_key(int16_t key) {
  static char res[20];

  if (key == KEY_ROUTING1) {
    snprintf(res, sizeof(res), "%s",
             search_code(ROUTING1, 0));
    return res;
  }

  if (key == KEY_ROUTING2) {
    snprintf(res, sizeof(res), "%s",
             search_code(ROUTING2, 0));
    return res;
  }

  if (key == KEY_STATUS2) {
    snprintf(res, sizeof(res), "%s",
             search_code(STATUS2, 0));
    return res;
  }

  if (key >= KEYS_SWITCH) {
    static const unsigned char map[] = {0, 2, 3, 1};
    key -= KEYS_SWITCH + 1;
    key = (map[key >> 1] << 1) | (key & 0X1);
    snprintf(res, sizeof(res), "%s %s %s",
             (key & 0X04)? "key": "switch",
             (key & 0X02)? "right": "left",
             (key & 0X01)? "front": "rear");
    return res;
  }

  if (key >= KEYS_BAR) {
    snprintf(res, sizeof(res), "%s %s",
             search_symbol(BAR),
             search_code(BARCODE, key-KEYS_BAR));
    return res;
  }

  if (key >=  KEYS_STATUS) {
    snprintf(res, sizeof(res), "status %d", key-KEYS_STATUS);
    return res;
  }

  if (key >=  KEYS_FRONT) {
    snprintf(res, sizeof(res), "front %d", key-KEYS_FRONT);
    return res;
  }

  snprintf(res, sizeof(res), "unknown");
  return res;
}

/* --------------------------------------------------------------- */
/* Param help is true, when procedure generates one helpfile for
   each terminaltype. Otherwise the procedure generates one
   big parameterfile via stdout                                    */

static void
printkeys (FILE *fh, const TerminalDefinition *terminal, const CommandDefinition *cmd) {
  const char *delimiter = "";
  int m;

  for (m=0; m<terminal->modifierCount; m++) {
    if (cmd->modifiers & (1 << m)) {
      fprintf(fh, "%s%s", delimiter, search_key(terminal->modifierKeys[m])); 
      delimiter = " and ";
    }
  }

  if (cmd->key != KEY_NONE) {
    fprintf(fh, "%s%s", delimiter, search_key(cmd->key));
  }
}

static void terminals(int help, int verbose)
{
  int t;
  FILE *fh = stdout;

  /* TODO: Comment/Help for status display */

  for (t=0; t<pmTerminalCount; t++) {
    const TerminalDefinition *terminal = &pmTerminals[t];

    if (terminal->modelName) {
      if (help) {
        char *path;
        fh = NULL;

        {
          const char *components[] = {terminal->keyBindings, ".hlp"};
          path = joinStrings(components, ARRAY_COUNT(components));
        }

        if (path) {
          if ((fh = fopen(path, "w"))) {
            fprintf(fh, "Help for %s\n\n", terminal->modelName);

            if (verbose)
              fprintf(stderr, "read_config: generating help file %s for %s\n", 
                      path, terminal->modelName);
          } else {
            perror("fopen");
            fprintf(stderr, "read_config: error creating help file %s for %s\n", 
                    path, terminal->modelName);
          }

          free(path);
        }

        if (!fh) continue;
      } else {
        if (verbose)
	  fprintf(stderr, "read_config: writing configuration records for: %s\n",
	          terminal->modelName);
      }
      
      if (!help)
	fprintf(fh, "# --------------------------------------------\n");

      fprintf(fh, "# Terminal Parameters:\n");
     
      fprintf(fh, "%s = %d\n", 
	      search_symbol(IDENT), terminal->modelIdentifier);
      fprintf(fh, "%s = \"%s\"\n", 
	      search_symbol(NAME), terminal->modelName);
      fprintf(fh, "%s = %d\n", 
	      search_symbol(PROTOCOLREVISION), terminal->protocolRevision);
      fprintf(fh, "%s = \"%s\"\n", 
	      search_symbol(HELPFILE), terminal->keyBindings);

      fprintf(fh, "%s = %d\n", 
	      search_symbol(DISPLAYSIZE),      terminal->textColumns);
      if (terminal->statusCount)
        fprintf(fh, "%s = %d\n", 
                search_symbol(STATCELLS), terminal->statusCount);
      if (terminal->frontKeys)
        fprintf(fh, "%s = %d\n", 
                search_symbol(FRONTKEYS), terminal->frontKeys);
      if (terminal->hasBar)
	fprintf(fh, "%s = %d %d %d %d\n",
                search_symbol(HASBAR),
                terminal->leftSwitches, terminal->rightSwitches,
                terminal->leftKeys, terminal->rightKeys);

      {
        int first = 1;
        int s;
        for (s=0; s<terminal->statusCount; s++) {
          uint16_t code = terminal->statusCells[s];
          if (code != OFFS_EMPTY) {
            if (first) {
              first = 0;
              fprintf(fh, "# Status Cells:\n");    
              fprintf(fh, "# flag: left half is cell number,"
                          " right half is no/all dots for off/on\n");
              fprintf(fh, "# horiz: horizontal two-digit number (for vertical status display)\n");
              fprintf(fh, "# number: vertical two-digit number (for horizontal status display)\n");
            }

            fprintf(fh, "%s %d = ", search_symbol(STAT), s+1);
            if (code >= OFFS_NUMBER) {
              fprintf(fh, "%s", search_symbol(NUMBER));
              code -= OFFS_NUMBER;
            } else if (code >= OFFS_FLAG) {
              fprintf(fh, "%s", search_symbol(FLAG));
              code -= OFFS_FLAG;
            } else if (code >= OFFS_HORIZ) {
              fprintf(fh, "%s", search_symbol(HORIZ));
              code -= OFFS_HORIZ;
            }

            fprintf(fh, " %s # %s\n",
                    search_code(STATCODE, code),
                    search_help_text(code + KEYS_STATUS));
          }
        }
      }

      {
        int first = 1;
        int m;
        for (m=0; m<terminal->modifierCount; m++)  {
          uint16_t key = terminal->modifierKeys[m];
          if (key) {
            if (first) {
              first = 0;
              fprintf(fh, "# Modifier Keys:\n");    
            }

            fprintf(fh, "modifier = %s\n", search_key(key));
          }
        }
      }

      fprintf(fh, "# Command Definitions:\n");    
      {
        int c;
        for (c=0; c<terminal->commandCount; c++) {
          const CommandDefinition *command = &terminal->commandDefinitions[c];
          if (command->code) {
            fprintf(fh, "%s", search_cmd(command->code & BRL_MSK_CMD));
            if (command->code & BRL_FLG_TOGGLE_MASK) 
              fprintf(fh, " %s", search_toggle(command->code));
            if (command->code & BRL_FLG_ROUTE) 
              fprintf(fh, " %s", search_code(ROUTE, 0));
            fprintf(fh, " = ");
            printkeys(fh, terminal, command);
            fprintf(fh, " # ");
            {
              const char *description = get_command_description(command->code & BRL_MSK_CMD);
              if (!*description) goto description_done;

              if (command->code & BRL_FLG_TOGGLE_MASK)  {
                int on = command->code & BRL_FLG_TOGGLE_ON;
                const char *firstBlank = strchr(description, ' ');
                if (firstBlank) {
                  {
                    const char *slash = memchr(description, '/', firstBlank-description);
                    if (slash) {
                      if (on) {
                        fprintf(fh, "%.*s", (int)(slash-description), description);
                      } else {
                        slash++;
                        fprintf(fh, "%.*s", (int)(firstBlank-slash), slash);
                      }
                      fprintf(fh, "%s", firstBlank);
                      goto toggle_done;
                    }
                  }

                  {
                    const char *oldVerb = "toggle";
                    int oldLength = strlen(oldVerb);
                    if ((oldLength == (firstBlank-description)) &&
                        (memcmp(description, oldVerb, oldLength) == 0)) {
                      const char *lastWord = strrchr(firstBlank, ' ');
                      if (lastWord) {
                        const char *slash = strchr(++lastWord, '/');
                        if (slash) {
                          fprintf(fh, "set%.*s", (int)(lastWord-firstBlank), firstBlank);
                          if (on) {
                            fprintf(fh, "%.*s", (int)(slash-lastWord), lastWord);
                          } else {
                            fprintf(fh, "%s", slash+1);
                          }
                          goto toggle_done;
                        }
                      }
                    }
                  }
                }

                fprintf(fh, "%s - set %s",
                        description, search_toggle(command->code));
                goto toggle_done;
              }
              fprintf(fh, "%s", description);
            toggle_done:

              if (command->code & BRL_FLG_ROUTE)
                fprintf(fh, " and route cursor");
            }

          description_done:
            fprintf(fh, "\n");
          }
        }
      }

      if (help)
	fclose(fh);
    }
  }
}

int main(int argc, char* argv[])
{
  if (argc > 1) {
    filename = argv[1];

    if (strlen(filename) == 1) {
      if (*filename=='d') {
        terminals(0, 0);
        return 0;
      }

      if (*filename=='D') {
        terminals(0, 1);
        return 0;
      }

      if (*filename=='h') {
        terminals(1, 0);
        return 0;
      }

      if (*filename=='H') {
        terminals(1, 1);
        return 0;
      }
    }
  }

  configurationFile = stdin;
  memset(pmTerminalTable, 0, sizeof(pmTerminalTable));
  parseConfigurationFile();
  terminals(0, 1);
  return 0;
}

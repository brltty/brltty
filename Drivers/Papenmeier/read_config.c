/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "config.tab.c"

/* taken from ../brl_load.c */
const CommandEntry commandTable[] = {
  #include "Programs/cmds.auto.h"
  {EOF, NULL, NULL}
};

static char* filename = "stdin";

static int
yyerror (char *problem)  /* Called by yyparse on error */
{
  fprintf(stderr, "%s[%d]: %s\n", filename, lineNumber, problem);
  exit(99);
 return 0;
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
  if ((cmd & ~VAL_ARG_MASK) == VAL_PASSKEY) {
    return search_code(VPK, cmd);
  } else {
    return search_code(KEYCODE, cmd);
  }
}

static char* search_onoff(int cmd)
{
  if (cmd & VAL_TOGGLE_ON)
    return search_code(ON, 0);
  if (cmd & VAL_TOGGLE_OFF)
    return search_code(OFF, 0);
  return "";
}

static char* search_key(int16_t key) {
  static char res[20];

  if (key == ROUTINGKEY) {
    snprintf(res, sizeof(res), "%s",
             search_code(ROUTING, 0));
    return res;
  }

  if (key >= OFFS_SWITCH) {
    static const unsigned char map[] = {0, 2, 3, 1};
    key -= OFFS_SWITCH + 1;
    key = (map[key >> 1] << 1) | (key & 0X1);
    snprintf(res, sizeof(res), "%s %s %s",
             (key & 0X04)? "key": "switch",
             (key & 0X02)? "right": "left",
             (key & 0X01)? "front": "rear");
    return res;
  }

  if (key >= OFFS_EASY) {
    snprintf(res, sizeof(res), "easy %s",
             search_code(EASYCODE, key-OFFS_EASY));
    return res;
  }

  if (key >=  OFFS_STAT) {
    snprintf(res, sizeof(res), "status %d", key-OFFS_STAT);
    return res;
  }

  if (key >=  OFFS_FRONT) {
    snprintf(res, sizeof(res), "front %d", key-OFFS_FRONT);
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
printkeys (FILE *fh, const CommandDefinition *cmd, const int16_t *modifiers) {
  const char *delimiter = "";
  int modifier;

  for (modifier=0; modifier<MODMAX; modifier++) {
    if (cmd->modifiers & (1 << modifier)) {
      fprintf(fh, "%s%s", delimiter, search_key(modifiers[modifier])); 
      delimiter = " and ";
    }
  }

  if (cmd->key != NOKEY) {
    fprintf(fh, "%s%s", delimiter, search_key(cmd->key));
  }
}

void terminals(int help, int verbose)
{
  int tn, i, j;
  FILE * fh = stdout;

  /* TODO: Comment/Help for status display */

  for(tn=0; tn < num_terminals; tn++) {
    if (pm_terminals[tn].name != 0) {
      if (help) {
	fh = fopen(pm_terminals[tn].helpFile, "wa");
	if (!fh) {
	  perror("fopen");
	  fprintf(stderr, "read_config: Error creating help file %s for %s.\n", 
		  pm_terminals[tn].helpFile, pm_terminals[tn].name);
	  continue;
	}
	if (verbose)
	  fprintf(stderr, "read_config: Generating help file %s for %s.\n", 
		  pm_terminals[tn].helpFile, pm_terminals[tn].name);
      } else {
        if (verbose)
	  fprintf(stderr, "read_config: Writing configuration records for %s.\n",
	          pm_terminals[tn].name);
      }
      
      if (!help)
	fprintf(fh, "# --------------------------------------------\n");

      fprintf(fh, "# Terminal Parameter:\n");
     
      fprintf(fh, "%s = %d\n", 
	      search_symbol(IDENT), pm_terminals[tn].identifier);
      fprintf(fh, "%s = \"%s\"\n", 
	      search_symbol(NAME), pm_terminals[tn].name);
      fprintf(fh, "%s = \"%s\"\n", 
	      search_symbol(HELPFILE), pm_terminals[tn].helpFile);

      fprintf(fh, "%s = %d\n", 
	      search_symbol(SIZE),      pm_terminals[tn].columns);
      fprintf(fh, "%s = %d\n", 
	      search_symbol(STATCELLS), pm_terminals[tn].statusCells);
      fprintf(fh, "%s = %d\n", 
	      search_symbol(FRONTKEYS), pm_terminals[tn].frontKeys);
      if (pm_terminals[tn].hasEasyBar)
	fprintf(fh, "%s\n", search_symbol(EASYBAR));

      fprintf(fh, "# Terminal settings:\n");

      fprintf(fh, "# Statusdisplay settings:\n");    
      fprintf(fh, "# flag: left half: line number"
	      "  right half: no/all bits set\n");
      fprintf(fh, "# horiz: two digits on vertikal status-display\n"
	      "# number: display two digits in one cell "
	      "when status on horiz.display\n");
      for(i=0; i < STATMAX; i++) 
	if(pm_terminals[tn].statshow[i] != OFFS_EMPTY)
	  {
	    int val = pm_terminals[tn].statshow[i];

	    fprintf(fh, "%s %d = ", search_symbol(STAT), i+1);
	    if (val >= OFFS_NUMBER) {
	      fprintf(fh, "%s", 
		      search_symbol(NUMBER));
	      val -= OFFS_NUMBER;
	    } else if (val >= OFFS_FLAG) {
	      fprintf(fh, "%s", 
		      search_symbol(FLAG));
	      val -= OFFS_FLAG;
	    } else if (val >= OFFS_HORIZ) {
	      fprintf(fh, "%s", 
		      search_symbol(HORIZ));
	      val -= OFFS_HORIZ;
	    }

	    fprintf(fh, " %s # %s\n",
		    search_code(STATCODE, val),
		    search_help_text(val + OFFS_STAT));
	  }

      fprintf(fh, "# Modifierkey and input mode dots - settings:\n");    
      for(i=0; i < MODMAX; i++) 
	if (pm_terminals[tn].modifiers[i] != 0 &&
	    pm_terminals[tn].modifiers[i] != ROUTINGKEY) 
	  fprintf(fh, "modifier = %s # dot %d\n", 
		  search_key(pm_terminals[tn].modifiers[i]), i+1);

      fprintf(fh, "# Commandkey settings:\n");    
      for(i=0; i < CMDMAX; i++) {
	if (pm_terminals[tn].commands[i].code != 0) {
	  char* txtand = "";
	  fprintf(fh, "%s",
		  search_cmd(pm_terminals[tn].commands[i].code & VAL_CMD_MASK));
	  if (pm_terminals[tn].commands[i].code & VAL_TOGGLE_MASK) 
	    fprintf(fh, " %s", search_onoff(pm_terminals[tn].commands[i].code) );
	  fprintf(fh, " = ");
	  printkeys(fh, &pm_terminals[tn].commands[i], pm_terminals[tn].modifiers);
	  fprintf(fh, " # %s",
		  get_command_description(pm_terminals[tn].commands[i].code & VAL_CMD_MASK));
	  if (pm_terminals[tn].commands[i].code & VAL_TOGGLE_MASK) 
	    fprintf(fh, " - set %s", search_onoff(pm_terminals[tn].commands[i].code) );
	  fprintf(fh, "\n");
	}
      }
      if (help)
	fclose (fh);
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
  memset(&pm_terminals, 0, sizeof(pm_terminals));
  parse ();
  terminals(0, 1);
  return 0;
}

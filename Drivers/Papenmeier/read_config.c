/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

static int yyerror (char* s)  /* Called by yyparse on error */
{
  fprintf(stderr, "%s[%d]: %s\n", filename, linenumber, s);
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

static char* search_easycode(int cmd)
{
  return search_code(EASYCODE, cmd);
}

static char* search_stat(int cmd)
{
  return search_code(STATCODE, cmd);
}

static char* search_cmd(int cmd)
{
  if ((cmd & ~VAL_ARG_MASK) == VAL_PASSKEY) {
    return search_code(VPK, cmd);
  } else
    return search_code(KEYCODE, cmd);
}

static char* search_onoff(int cmd)
{
  if (cmd & VAL_SWITCHON)
    return search_code(ON, 0);
  if (cmd & VAL_SWITCHOFF)
    return search_code(OFF, 0);
  return "";
}

static char* search_key(int code) {
  static char res[20];

  if (code == ROUTINGKEY) {
    strcpy(res, search_code(ROUTING, 0));
    return res;
  }
  if (code >= OFFS_SWITCH) {
    strcpy(res, "switch  ");
    res[7]= '0' + (code-OFFS_SWITCH) % 10;
    return res;
  } else if (code >= OFFS_EASY) {
    strcpy(res, "easy  ");
    strcpy(res+5, search_easycode(code - OFFS_EASY));
    return res;
  } else if (code >=  OFFS_STAT) {
    strcpy(res, "status    ");
    code -= OFFS_STAT;
  } else
    strcpy(res, "front     ");
  if (code >= 10)
    res[7]= '0' + code / 10;
  res[8]= '0' + code % 10;
  return res;
}

/* --------------------------------------------------------------- */
/* Param help is true, when procedure generates one helpfile for
   each terminaltype. Otherwise the procedure generates one
   big parameterfile via stdout                                    */

void printkeys(FILE * fh, commands* cmd, int modifiers[])
{
  char* txtand = "";
  int j;
  if (cmd->keycode != NOKEY) {
    fprintf(fh, "%s ", search_key(cmd->keycode));
    txtand = " and ";
  }
  for(j=0; j < MODMAX; j++)
    if ( ((1<<j) & (cmd->modifiers))==(1<<j)) {
      fprintf(fh, "%s%s",
	      txtand, search_key(modifiers[j])); 
      txtand = " and ";
    }
} 

void terminals(int help, int verbose)
{
  int tn, i, j;
  FILE * fh = stdout;

  /* TODO: Comment/Help for status display */

#define CMDMASK    (VAL_SWITCHON-1)

  for(tn=0; tn < num_terminals; tn++) {
    if (pm_terminals[tn].name != 0) {
      if (help) {
	fh = fopen(pm_terminals[tn].helpfile, "wa");
	if (!fh) {
	  perror("fopen");
	  fprintf(stderr, "read_config: Error creating help file %s for %s.\n", 
		  pm_terminals[tn].helpfile, pm_terminals[tn].name);
	  continue;
	}
	if (verbose)
	  fprintf(stderr, "read_config: Generating help file %s for %s.\n", 
		  pm_terminals[tn].helpfile, pm_terminals[tn].name);
      } else {
        if (verbose)
	  fprintf(stderr, "read_config: Writing configuration records for %s.\n",
	          pm_terminals[tn].name);
      }
      
      if (!help)
	fprintf(fh, "# --------------------------------------------\n");

      fprintf(fh, "# Terminal Parameter:\n");
     
      fprintf(fh, "%s = %d\n", 
	      search_symbol(IDENT), pm_terminals[tn].ident);
      fprintf(fh, "%s = \"%s\"\n", 
	      search_symbol(NAME), pm_terminals[tn].name);
      fprintf(fh, "%s = \"%s\"\n", 
	      search_symbol(HELPFILE), pm_terminals[tn].helpfile);

      fprintf(fh, "%s = %d\n", 
	      search_symbol(SIZE),      pm_terminals[tn].x);
      fprintf(fh, "%s = %d\n", 
	      search_symbol(STATCELLS), pm_terminals[tn].statcells);
      fprintf(fh, "%s = %d\n", 
	      search_symbol(FRONTKEYS), pm_terminals[tn].frontkeys);
      if (pm_terminals[tn].haseasybar)
	fprintf(fh, "%s\n", search_symbol(EASYBAR));

      fprintf(fh, "# Terminal settings:\n");

      fprintf(fh, "# Statusdisplay settings:\n");    
      fprintf(fh, "# flag: left half: line number"
	      "  right half: no/all bits set\n");
      fprintf(fh, "# horiz: two digits on vertikal status-display\n"
	      "# number: display two digits in one cell "
	      "when status on horiz.display\n");
      for(i=0; i < STATMAX; i++) 
	if(pm_terminals[tn].statshow[i] != STAT_EMPTY)
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
		    search_stat(val),
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
	if (pm_terminals[tn].cmds[i].code != 0) {
	  char* txtand = "";
	  fprintf(fh, "%s",
		  search_cmd(pm_terminals[tn].cmds[i].code & CMDMASK));
	  if (pm_terminals[tn].cmds[i].code >= CMDMASK) 
	    fprintf(fh, " %s", search_onoff(pm_terminals[tn].cmds[i].code) );
	  fprintf(fh, " = ");

	  /*
	  if (pm_terminals[tn].cmds[i].keycode != NOKEY) {
	    fprintf(fh, "%s ", search_key(pm_terminals[tn].cmds[i].keycode));
	    txtand = " and ";
	  }
	  for(j=0; j < MODMAX; j++)
	    if ( ((1<<j) &pm_terminals[tn].cmds[i].modifiers)==(1<<j)) {
	      fprintf(fh, "%s%s",
		      txtand, search_key(pm_terminals[tn].modifiers[j])); 
	      txtand = " and ";
	    }
	  */

	  printkeys(fh, &pm_terminals[tn].cmds[i], pm_terminals[tn].modifiers);
	  fprintf(fh, " # %s",
		  get_command_description(pm_terminals[tn].cmds[i].code & CMDMASK));
	  if (pm_terminals[tn].cmds[i].code > CMDMASK) 
	    fprintf(fh, " - set %s", search_onoff(pm_terminals[tn].cmds[i].code) );
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

  configfile = stdin;
  memset(&pm_terminals, 0, sizeof(pm_terminals));
  parse ();
  terminals(0, 1);
  return 0;
}

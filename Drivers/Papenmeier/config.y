%{
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

/*
 * Support for all Papenmeier Terminal + config file
 *   Heimo.Schön <heimo.schoen@gmx.at>
 *   August Hörandl <august.hoerandl@gmx.at>
 *
 * Papenmeier/read_config.y
 *  read (scan + interpret) the configuration file - included by braille.c
 *  this file can be used as a standalone test programm - see
 *   Makefile for details
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "brl-cfg.h"

static int yylex(void);
static int yyerror(char*);    
static int yyparse();

/* to be called: */
static int parse (void);

static char *nameval = NULL;
static int numval, keyindex, cmdval; 

static int numkeys = 0;
static int16_t keys[KEYMAX];

static FILE *configurationFile = NULL;
static int lineNumber = 1;

 /* --------------------------------------------------------------- */
 /* some functions set_*(): set the big table with the values read  */
 
 /* global var: the current entry (index) */
 static int currentTerminal = -1;
 /* the number of modifiers - last entry used */
 static int currentModifier;
 /* the number of defined commands - last entry use*/
 static int currentCommand;

 static void
 assert_condition (int condition, char *problem, int line) {
   if (!condition) {
     yyerror(problem);
   }
 }

 static void
 assert_started (int line) {
  assert_condition((currentTerminal >= 0), "terminal not being defined", line);
 }

 static inline void set_modifier(int code);

 static inline void set_ident(int num)
  {
    int i;
    TerminalDefinition *terminal = &pm_terminals[++currentTerminal];
    assert_condition((currentTerminal < num_terminals), "too many terminals", __LINE__);
    currentModifier = -1;
    currentCommand = -1;
    terminal->identifier = num;

    terminal->columns = 0;
    terminal->rows = 1;

    terminal->statusCells = 0;
    terminal->frontKeys = 0;
    terminal->hasEasyBar = 0;
  }

 static inline void set_name(char* nameval)
  {
    assert_started(__LINE__);
    strncpy(pm_terminals[currentTerminal].name, nameval,
    	    sizeof(pm_terminals[currentTerminal].name));
  }


 static inline void set_help(char* name)
   {
     assert_started(__LINE__);
     strncpy(pm_terminals[currentTerminal].helpFile, name,
     	     sizeof(pm_terminals[currentTerminal].helpFile));
   }

 static inline void set_size(int code)
  {
    assert_started(__LINE__);
    pm_terminals[currentTerminal].columns = code;
  }

 static inline void set_statcells(int code)
  {
    assert_started(__LINE__);
    pm_terminals[currentTerminal].statusCells = code;
  }

 static inline void set_frontkeys(int code)
  {
    assert_started(__LINE__);
    pm_terminals[currentTerminal].frontKeys = code;
  }

 static inline void set_haseasybar(int code)
  {
    assert_started(__LINE__);
    pm_terminals[currentTerminal].hasEasyBar = code;
  }

 static inline void set_showstat(int pos, int code)
  {
    assert_started(__LINE__);
    assert_condition(((0 < pos) && (pos <= STATMAX)), "invalid status cell number", __LINE__);
    pm_terminals[currentTerminal].statshow[pos-1] = code;
  }

 static inline void set_modifier(int code)
  {
    assert_started(__LINE__);
    currentModifier++;
    assert_condition((currentModifier < MODMAX), "too many modifiers", __LINE__);
    pm_terminals[currentTerminal].modifiers[currentModifier] = code;
  }

 static inline void add_command(int code, int numkeys, int16_t keys[])
  {
    TerminalDefinition *terminal;
    CommandDefinition *cmd;
    int k, m;

    assert_started(__LINE__);
    terminal = &pm_terminals[currentTerminal];

    currentCommand++;
    assert_condition((currentCommand < CMDMAX), "too many commands", __LINE__);
    cmd = &terminal->commands[currentCommand];
    cmd->code = code;
    cmd->key = NOKEY;
    cmd->modifiers = 0;

    for (k=0; k<numkeys; k++) {
      int16_t key = keys[k];
      int found = 0;
      for (m=0; m<=currentModifier; m++) {
	if (key == terminal->modifiers[m]) {
          int bit = 1 << m;
          assert_condition(!(cmd->modifiers & bit), "duplicate modifier", __LINE__);
          cmd->modifiers |= bit;
          found = 1;
          break;
	}
      }

      if (!found) {
        assert_condition((cmd->key == NOKEY), "more than one key", __LINE__);
        cmd->key = key;
      }
    }
  }

%}

%start input

%token NUM STRING IS AND
%token NAME IDENT HELPFILE SIZE STATCELLS FRONTKEYS EASYBAR
%token MODIFIER
%token ROUTING EASY SWITCH KEY
%token LEFT RIGHT REAR FRONT
%token STAT KEYCODE STATCODE EASYCODE
%token NUMBER FLAG HORIZ
%token ON OFF 
%token VPK
%%

input:    /* empty */
       | input inputline
       ;

inputline:  '\n'
       | error '\n'                 { yyerrok;  }
       | IDENT eq NUM '\n'          { set_ident(numval); }
       | NAME eq STRING '\n'        { set_name(nameval); }
       | HELPFILE eq STRING '\n'    { set_help(nameval); }
       | SIZE eq NUM '\n'           { set_size(numval); }
       | STATCELLS eq NUM '\n'      { set_statcells(numval); }
       | FRONTKEYS eq NUM '\n'      { set_frontkeys(numval); }
       | EASYBAR '\n'               { set_haseasybar(1); }
       | EASYBAR eq NUM '\n'        { set_haseasybar(numval != 0); }

       | statdef eq statdisp '\n'  { set_showstat(keyindex, numval);  }
       | MODIFIER eq anykey '\n'   { set_modifier(keyindex); }
       | keycode eq modifiers '\n' { add_command(cmdval, numkeys, keys); }
       | keycode ON eq modifiers '\n' { add_command(cmdval | VAL_TOGGLE_ON, numkeys, keys); }
       | keycode OFF eq modifiers '\n' { add_command(cmdval | VAL_TOGGLE_OFF, numkeys, keys); }
       ;

eq:    '='
       | IS
       | /* empty */
       ;

and:   '+'
       | AND
       | /* empty */
       ;

statdef: STAT NUM { keyindex=numval;  } 
       ;

keycode: KEYCODE { cmdval=numval; numkeys = 0; }
       | VPK { cmdval = numval; numkeys = 0; } 
       ;

statdisp: STATCODE            {  }
        | HORIZ STATCODE      { numval += OFFS_HORIZ; }
        | FLAG STATCODE       { numval += OFFS_FLAG; }
        | NUMBER STATCODE     { numval += OFFS_NUMBER; }
        ;

anykey:   STAT   NUM         { keyindex= OFFS_STAT + numval; } 
        | FRONT  NUM         { keyindex= OFFS_FRONT + numval; } 
        | EASY   EASYCODE    { keyindex= OFFS_EASY + numval; } 
        | SWITCH LEFT  REAR  { keyindex= OFFS_SWITCH + SWITCH_LEFT_REAR; }
        | SWITCH LEFT  FRONT { keyindex= OFFS_SWITCH + SWITCH_LEFT_FRONT; }
        | SWITCH RIGHT REAR  { keyindex= OFFS_SWITCH + SWITCH_RIGHT_REAR; }
        | SWITCH RIGHT FRONT { keyindex= OFFS_SWITCH + SWITCH_RIGHT_FRONT; }
        | KEY    LEFT  REAR  { keyindex= OFFS_SWITCH + KEY_LEFT_REAR; }
        | KEY    LEFT  FRONT { keyindex= OFFS_SWITCH + KEY_LEFT_FRONT; }
        | KEY    RIGHT REAR  { keyindex= OFFS_SWITCH + KEY_RIGHT_REAR; }
        | KEY    RIGHT FRONT { keyindex= OFFS_SWITCH + KEY_RIGHT_FRONT; }
        | ROUTING            { keyindex= ROUTINGKEY; }
        ; 

modifiers: modifier
         | modifier and modifiers
         ;

modifier : anykey { keys[numkeys++] = keyindex; } 
         ;

%%

/* --------------------------------------------------------------- */
/* all the keywords */
/* the commands CMD_* CR_* in Programs/brl.h are autogenerated - see Makefile */

struct init_v
{
  char*  sname;			/* symbol (lowercase) */
  int token;			/* yacc token */
  int val;			/* key value */
};

static struct init_v symbols[]= {
  { "helpfile",    HELPFILE ,0 },
  { "is",          IS, 0 },
  { "and",         AND, 0 },

  { "identification", IDENT, 0 },
  { "identity",    IDENT, 0 },
  { "terminal",    NAME, 0 },
  { "type",        NAME, 0 },
  { "typ",         NAME, 0 },

  { "modifier",    MODIFIER, 0},

  { "displaysize", SIZE, 0 },
  { "statuscells", STATCELLS, 0 },
  { "frontkeys",   FRONTKEYS, 0 },
  { "haseasybar",  EASYBAR, 0 },
  { "easybar",     EASYBAR, 0 },

  { "status",      STAT,   0 },
  { "easy",        EASY,   0 },

  { "front",       FRONT,  0 },
  { "rear",        REAR,   0 },
  { "left",        LEFT,   0 },
  { "right",       RIGHT,  0 },

  { "switch",      SWITCH, 0 },
  { "key",         KEY,    0 },

  { "horiz",       HORIZ,  0 },
  { "flag",        FLAG,   0 },
  { "number",      NUMBER, 0 },

  { "left1",       EASYCODE, EASY_L1},
  { "left2",       EASYCODE, EASY_L2 },
  { "up1",         EASYCODE, EASY_U1},
  { "up2",         EASYCODE, EASY_U2 },
  { "right1",      EASYCODE, EASY_R1},
  { "right2",      EASYCODE, EASY_R2 },
  { "down1",       EASYCODE, EASY_D1},
  { "down2",       EASYCODE, EASY_D2 },

  { "routing",     ROUTING, 0 },
  { "route",       ROUTING, 0 },

  { "on",          ON, 0 },
  { "off",         OFF, 0 },

#include "cmd.auto.h"
  { "EOF", KEYCODE, -1 },
  { NULL, 0, 0 }
};

/* --------------------------------------------------------------- */
/* all the help */
/* the commands CMD_* in Programs/brl.h are autogenerated - see Makefile */

struct help_v
{
  int    cmd;			/* cmd */
  char*  txt;			/* help text */
};

static struct help_v hlptxt[]= {
#include "hlp.auto.h"
  { 0, NULL }
};


/* --------------------------------------------------------------- */

int yylex ()
{
  int c;
  static char symbuf[40] = { 0 };
  const int length = sizeof(symbuf)/sizeof(symbuf[0]);

  /* Ignore whitespace, get first nonwhite character.  */
  while ((c = getc(configurationFile)) == ' ' || c == '\t')
    ;

  if (c == EOF)
    return 0;

  if (c == '#') {		/* comment to end of line */
    do {
      c = getc(configurationFile);
    } while (c != '\n' && c != EOF);
    lineNumber++;
    return '\n';
  }

  /* Char starts a number => parse the number. */
  if (c == '.' || isdigit (c)) {
    ungetc (c, configurationFile);
    fscanf (configurationFile, "%d", &numval);
    return NUM;
  }

  if (c == '"') {		/* string */
    int i=0;
    symbuf[0] = '\0';
    c = getc(configurationFile);
    while(c !='"' && c != EOF) {
      /* If buffer is full */
      if (i == length)
	break;
      /* Add this character to the buffer. */
      symbuf[i++] = c;
      c = getc(configurationFile);
    }
    symbuf[i] = 0;
    nameval = symbuf;
    return STRING;
  }

  /* Char starts an identifier => read the name. */
  if (isalpha (c) || c=='_') {
    int i = 0;
    do {
      /* If buffer is full */
      if (i == length)
	break;
      
      /* Add this character to the buffer. */
      symbuf[i++] = c;
      c = getc(configurationFile);
    } while (c != EOF && (isalnum (c) || c=='_'));
     
    ungetc (c, configurationFile);
    symbuf[i] = 0;
    for(i=0;symbols[i].sname; i++)
      if(strcasecmp(symbuf,symbols[i].sname) == 0) {
	numval = symbols[i].val;
	return symbols[i].token;
      }
    /* unused feature */
    nameval = symbuf;
    return NAME;
  }
     
  /* Any other character is a token by itself. */
  if (c == '\n')
    lineNumber++;

  return c;
}


/* --------------------------------------------------------------- */

int
parse (void) {
  lineNumber = 1;
  currentTerminal = -1;

  nameval = NULL;
  numval = 0;
  keyindex = 0;
  cmdval = 0; 
  numkeys = 0;

  return yyparse ();
}

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

static int terminalsSize;
static int commandsSize;

static int
reallocateTable (void **address, int size, int count) {
  void *newAddress = realloc(*address, (count * size));
  if (!count || (newAddress != NULL)) {
    *address = newAddress;
    return 1;
  } else {
    yyerror("insufficient memory");
  }
  return 0;
}

static int
ensureTableSize (void **address, int size, int count, int *limit, int increment) {
  if (count == *limit) {
    int newLimit = *limit + increment;
    if (!reallocateTable(address, size, newLimit)) return 0;
    *limit = newLimit;
  }
  return 1;
}

static TerminalDefinition *
getCurrentTerminal (void) {
  if (pmTerminalCount > 0) return &pmTerminals[pmTerminalCount - 1];
  yyerror("terminal not being defined");
  return NULL;
}

static int
finishCurrentTerminal (void) {
  if (pmTerminalCount) {
    TerminalDefinition *terminal = &pmTerminals[pmTerminalCount - 1];
    if (!reallocateTable((void **)&terminal->statshow, sizeof(*terminal->statshow), terminal->statusCount)) return 0;
    if (!reallocateTable((void **)&terminal->modifiers, sizeof(*terminal->modifiers), terminal->modifierCount)) return 0;
    if (!reallocateTable((void **)&terminal->commands, sizeof(*terminal->commands), terminal->commandCount)) return 0;
  }
  return 1;
}

static TerminalDefinition *
addTerminal (int identifier) {
  if (finishCurrentTerminal()) {
    if (ensureTableSize((void **)&pmTerminals, sizeof(*pmTerminals),
                        pmTerminalCount, &terminalsSize, 4)) {
      TerminalDefinition *terminal = &pmTerminals[pmTerminalCount++];
      terminal->identifier = identifier;
      terminal->name = NULL;
      terminal->helpFile = NULL;

      terminal->columns = 0;
      terminal->rows = 1;

      terminal->frontKeys = 0;
      terminal->hasEasyBar = 0;

      terminal->statusCount = 0;
      terminal->modifierCount = 0;
      terminal->commandCount = 0;

      terminal->statshow = NULL;
      terminal->modifiers = NULL;
      terminal->commands = NULL;

      commandsSize = 0;
      return terminal;
    }
  }
  return NULL;
}

static int
setName (char *name) {
  TerminalDefinition *terminal = getCurrentTerminal();
  if (terminal) {
    if (!terminal->name) {
      terminal->name = strdup(name);
      return 1;
    } else {
      yyerror("duplicate name");
    }
  }
  return 0;
}

static int
setHelp (char *file) {
  TerminalDefinition *terminal = getCurrentTerminal();
  if (terminal) {
    if (!terminal->helpFile) {
      terminal->helpFile = strdup(file);
      return 1;
    } else {
      yyerror("duplicate help file");
    }
  }
  return 0;
}

static int
setColumns (int columns) {
  TerminalDefinition *terminal = getCurrentTerminal();
  if (terminal) {
    terminal->columns = columns;
    return 1;
  }
  return 0;
}

static int
setStatusCells (int count) {
  TerminalDefinition *terminal = getCurrentTerminal();
  if (terminal) {
    terminal->statusCount = count;
    terminal->statshow = malloc(count * sizeof(terminal->statshow[0]));

    {
      int s;
      for (s=0; s<count; s++) terminal->statshow[s] = OFFS_EMPTY;
    }

    return 1;
  }
  return 0;
}

static int
setFrontKeys (int count) {
  TerminalDefinition *terminal = getCurrentTerminal();
  if (terminal) {
    terminal->frontKeys = count;
    return 1;
  }
  return 0;
}

static int
setHasEasyBar (int yes) {
  TerminalDefinition *terminal = getCurrentTerminal();
  if (terminal) {
    terminal->hasEasyBar = yes;
    return 1;
  }
  return 0;
}

static int
setStatusCell (int number, int format) {
  TerminalDefinition *terminal = getCurrentTerminal();
  if (terminal) {
    if ((0 < number) && (number <= terminal->statusCount)) {
      uint16_t *cell = &terminal->statshow[number-1];
      if (*cell == OFFS_EMPTY) {
        *cell = format;
        return 1;
      } else {
        yyerror("duplicate status cell");
      }
    } else {
      yyerror("invalid status cell number");
    }
  }
  return 0;
}

static int
addModifier (int key) {
  TerminalDefinition *terminal = getCurrentTerminal();
  if (terminal) {
    if (terminal->modifierCount < MODMAX) {
      if (!terminal->modifiers)
        terminal->modifiers = malloc(MODMAX * sizeof(terminal->modifiers[0]));
      if (terminal->modifiers) {
        terminal->modifiers[terminal->modifierCount++] = key;
        return 1;
      }
    } else {
      yyerror("too many modifiers");
    }
  }
  return 0;
}

static CommandDefinition *
addCommand (int code) {
  TerminalDefinition *terminal = getCurrentTerminal();
  if (terminal) {
    if (ensureTableSize((void **)&terminal->commands, sizeof(*terminal->commands),
                        terminal->commandCount, &commandsSize, 0X20)) {
      CommandDefinition *cmd = &terminal->commands[terminal->commandCount++];;
      int k;

      cmd->code = code;
      cmd->key = NOKEY;
      cmd->modifiers = 0;

      for (k=0; k<numkeys; k++) {
        int16_t key = keys[k];
        int found = 0;
        int m;
        for (m=0; m<terminal->modifierCount; m++) {
          if (key == terminal->modifiers[m]) {
            int bit = 1 << m;
            if (cmd->modifiers & bit) {
              yyerror("duplicate modifier");
              goto removeCommand;
            }
            cmd->modifiers |= bit;
            found = 1;
            break;
          }
        }

        if (!found) {
          if (cmd->key != NOKEY) {
            yyerror("more than one nonmodifier key");
            goto removeCommand;
          }
          cmd->key = key;
        }
      }
      return cmd;

    removeCommand:
      terminal->commandCount--;
    }
  }
  return NULL;
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
       | IDENT eq NUM '\n'          { addTerminal(numval); }
       | NAME eq STRING '\n'        { setName(nameval); }
       | HELPFILE eq STRING '\n'    { setHelp(nameval); }
       | SIZE eq NUM '\n'           { setColumns(numval); }
       | STATCELLS eq NUM '\n'      { setStatusCells(numval); }
       | FRONTKEYS eq NUM '\n'      { setFrontKeys(numval); }
       | EASYBAR '\n'               { setHasEasyBar(1); }
       | EASYBAR eq NUM '\n'        { setHasEasyBar(numval != 0); }

       | statdef eq statdisp '\n'  { setStatusCell(keyindex, numval);  }
       | MODIFIER eq anykey '\n'   { addModifier(keyindex); }
       | keycode eq modifiers '\n' { addCommand(cmdval); }
       | keycode ON eq modifiers '\n' { addCommand(cmdval | VAL_TOGGLE_ON); }
       | keycode OFF eq modifiers '\n' { addCommand(cmdval | VAL_TOGGLE_OFF); }
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
  if (pmTerminalsAllocated) {
    while (pmTerminalCount) {
      TerminalDefinition *terminal = &pmTerminals[--pmTerminalCount];
      if (terminal->name) free(terminal->name);
      if (terminal->helpFile) free(terminal->helpFile);
      if (terminal->statshow) free(terminal->statshow);
      if (terminal->modifiers) free(terminal->modifiers);
      if (terminal->commands) free(terminal->commands);
    }

    if (pmTerminals) {
      free(pmTerminals);
      pmTerminals = NULL;
    }
  } else {
    pmTerminalCount = 0;
    pmTerminals = NULL;
    pmTerminalsAllocated = 1;
  }

  lineNumber = 1;
  terminalsSize = 0;

  nameval = NULL;
  numval = 0;
  keyindex = 0;
  cmdval = 0;
  numkeys = 0;

  {
    int result = yyparse();
    finishCurrentTerminal();
    return result;
  }
}

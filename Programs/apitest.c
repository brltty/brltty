/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

/* apitest provides a small test utility for BRLTTY's API */
 
#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "options.h"
#include "api.h"
#include "brldefs.h"
#include "cmd.h"

static brlapi_settings_t settings;

static int opt_learnMode;
static int opt_showDots;
static int opt_showIdentifier;
static int opt_showName;
static int opt_showSize;
static int opt_showKeyCodes;
static int opt_suspendMode;

BEGIN_OPTION_TABLE
  {"identifier", NULL, 'i', 0, 0,
   &opt_showIdentifier, NULL,
   "Show the driver's identifier."},

  {"name", NULL, 'n', 0, 0,
   &opt_showName, NULL,
   "Show the driver's name."},

  {"window", NULL, 'w', 0, 0,
   &opt_showSize, NULL,
   "Show the braille window's size."},

  {"dots", NULL, 'd', 0, 0,
   &opt_showDots, NULL,
   "Show dot pattern."},

  {"learn", NULL, 'l', 0, 0,
   &opt_learnMode, NULL,
   "Enter interactive command learn mode."},

  {"keycodes", NULL, 'k', 0, 0,
   &opt_showKeyCodes, NULL,
   "Enter interactive keycode learn mode."}, 

  {"suspend", NULL, 's', 0, 0,
   &opt_suspendMode, NULL,
   "Suspend driver (press ^C on the PC keyboard or send SIGUSR1 to get back braille)."},

  {"brlapi-server", "[host][:port]", 'S', 0, 0,
   &settings.hostName, NULL,
   "The host name (or address) and port of the BrlAPI server."},

  {"brlapi-key", "file", 'K', 0, 0,
   &settings.authKey, NULL,
   "The path to the file containing BrlAPI's authorization key."},
END_OPTION_TABLE

void showDisplaySize(void)
{
  unsigned int x, y;
  fprintf(stderr,"Getting display size: ");
  if (brlapi_getDisplaySize(&x, &y)<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%dX%d\n", x, y);
}

void showDriverIdentifier(void)
{
  char id[3];
  fprintf(stderr, "Getting driver id: ");
  if (brlapi_getDriverId(id, sizeof(id))<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%s\n", id);
}

void showDriverName(void)
{
  char name[30];
  fprintf(stderr, "Getting driver name: ");
  if (brlapi_getDriverName(name, sizeof(name))<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%s\n", name);
}

#define DOTS_TEXT "dots: "
#define DOTS_TEXTLEN (strlen(DOTS_TEXT))
#define DOTS_LEN 8
#define DOTS_TOTALLEN (DOTS_TEXTLEN+DOTS_LEN)
void showDots(void)
{
  unsigned int x, y;
  brl_keycode_t k;
  if (brlapi_getDisplaySize(&x, &y)<0) {
    brlapi_perror("failed");
    exit(1);
  }
  if (brlapi_getTty(-1, NULL)<0) {
    brlapi_perror("getTty");
    exit(1);
  }
  if (x*y<DOTS_TOTALLEN) {
    fprintf(stderr,"can't show dots with a braille display with less than %d cells\n",(int)DOTS_TOTALLEN);
    exit(1);
  }
  {
    char text[x*y];
    unsigned char or[x*y];
    brlapi_writeStruct ws = BRLAPI_WRITESTRUCT_INITIALIZER;
    fprintf(stderr,"Showing dot patterns\n");
    memcpy(text,DOTS_TEXT,DOTS_TEXTLEN);
    memset(text+DOTS_TEXTLEN,' ',sizeof(text)-DOTS_TEXTLEN);
    ws.text = text;
    memset(or,0,sizeof(or));
    or[DOTS_TEXTLEN+0] = BRL_DOT1;
    or[DOTS_TEXTLEN+1] = BRL_DOT2;
    or[DOTS_TEXTLEN+2] = BRL_DOT3;
    or[DOTS_TEXTLEN+3] = BRL_DOT4;
    or[DOTS_TEXTLEN+4] = BRL_DOT5;
    or[DOTS_TEXTLEN+5] = BRL_DOT6;
    or[DOTS_TEXTLEN+6] = BRL_DOT7;
    or[DOTS_TEXTLEN+7] = BRL_DOT8;
    ws.attrOr = or;
    if (brlapi_write(&ws)<0) {
      brlapi_perror("brlapi_write");
      exit(1);
    }
  }
  brlapi_readKey(1, &k);
}

void enterLearnMode(void)
{
  int res;
  brl_keycode_t code;
  int cmd;
  char buf[0X100];

  fprintf(stderr,"Entering command learn mode\n");
  if (brlapi_getTty(-1, NULL)<0) {
    brlapi_perror("getTty");
    return;
  }

  if (brlapi_writeText(0, "command learn mode")<0) {
    brlapi_perror("brlapi_writeText");
    exit(1);
  }

  while ((res = brlapi_readKey(1, &code)) != -1) {
    fprintf(stderr, "got key %016"BRLAPI_PRIxKEYCODE"\n",code);
    switch (code & BRLAPI_KEY_TYPE_MASK) {
    case BRLAPI_KEY_TYPE_CMD:
      cmd = ((code&BRLAPI_KEY_CMD_BLK_MASK)>>8)|(code&BRLAPI_KEY_CMD_ARG_MASK);
      break;
    case BRLAPI_KEY_TYPE_SYM: {
        unsigned long keysym;
        keysym = code & BRLAPI_KEY_CODE_MASK;
	switch (keysym) {
	case BRLAPI_KEY_SYM_BACKSPACE: cmd=BRL_BLK_PASSKEY|BRL_KEY_BACKSPACE; break;
	case BRLAPI_KEY_SYM_TAB: cmd=BRL_BLK_PASSKEY|BRL_KEY_TAB; break;
	case BRLAPI_KEY_SYM_ENTER: cmd=BRL_BLK_PASSKEY|BRL_KEY_ENTER; break;
	case BRLAPI_KEY_SYM_ESCAPE: cmd=BRL_BLK_PASSKEY|BRL_KEY_ESCAPE; break;
	case BRLAPI_KEY_SYM_HOME: cmd=BRL_BLK_PASSKEY|BRL_KEY_HOME; break;
	case BRLAPI_KEY_SYM_CURSOR_LEFT: cmd=BRL_BLK_PASSKEY|BRL_KEY_CURSOR_LEFT; break;
	case BRLAPI_KEY_SYM_CURSOR_UP: cmd=BRL_BLK_PASSKEY|BRL_KEY_CURSOR_UP; break;
	case BRLAPI_KEY_SYM_CURSOR_RIGHT: cmd=BRL_BLK_PASSKEY|BRL_KEY_CURSOR_RIGHT; break;
	case BRLAPI_KEY_SYM_CURSOR_DOWN: cmd=BRL_BLK_PASSKEY|BRL_KEY_CURSOR_DOWN; break;
	case BRLAPI_KEY_SYM_PAGE_UP: cmd=BRL_BLK_PASSKEY|BRL_KEY_PAGE_UP; break;
	case BRLAPI_KEY_SYM_PAGE_DOWN: cmd=BRL_BLK_PASSKEY|BRL_KEY_PAGE_DOWN; break;
	case BRLAPI_KEY_SYM_END: cmd=BRL_BLK_PASSKEY|BRL_KEY_END; break;
	case BRLAPI_KEY_SYM_INSERT: cmd=BRL_BLK_PASSKEY|BRL_KEY_INSERT; break;
	case BRLAPI_KEY_SYM_DELETE: cmd=BRL_BLK_PASSKEY|BRL_KEY_DELETE; break;
	default:
	  if (keysym < 0x100)
	    cmd = BRL_BLK_PASSCHAR|keysym;
	  else if (keysym >= BRLAPI_KEY_SYM_FUNCTION && keysym <= BRLAPI_KEY_SYM_FUNCTION + 34)
	    cmd = BRL_BLK_PASSKEY | (BRL_KEY_FUNCTION + keysym - BRLAPI_KEY_SYM_FUNCTION);
	  else {
	    fprintf(stderr,"unknown code %"BRLAPI_PRIxKEYCODE"\n", code);
	    cmd = -1;
	  }
	  break;
	}
	break;
      }
    default:
      fprintf(stderr,"unknown code %"BRLAPI_PRIxKEYCODE, code);
      cmd = -1;
      break;
    }
    cmd = cmd
    | (code & BRLAPI_KEY_FLG_TOGGLE_ON		? BRL_FLG_TOGGLE_ON	: 0)
    | (code & BRLAPI_KEY_FLG_TOGGLE_OFF		? BRL_FLG_TOGGLE_OFF	: 0)
    | (code & BRLAPI_KEY_FLG_ROUTE		? BRL_FLG_ROUTE		: 0)
    | (code & BRLAPI_KEY_FLG_REPEAT_INITIAL	? BRL_FLG_REPEAT_INITIAL: 0)
    | (code & BRLAPI_KEY_FLG_REPEAT_DELAY	? BRL_FLG_REPEAT_DELAY	: 0)
    | (code & BRLAPI_KEY_FLG_LINE_SCALED	? BRL_FLG_LINE_SCALED	: 0)
    | (code & BRLAPI_KEY_FLG_LINE_TOLEFT	? BRL_FLG_LINE_TOLEFT	: 0)
    | (code & BRLAPI_KEY_FLG_CONTROL		? BRL_FLG_CHAR_CONTROL	: 0)
    | (code & BRLAPI_KEY_FLG_META		? BRL_FLG_CHAR_META	: 0)
    | (code & BRLAPI_KEY_FLG_UPPER		? BRL_FLG_CHAR_UPPER	: 0)
    | (code & BRLAPI_KEY_FLG_SHIFT		? BRL_FLG_CHAR_SHIFT	: 0)
      ;
    describeCommand(cmd, buf, sizeof(buf));
    brlapi_writeText(0, buf);
    fprintf(stderr, "%s\n", buf);
    if (cmd==BRL_CMD_LEARN) return;
  }
  brlapi_perror("brlapi_readKey");
}

void showKeyCodes(void)
{
  int res;
  brl_keycode_t cmd;
  char buf[0X100];

  fprintf(stderr,"Entering keycode learn mode\n");
  if (brlapi_getDriverName(buf, sizeof(buf))==-1) {
    brlapi_perror("getDriverName");
    return;
  }
  if (brlapi_getTty(-1, buf)<0) {
    brlapi_perror("getTty");
    return;
  }

  if (brlapi_unignoreKeyRange(0, BRL_KEYCODE_MAX)==-1) {
    brlapi_perror("unignoreKeyRange");
    return;
  }

  if (brlapi_writeText(0, "show key codes")<0) {
    brlapi_perror("brlapi_writeText");
    exit(1);
  }

  while ((res = brlapi_readKey(1, &cmd)) != -1) {
    sprintf(buf, "0X%" BRLAPI_PRIxKEYCODE " (%" BRLAPI_PRIuKEYCODE ")",cmd, cmd);
    brlapi_writeText(0, buf);
    fprintf(stderr, "%s\n", buf);
  }
  brlapi_perror("brlapi_readKey");
}

#ifdef SIGUSR1
void emptySignalHandler(int sig) { }
#endif /* SIGUSR1 */

void suspend(void)
{
  char name[30];
  fprintf(stderr, "Getting driver name: ");
  if (brlapi_getDriverName(name, sizeof(name))<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%s\n", name);
  fprintf(stderr, "Suspending\n");
  if (brlapi_suspend(name)) {
    brlapi_perror("suspend");
  } else {
#ifdef SIGUSR1
    signal(SIGUSR1,emptySignalHandler);
#endif /* SIGUSR1 */
    fprintf(stderr, "Sleeping\n");
#ifdef HAVE_PAUSE
    pause();
#endif /* HAVE_PAUSE */
    fprintf(stderr, "Resuming\n");
#ifdef SIGUSR1
    signal(SIGUSR1,SIG_DFL);
#endif /* SIGUSR1 */
    if (brlapi_resume())
      brlapi_perror("resume");
  }
}

int main(int argc, char *argv[])
{
  int status = 0;
  brlapi_fileDescriptor fd;
  settings.hostName = NULL; settings.authKey = NULL;

  processOptions(optionTable, optionCount,
                 "apitest", &argc, &argv,
                 NULL, NULL, NULL,
                 "");

  fprintf(stderr, "Connecting to BrlAPI... ");
  if ((fd=brlapi_initializeConnection(&settings, &settings)) >= 0) {
    fprintf(stderr, "done (fd=%"PRIFD")\n", fd);
    fprintf(stderr,"Connected to %s using key file %s\n", settings.hostName, settings.authKey);

    if (opt_showIdentifier) {
      showDriverIdentifier();
    }

    if (opt_showName) {
      showDriverName();
    }

    if (opt_showSize) {
      showDisplaySize();
    }

    if (opt_showDots) {
      showDots();
    }

    if (opt_learnMode) {
      enterLearnMode();
    }

    if (opt_showKeyCodes) {
      showKeyCodes();
    }

    if (opt_suspendMode) {
      suspend();
    }

    brlapi_closeConnection();
    fprintf(stderr, "Disconnected\n"); 
  } else {
    fprintf(stderr, "failed to connect to %s using key %s",settings.hostName, settings.authKey);
    brlapi_perror("");
    status = 1;
  }
  return status;
}

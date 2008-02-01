/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "scr.h"
#include "cmd.h"
#include "charset.h"

#define BRLAPI_NO_DEPRECATED
#include "brlapi.h"

typedef enum {
  PARM_HOST=0,
  PARM_AUTH=1
} DriverParameter;
#define BRLPARMS "host", "auth"

#include "brl_driver.h"

#define CHECK(cond, label) \
  do { \
    if (!(cond)) { \
      LogPrint(LOG_ERR, "%s", brlapi_strerror(&brlapi_error)); \
      goto label; \
    } \
  } while (0);

static int displaySize;
static unsigned char *prevData;
static wchar_t *prevText;
static int prevCursor;
static int prevShown;

static int restart;

/* Function : brl_construct */
/* Opens a connection with BrlAPI's server */
static int brl_construct(BrailleDisplay *brl, char **parameters, const char *device)
{
  brlapi_connectionSettings_t settings;
  settings.host = parameters[PARM_HOST];
  settings.auth = parameters[PARM_AUTH];
  CHECK((brlapi_openConnection(&settings, &settings)>=0), out);
  LogPrint(LOG_DEBUG, "Connected to %s using %s", settings.host, settings.auth);
  CHECK((brlapi_enterTtyModeWithPath(NULL, 0, NULL)>=0), out0);
  LogPrint(LOG_DEBUG, "Got tty successfully");
  CHECK((brlapi_getDisplaySize(&brl->x, &brl->y)==0), out1);
  LogPrint(LOG_DEBUG,"Found out display size: %dx%d", brl->x, brl->y);
  displaySize = brl->x*brl->y;
  prevData = malloc(displaySize);
  CHECK((prevData!=NULL), out1);
  prevText = malloc(displaySize * sizeof(wchar_t));
  CHECK((prevText!=NULL), out2);
  prevShown = 0;
  restart = 0;
  LogPrint(LOG_DEBUG, "Memory allocated, returning 1");
  return 1;
  
out2:
  free(prevData);
out1:
  brlapi_leaveTtyMode();
out0:
  brlapi_closeConnection();
out:
  LogPrint(LOG_DEBUG, "Something went wrong, returning 0");
  return 0;
}

/* Function : brl_destruct */
/* Frees memory and closes the connection with BrlAPI */
static void brl_destruct(BrailleDisplay *brl)
{
  free(prevData);
  free(prevText);
  brlapi_closeConnection();
}

/* function : brl_writeWindow */
/* Displays a text on the braille window, only if it's different from */
/* the one already displayed */
static int brl_writeWindow(BrailleDisplay *brl, const wchar_t *text)
{
  brlapi_writeArguments_t arguments = BRLAPI_WRITEARGUMENTS_INITIALIZER;
  int vt;
  vt = currentVirtualTerminal();
  if (vt == -1) {
    /* should leave display */
    if (prevShown) {
      brlapi_write(&arguments);
      prevShown = 0;
    }
  } else {
    unsigned char and[displaySize];
    if (prevShown
	&& memcmp(prevData,brl->buffer,displaySize) == 0
	&& (!text || wmemcmp(prevText,text,displaySize) == 0)
	&& brl->cursor == prevCursor)
      return 1;
    memset(and,0,sizeof(and));
    if (text) {
      arguments.text = (char*) text;
      arguments.textSize = displaySize * sizeof(wchar_t);
      arguments.charset = (char*) getWcharCharset();
    }
    arguments.regionBegin = 1;
    arguments.regionSize = displaySize;
    arguments.andMask = and;
    arguments.orMask = brl->buffer;
    arguments.cursor = brl->cursor + 1;
    if (brlapi_write(&arguments)==0) {
      memcpy(prevData,brl->buffer,displaySize);
      wmemcpy(prevText,text,displaySize);
      prevCursor = brl->cursor;
      prevShown = 1;
    } else {
      LogPrint(LOG_ERR, "write: %s", brlapi_strerror(&brlapi_error));
      restart = 1;
    }
  }
  return 1;
}

/* Function : brl_readCommand */
/* Reads a command from the braille keyboard */
static int brl_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext context)
{
  brlapi_keyCode_t keycode;
  if (restart) return BRL_CMD_RESTARTBRL;
  switch (brlapi_readKey(0, &keycode)) {
    case 0: return EOF;
    case 1: return cmdBrlapiToBrltty(keycode);
    default: return BRL_CMD_RESTARTBRL;
  }
}

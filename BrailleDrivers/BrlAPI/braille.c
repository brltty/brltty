/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#define BA_VERSION "BRLTTY driver for BrlAPI, version 0.1, 2005"
#define BA_COPYRIGHT "   Copyright Sebastien HINDERER <Sebastien.Hinderer@ens-lyon.org>, \
Samuel THIBAULT <samuel.thibault@ens-lyon.org>"

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "Programs/misc.h"
#include "Programs/api.h"
#include "Programs/scr.h"

typedef enum {
  PARM_HOSTNAME=0,
  PARM_AUTHKEY=1,
  PARM_TTY=2
} DriverParameter;
#define BRLPARMS "host", "key", "tty"

#define BRL_HAVE_VISUAL_DISPLAY
#include "Programs/brl_driver.h"

#define CHECK(cond, label) \
  do { \
    if (!(cond)<0) { \
      LogPrint(LOG_ERR, "%s", brlapi_strerror(&brlapi_error)); \
      goto label; \
    } \
  } while (0);

static int displaySize;
static unsigned char *prevData;
static unsigned char *prevText;
static int prevCursor;
static int prevShown;

/* Function : brl_identify */
/* Prints information about the driver in the system log and on stderr */
static void brl_identify(void)
{
  LogPrint(LOG_NOTICE, BA_VERSION);
  LogPrint(LOG_INFO, "   " BA_COPYRIGHT);
}

/* Function : brl_open */
/* Opens a connection with BrlAPI's server */
static int brl_open(BrailleDisplay *brl, char **parameters, const char *device)
{
  int ttyMin = -1, tty = -1;
  brlapi_settings_t settings;
  settings.hostName = parameters[PARM_HOSTNAME];
  settings.authKey = parameters[PARM_AUTHKEY];
  if (!validateInteger(&tty, parameters[PARM_TTY], &ttyMin, NULL))
    LogPrint(LOG_WARNING, "%s: %s", "invalid TTY", parameters[PARM_TTY]);
  CHECK((brlapi_initializeConnection(&settings, &settings)>=0), out);
  LogPrint(LOG_DEBUG, "Connected to %s using %s", settings.hostName, settings.authKey);
  CHECK((brlapi_getTty(tty, NULL)>=0), out0);
  LogPrint(LOG_DEBUG, "Got tty successfully");
  CHECK((brlapi_getDisplaySize(&brl->x, &brl->y)==0), out1);
  LogPrint(LOG_DEBUG,"Found out display size: %dx%d", brl->x, brl->y);
  displaySize = brl->x*brl->y;
  prevData = malloc(displaySize);
  CHECK((prevData!=NULL), out1);
  prevText = malloc(displaySize);
  CHECK((prevText!=NULL), out2);
  prevShown = 0;
  LogPrint(LOG_DEBUG, "Memory allocated, returning 1");
  return 1;
  
out2:
  free(prevData);
out1:
  brlapi_leaveTty();
out0:
  brlapi_closeConnection();
out:
  LogPrint(LOG_DEBUG, "Something went wrong, returning 0");
  return 0;
}

/* Function : brl_close */
/* Frees memory and closes the connection with BrlAPI */
static void brl_close(BrailleDisplay *brl)
{
  free(prevData);
  free(prevText);
  brlapi_closeConnection();
}

/* function : brl_writeWindow */
/* Displays a text on the braille window, only if it's different from */
/* the one already displayed */
static void brl_writeWindow(BrailleDisplay *brl)
{
  int vt;
  vt = currentVirtualTerminal();
  if (vt == -1) {
    /* should leave display */
    if (prevShown) {
      brlapi_writeStruct ws = BRLAPI_WRITESTRUCT_INITIALIZER;
      brlapi_write(&ws);
      prevShown = 0;
    }
    return;
  } else {
    brlapi_writeStruct ws = BRLAPI_WRITESTRUCT_INITIALIZER;
    unsigned char and[displaySize];
    if (prevShown && memcmp(prevData,brl->buffer,displaySize)==0) return;
    memset(and,0,sizeof(and));
    ws.attrAnd = and;
    ws.attrOr = brl->buffer;
    if (brlapi_write(&ws)==0) {
      memcpy(prevData,brl->buffer,displaySize);
      prevShown = 1;
    } else LogPrint(LOG_ERR, "write: %s", brlapi_strerror(&brlapi_error));
  }
}

/* function : brl_writeVisual */
/* Displays a text on the braille window, only if it's different from */
/* the one already displayed */
static void brl_writeVisual(BrailleDisplay *brl)
{
  int vt;
  vt = currentVirtualTerminal();
  if (vt == -1) {
    /* should leave display */
    if (prevShown) {
      brlapi_writeStruct ws = BRLAPI_WRITESTRUCT_INITIALIZER;
      brlapi_write(&ws);
      prevShown = 0;
    }
    return;
  } else {
    if (prevShown && memcmp(prevText,brl->buffer,displaySize)==0 && brl->cursor == prevCursor) return;
    if (brlapi_writeText(brl->cursor+1,(char *) brl->buffer)==0) {
      memcpy(prevText,brl->buffer,displaySize);
      prevCursor = brl->cursor;
      prevShown = 1;
    } else LogPrint(LOG_ERR, "write: %s", brlapi_strerror(&brlapi_error));
  }
}

/* Function : brl_writeStatus */
/* Not supported by BrlAPI yet */
static void brl_writeStatus(BrailleDisplay *brl, const unsigned char *s)
{
}

/* Function : brl_readCommand */
/* Reads a command from the braille keyboard */
static int brl_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext context)
{
  brl_keycode_t command;
  if (brlapi_readKey(0, &command)==1) return command;
  return EOF;
}

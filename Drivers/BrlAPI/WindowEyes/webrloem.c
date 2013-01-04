/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#define BRLAPI_NO_SINGLE_SESSION
#define BRLAPI_NO_DEPRECATED

#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include "brlapi.h"

#include "webrloem.h"

#define C(cmd) (BRLAPI_KEY_CMD_##cmd & BRLAPI_KEY_CMD_ARG_MASK)

static const unsigned command_button[] = {
  [ C(LNUP) ] =			1  /* go up one line */,
  [ C(LNDN) ] =			2  /* go down one line */,
  [ C(WINUP) ] =		3  /* go up several lines */,
  [ C(WINDN) ] =		4  /* go down several lines */,
  [ C(PRDIFLN) ] =		5  /* go up to nearest line with different content */,
  [ C(NXDIFLN) ] =		6  /* go down to nearest line with different content */,
  [ C(ATTRUP) ] =		7  /* go up to nearest line with different highlighting */,
  [ C(ATTRDN) ] =		8  /* go down to nearest line with different highlighting */,
  [ C(TOP) ] =			9  /* go to top line */,
  [ C(BOT) ] =			10 /* go to bottom line */,
  [ C(TOP_LEFT) ] =		11 /* go to beginning of top line */,
  [ C(BOT_LEFT) ] =		12 /* go to beginning of bottom line */,
  [ C(PRPGRPH) ] =		13 /* go up to last line of previous paragraph */,
  [ C(NXPGRPH) ] =		14 /* go down to first line of next paragraph */,
  [ C(PRPROMPT) ] =		15 /* go up to previous command prompt */,
  [ C(NXPROMPT) ] =		16 /* go down to next command prompt */,
  [ C(CHRLT) ] =		17 /* go left one character */,
  [ C(CHRRT) ] =		18 /* go right one character */,
  [ C(HWINLT) ] =		19 /* go left half a window */,
  [ C(HWINRT) ] =		20 /* go right half a window */,
  [ C(FWINLT) ] =		21 /* go left one window */,
  [ C(FWINRT) ] =		22 /* go right one window */,
  [ C(FWINLTSKIP) ] =		23 /* go left to nearest non-blank window */,
  [ C(FWINRTSKIP) ] =		24 /* go right to nearest non-blank window */,
  [ C(LNBEG) ] =		25 /* go to beginning of line */,
  [ C(LNEND) ] =		26 /* go to end of line */,
  [ C(HOME) ] =			27 /* go to cursor */,
  [ C(BACK) ] =			28 /* go back (undo unexpected cursor tracking motion) */,
  [ C(FREEZE) ] =		29 /* toggle screen mode frozen/live */,
  [ C(DISPMD) ] =		30 /* toggle display mode attributes/text */,
  [ C(SIXDOTS) ] =		31 /* toggle text style 6-dot/8-dot */,
  [ C(SLIDEWIN) ] =		32 /* toggle sliding window on/off */,
  [ C(SKPIDLNS) ] =		33 /* toggle skipping of lines with identical content on/off */,
  [ C(SKPBLNKWINS) ] =		34 /* toggle skipping of blank windows on/off */,
  [ C(CSRVIS) ] =		35 /* toggle cursor visibility on/off */,
  [ C(CSRTRK) ] =		36 /* toggle cursor tracking on/off */,
  [ C(CSRSIZE) ] =		37 /* toggle cursor style block/underline */,
  [ C(CSRBLINK) ] =		38 /* toggle cursor blinking on/off */,
  [ C(ATTRVIS) ] =		39 /* toggle attribute underlining on/off */,
  [ C(ATTRBLINK) ] =		40 /* toggle attribute blinking on/off */,
  [ C(CAPBLINK) ] =		41 /* toggle capital letter blinking on/off */,
  [ C(TUNES) ] =		42 /* toggle alert tunes on/off */,
  [ C(AUTOREPEAT) ] =		43 /* toggle autorepeat on/off */,
  [ C(AUTOSPEAK) ] =		44 /* toggle autospeak on/off */,
  [ C(HELP) ] =			45 /* enter/leave help display */,
  [ C(INFO) ] =			46 /* enter/leave status display */,
  [ C(LEARN) ] =		47 /* enter/leave command learn mode */,
  [ C(PREFMENU) ] =		48 /* enter/leave preferences menu */,
  [ C(PREFSAVE) ] =		49 /* save current preferences */,
  [ C(PREFLOAD) ] =		50 /* restore saved preferences */,
  [ C(MUTE) ] =			51 /* stop speaking immediately */,
  [ C(SPKHOME) ] =		52 /* go to current (most recent) speech position */,
  [ C(SAY_LINE) ] =		53 /* speak current line */,
  [ C(SAY_ABOVE) ] =		54 /* speak from top of screen through current line */,
  [ C(SAY_BELOW) ] =		55 /* speak from current line through bottom of screen */,
  [ C(SAY_SLOWER) ] =		56 /* decrease speech rate */,
  [ C(SAY_FASTER) ] =		57 /* increase speech rate */,
  [ C(SAY_SOFTER) ] =		58 /* decrease speech volume */,
  [ C(SAY_LOUDER) ] =		59 /* increase speech volume */,
  [ C(SWITCHVT_PREV) ] =	60 /* switch to previous virtual terminal */,
  [ C(SWITCHVT_NEXT) ] =	61 /* switch to next virtual terminal */,
  [ C(CSRJMP_VERT) ] =		62 /* bring cursor to line (no horizontal motion) */,
  [ C(RESTARTBRL) ] =		63 /* reinitialize braille driver */,
  [ C(RESTARTSPEECH) ] =	64 /* reinitialize speech driver */,
};

int raw;

bool WEBrailleOpen(int portType, int portNumber, DWORD *handle)
{
  brlapi_handle_t *brl_handle = malloc(brlapi_getHandleSize());
  brlapi_range_t ranges[] = {
    {
      .first = BRLAPI_KEY_TYPE_CMD | BRLAPI_KEY_CMD_ROUTE,
      .last = BRLAPI_KEY_TYPE_CMD | BRLAPI_KEY_CMD_ROUTE | BRLAPI_KEY_CMD_ARG_MASK,
    }
  };
  int i,j;
  brlapi_range_t *cmd_ranges;

  if (brlapi__openConnection(brl_handle, NULL, NULL) == INVALID_HANDLE_VALUE)
    goto error;

  raw = portNumber == 2;
  
  if (raw) {
    char name[BRLAPI_MAXNAMELENGTH+1];
    if (brlapi__getDriverName(brl_handle, name, sizeof(name)) == -1)
      goto error;
    if (brlapi__enterTtyModeWithPath(brl_handle, NULL, 0, name) == -1)
      goto error;
  } else {
    if (brlapi__enterTtyModeWithPath(brl_handle, NULL, 0, NULL) == -1)
      goto error;
  }
  if (brlapi__ignoreAllKeys(brl_handle) == -1)
    goto error;
  if (brlapi__acceptKeyRanges(brl_handle, ranges, sizeof(ranges)/sizeof(ranges[0])) == -1)
    goto error;

  cmd_ranges = malloc(64 * sizeof(cmd_ranges[0]));
  for (i = 0, j = 0; i < sizeof(command_button)/sizeof(command_button[0]); i++) {
    if (command_button[i]) {
      cmd_ranges[j].first = cmd_ranges[j].last = BRLAPI_KEY_TYPE_CMD | (BRLAPI_KEY_CMD(0) + command_button[i]);
      j++;
    }
  }
  if (brlapi__acceptKeyRanges(brl_handle, cmd_ranges, j) == -1)
    goto error;
  free(cmd_ranges);

  *handle = (DWORD) brl_handle;
  return TRUE;

error:
  free(brl_handle);
  return FALSE;
}

bool WEBrailleClose(DWORD handle)
{
  brlapi_handle_t *brl_handle = (void*) handle;
  brlapi__closeConnection(brl_handle);
  return TRUE;
}

bool WEGetBrailleDisplayInfo(DWORD handle, int *numberOfCells, int *numberOfStatusCells)
{
  brlapi_handle_t *brl_handle = (void*) handle;
  unsigned int x, y;
  
  if (brlapi__getDisplaySize(brl_handle, &x, &y) == -1)
    return FALSE;

  *numberOfCells = x * y;
  *numberOfStatusCells = 0;
  return TRUE;
}

static bool keysent = FALSE;

bool WEGetBrailleKey(DWORD handle, DWORD *key1, DWORD *key2, int *routingKey, int *doubleRoutingKey, bool *routingOverStatusCell)
{
  brlapi_handle_t *brl_handle = (void*) handle;
  brlapi_keyCode_t code;
  unsigned cmd;

  *key1 = 0;
  *key2 = 0;
  *routingKey = 0;
  *doubleRoutingKey = 0;
  *routingOverStatusCell = FALSE;

  if (keysent) {
    keysent = FALSE;
    return TRUE;
  }

  if (brlapi__readKey(brl_handle, 0, &code) != 1)
    return FALSE;

  if (raw) {
    keysent = TRUE;
    *key1 = code & 0xffffffffu;
    *key2 = code >> 32;
    return TRUE;
  }

  if ((code & BRLAPI_KEY_TYPE_MASK) != BRLAPI_KEY_TYPE_CMD)
    return FALSE;

  cmd = code & BRLAPI_KEY_CMD_BLK_MASK;

  if (cmd == BRLAPI_KEY_CMD_ROUTE) {
    *routingKey = (code & BRLAPI_KEY_CMD_ARG_MASK) + 1;
  } else {
    unsigned button;
    unsigned arg = code & BRLAPI_KEY_CMD_ARG_MASK;

    if (arg >= sizeof(command_button)/sizeof(command_button[0]))
      return FALSE;

    button = command_button[arg];
    if ((button > 64) || (button == 0))
      return FALSE;
    if (button > 32)
      *key2 = 1 << (button - 1 - 32);
    else
      *key1 = 1 << (button - 1);
  }

  keysent = TRUE;
  return TRUE;
}

bool WEUpdateBrailleDisplay(DWORD handle, BYTE *pMainCells, int mainCellsCount, BYTE *pStatusCells, int statusCellsCount)
{
  brlapi_handle_t *brl_handle = (void*) handle;
  int numCells, numStatCells;

  if (!WEGetBrailleDisplayInfo(handle, &numCells, &numStatCells))
    return FALSE;

  {
    unsigned char dots[numCells];
    memcpy(dots, pMainCells, mainCellsCount);
    memset(dots + mainCellsCount, 0, numCells - mainCellsCount);
    if (brlapi__writeDots(brl_handle, dots) == -1)
      return FALSE;
  }

  return TRUE;
}

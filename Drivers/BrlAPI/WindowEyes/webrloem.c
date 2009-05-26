/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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
#include "brlapi.h"

bool __stdcall WEBrailleOpen(int portType, int portNumber, DWORD *handle)
{
  brlapi_handle_t *brl_handle = malloc(sizeof(*handle));
  brlapi_range_t ranges[] = {
    {
      .first = BRLAPI_KEY_TYPE_CMD | BRLAPI_KEY_CMD(1),
      .last = BRLAPI_KEY_TYPE_CMD | BRLAPI_KEY_CMD(64),
    },
    {
      .first = BRLAPI_KEY_TYPE_CMD | BRLAPI_KEY_CMD_ROUTE,
      .last = BRLAPI_KEY_TYPE_CMD | BRLAPI_KEY_CMD_ROUTE | BRLAPI_KEY_CMD_ARG_MASK,
    }
  };

  if (brlapi__openConnection(brl_handle, NULL, NULL) == -1)
    goto error;
  if (brlapi__enterTtyModeWithPath(brl_handle, NULL, 0, NULL) == -1)
    goto error;
  if (brlapi__ignoreAllKeys(brl_handle) == -1)
    goto error;
  if (brlapi__acceptKeyRanges(brl_handle, ranges, sizeof(ranges)/sizeof(ranges[0])) == -1)
    goto error;

  *handle = (DWORD) brl_handle;
  return TRUE;

error:
  free(brl_handle);
  return FALSE;
}

bool __stdcall WEBrailleClose(DWORD handle)
{
  brlapi_handle_t *brl_handle = (void*) handle;
  brlapi__closeConnection(brl_handle);
  return TRUE;
}

bool __stdcall WEGetBrailleDisplayInfo(DWORD handle, int *numberOfCells, int *numberOfStatusCells)
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

bool __stdcall WEGetBrailleKey(DWORD handle, DWORD *key1, DWORD *key2, int *routingKey, int *doubleRoutingKey, bool *routingOverStatusCell)
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

  if ((code & BRLAPI_KEY_TYPE_MASK) != BRLAPI_KEY_TYPE_CMD)
    return FALSE;

  cmd = code & BRLAPI_KEY_CMD_BLK_MASK;

  if (cmd == BRLAPI_KEY_CMD_ROUTE) {
    *routingKey = (code & BRLAPI_KEY_CMD_ARG_MASK) + 1;
  } else {
    /* TODO: only 64 keys, maybe better choose them? */
    if (cmd > 64)
      return FALSE;
    if (cmd > 32)
      *key2 = 1 << (cmd - 1 - 32);
    else
      *key1 = 1 << (cmd - 1);
  }

  keysent = TRUE;
  return TRUE;
}

bool __stdcall WEUpdateBrailleDisplay(DWORD handle, BYTE *pMainCells, int mainCellsCount, BYTE *pStatusCells, int statusCellsCount)
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

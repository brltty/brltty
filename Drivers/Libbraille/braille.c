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

/* Libbraille/braille.c - Braille display driver using libbraille
 *
 * Written by Sébastien Sablé <sable@users.sourceforge.net>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "Programs/misc.h"

#include <braille.h>

#define BRL_HAVE_VISUAL_DISPLAY
#include "Programs/brl_driver.h"
#include "Programs/tbl.h"

static TranslationTable outputTable;
static TranslationTable inputTable;

static void
brl_identify(void)
{
  LogPrint(LOG_NOTICE, "BRLTTY wrapper for Libbraille");
  LogPrint(LOG_INFO, "   Copyright (C) 2004 by Sébastien Sablé <sable@users.sourceforge.net>");
}

static int
brl_open(BrailleDisplay *brl, char **parameters, const char *device)
{
  {
    static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
    makeOutputTable(&dots, &outputTable);
    reverseTranslationTable(&outputTable, &inputTable);
  }
  
  if(braille_init())
    {
      brl->x = braille_size();  /* initialise size of display */
      brl->y = 1;
      brl->helpPage = 0;

      braille_timeout(100);

      return 1;
    }
  
  return 0;
}

static void
brl_close(BrailleDisplay *brl)
{
  braille_close();
}

static void
brl_writeVisual(BrailleDisplay *brl)
{
  braille_write(brl->buffer, brl->x);
}

static void
brl_writeWindow(BrailleDisplay *brl)
{
  int i;

  for(i = 0; i < brl->x; i++)
    {
      braille_filter(outputTable[brl->buffer[i]], i);
    }
  braille_render();
}

static void
brl_writeStatus(BrailleDisplay *brl, const unsigned char *status)
{
}

static int
brl_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext context)
{
  int res = EOF;
  signed char status;
  brl_key key;

  status = braille_read(&key);
  if(status == -1)
    {
      LogPrint(LOG_ERR, "error in braille_read: %s", braille_geterror());
    }
  else if(status)
    {
      switch(key.type)
	{
	case BRL_NONE:
	  break;
	case BRL_CURSOR:
	  res = BRL_BLK_ROUTE + key.code;
	  break;
	case BRL_CMD:
	  switch(key.code)
	    {
	    case BRLK_UP:
	      res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP;
	      break;
	    case BRLK_DOWN:
	      res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN;
	      break;
	    case BRLK_RIGHT:
	      res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT;
	      break;
	    case BRLK_LEFT:
	      res = BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT;
	      break;
	    case BRLK_INSERT:
	      res = BRL_BLK_PASSKEY + BRL_KEY_INSERT;
	      break;
	    case BRLK_HOME:
	      res = BRL_BLK_PASSKEY + BRL_KEY_HOME;
	      break;
	    case BRLK_END:
	      res = BRL_BLK_PASSKEY + BRL_KEY_END;
	      break;
	    case BRLK_PAGEUP:
	      res = BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP;
	      break;
	    case BRLK_PAGEDOWN:
	      res = BRL_BLK_PASSKEY + BRL_KEY_PAGE_DOWN;
	      break;
	    case BRLK_BACKWARD:
	      res = BRL_CMD_FWINLT;
	      break;
	    case BRLK_FORWARD:
	      res = BRL_CMD_FWINRT;
	      break;
	    default:
	      break;
	    }
	  break;
	case BRL_KEY:
	  res = BRL_BLK_PASSDOTS | inputTable[key.braille];
	  break;
	default:
          break;
	}
    }

  return res;
}

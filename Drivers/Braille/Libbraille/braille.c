/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "brltty.h"

#include <braille.h>

typedef enum {
  PARM_DEVICE,
  PARM_DRIVER,
  PARM_TABLE
} DriverParameter;
#define BRLPARMS "device", "driver", "table"

#include "brl_driver.h"

static TranslationTable outputTable;
static TranslationTable inputTable;

static int
brl_construct(BrailleDisplay *brl, char **parameters, const char *device)
{
  {
    static const DotsTable dots = {
      BRAILLE(1, 0, 0, 0, 0, 0, 0, 0),
      BRAILLE(0, 1, 0, 0, 0, 0, 0, 0),
      BRAILLE(0, 0, 1, 0, 0, 0, 0, 0),
      BRAILLE(0, 0, 0, 1, 0, 0, 0, 0),
      BRAILLE(0, 0, 0, 0, 1, 0, 0, 0),
      BRAILLE(0, 0, 0, 0, 0, 1, 0, 0),
      BRAILLE(0, 0, 0, 0, 0, 0, 1, 0),
      BRAILLE(0, 0, 0, 0, 0, 0, 0, 1)
    };
    makeOutputTable(dots, outputTable);
    reverseTranslationTable(outputTable, inputTable);
  }
  
  if(*parameters[PARM_DEVICE])
    braille_config(BRL_DEVICE, parameters[PARM_DEVICE]);

  if(*parameters[PARM_DRIVER])
    braille_config(BRL_DRIVER, parameters[PARM_DRIVER]);

  if(*parameters[PARM_TABLE])
    braille_config(BRL_TABLE, parameters[PARM_TABLE]);

  if(braille_init())
    {
      LogPrint(LOG_INFO, "Libbraille Version: %s", braille_info(BRL_VERSION));

#ifdef BRL_PATH
      LogPrint(LOG_DEBUG, "Libbraille Installation Directory: %s", braille_info(BRL_PATH));
#endif /* BRL_PATH */

#ifdef BRL_PATHCONF
      LogPrint(LOG_DEBUG, "Libbraille Configuration Directory: %s", braille_info(BRL_PATHCONF));
#endif /* BRL_PATHCONF */

#ifdef BRL_PATHTBL
      LogPrint(LOG_DEBUG, "Libbraille Tables Directory: %s", braille_info(BRL_PATHTBL));
#endif /* BRL_PATHTBL */

#ifdef BRL_PATHDRV
      LogPrint(LOG_DEBUG, "Libbraille Drivers Directory: %s", braille_info(BRL_PATHDRV));
#endif /* BRL_PATHDRV */

      LogPrint(LOG_INFO, "Libbraille Table: %s", braille_info(BRL_TABLE));
      LogPrint(LOG_INFO, "Libbraille Driver: %s", braille_info(BRL_DRIVER));
      LogPrint(LOG_INFO, "Libbraille Device: %s", braille_info(BRL_DEVICE));

      LogPrint(LOG_INFO, "Display Type: %s", braille_info(BRL_TERMINAL));
      LogPrint(LOG_INFO, "Display Size: %d", braille_size());

      brl->textColumns = braille_size();  /* initialise size of display */
      brl->textRows = 1;
      brl->helpPage = 0;

      braille_timeout(100);

      return 1;
    }
  else
    {
      LogPrint(LOG_DEBUG, "Libbraille initialization erorr: %s", braille_geterror());
    }
  
  return 0;
}

static void
brl_destruct(BrailleDisplay *brl)
{
  braille_close();
}

static int
brl_writeWindow(BrailleDisplay *brl, const wchar_t *text)
{
  if(text)
    {
      char bytes[brl->textColumns];
      int i;

      for(i = 0; i < brl->textColumns; ++i)
        {
          wchar_t character = text[i];
          bytes[i] = iswLatin1(character)? character: '?';
        }
      braille_write(bytes, brl->textColumns);

      if(brl->cursor >= 0)
        {
          braille_filter(outputTable[cursorDots()], brl->cursor);
        }

      braille_render();
    }

  return 1;
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
      res = BRL_CMD_RESTARTBRL;
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
	    case BRLK_ABOVE:
	      res = BRL_CMD_LNUP;
	      break;
	    case BRLK_BELOW:
	      res = BRL_CMD_LNDN;
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

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

/* BrailleNote/braille.c - Braille display library
 * For Pulse Data International's Braille Note series
 * Author: Dave Mielke <dave@mielke.cc>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef HAVE_LINUX_VT_H
#include <linux/vt.h>
#endif /* HAVE_LINUX_VT_H */
static int displayDescriptor = -1;
static int displayTerminal;

#include "Programs/misc.h"

typedef enum {
   PARM_STATUSCELLS
} DriverParameter;
#define BRLPARMS "statuscells"

#include "Programs/brl_driver.h"
#include "Programs/tbl.h"
#include "braille.h"
#include "Programs/serial.h"

static int fileDescriptor = -1;
static struct termios oldSettings;
static struct termios newSettings;

static unsigned char *cellBuffer = NULL;
static unsigned int cellCount = 0;
static unsigned char *statusArea;
static unsigned int statusCells;
static unsigned char *dataArea;
static unsigned int dataCells;
static unsigned char *outputBuffer = NULL;

typedef enum {
   KBM_INPUT,
   KBM_INPUT_7,
   KBM_INPUT_78,
   KBM_INPUT_8,
   KBM_NAVIGATE
} KeyboardMode;
static KeyboardMode persistentKeyboardMode;
static KeyboardMode temporaryKeyboardMode;
static KeyboardMode currentKeyboardMode;

static unsigned int persistentRoutingOperation;
static unsigned int temporaryRoutingOperation;
static unsigned int currentRoutingOperation;

static TranslationTable outputTable;
static TranslationTable inputTable;

static void
refreshCells (void) {
   unsigned char *output = outputBuffer;
   int i;

   *output++ = BNO_BEGIN;
   *output++ = BNO_WRITE;

   for (i=0; i<cellCount; ++i) {
      if ((*output++ = outputTable[cellBuffer[i]]) == BNO_BEGIN) {
         *output++ = BNO_BEGIN;
      }
   }

   if (safe_write(fileDescriptor, outputBuffer, output-outputBuffer) == -1) {
      LogError("BrailleNote write");
   }
}

static void
writePrompt (const unsigned char *prompt) {
   unsigned int length = strlen(prompt);
   unsigned int index = 0;
   if (length > dataCells) {
      length = dataCells;
   }
   while (index < length) {
      dataArea[index] = textTable[prompt[index]];
      ++index;
   }
   while (index < dataCells) {
      dataArea[index++] = 0;
   }
   refreshCells();
}

static unsigned char
getByte (void) {
   unsigned char byte;
   fd_set mask;
   FD_ZERO(&mask);
   FD_SET(fileDescriptor, &mask);
   select(fileDescriptor+1, &mask, NULL, NULL, NULL);
   read(fileDescriptor, &byte, 1);
   return byte;
}

static unsigned char
getCharacter (void) {
   for (;;) {
      switch (getByte()) {
         default:
	    break;
	 case BNI_CHARACTER:
	    return untextTable[inputTable[getByte()]];
	 case BNI_SPACE:
	    switch (getByte()) {
	       default:
	          break;
	       case BNC_SPACE:
		  return ' ';
	    }
	    break;
	 case BNI_BACKSPACE:
	    switch (getByte() & 0X3F) {
	       default:
	          break;
	       case BNC_SPACE:
		  return '\b';
	    }
	    break;
	 case BNI_ENTER:
	    switch (getByte()) {
	       default:
	          break;
	       case BNC_SPACE:
		  return '\r';
	    }
	    break;
      }
      refreshCells();
   }
}

static int
getVirtualTerminal (void) {
   int vt = -1;
#ifdef HAVE_LINUX_VT_H
   int consoleDescriptor = getConsole();
   if (consoleDescriptor != -1) {
      struct vt_stat state;
      if (ioctl(consoleDescriptor, VT_GETSTATE, &state) != -1) {
         vt = state.v_active;
      }
   }
#endif /* HAVE_LINUX_VT_H */
   return vt;
}

static void
setVirtualTerminal (int vt) {
#ifdef HAVE_LINUX_VT_H
   int consoleDescriptor = getConsole();
   if (consoleDescriptor != -1) {
      LogPrint(LOG_DEBUG, "switching to virtual terminal %d", vt);
      if (ioctl(consoleDescriptor, VT_ACTIVATE, vt) != -1) {
	 if (ioctl(consoleDescriptor, VT_WAITACTIVE, vt) != -1) {
	    LogPrint(LOG_INFO, "switched to virtual terminal %d", vt);
	 } else {
	    LogError("virtual console wait");
	 }
      } else {
         LogError("virtual console activate");
      }
   }
#endif /* HAVE_LINUX_VT_H */
}

static void
openVisualDisplay (void) {
#ifdef HAVE_LINUX_VT_H
   if (displayDescriptor == -1) {
      int consoleDescriptor = getConsole();
      if (consoleDescriptor != -1) {
	 if (ioctl(consoleDescriptor, VT_OPENQRY, &displayTerminal) != -1) {
	    char path[0X20];
	    snprintf(path, sizeof(path), "/dev/tty%d", displayTerminal);
	    if ((displayDescriptor = open(path, O_WRONLY)) != -1) {
	       LogPrint(LOG_INFO, "visual display is %s", path);
	    }
	 }
      }
   }
   if (displayDescriptor != -1) {
      setVirtualTerminal(displayTerminal);
   }
#endif /* HAVE_LINUX_VT_H */
}

static void
closeVisualDisplay (int vt) {
   if (displayDescriptor != -1) {
      if (getVirtualTerminal() == displayTerminal) {
         setVirtualTerminal(vt);
      }
      close(displayDescriptor);
      displayDescriptor = -1;
      displayTerminal = 0;
   }
}

static void
writeVisualDisplay (unsigned char c) {
   if (displayDescriptor != -1) {
      write(displayDescriptor, &c, 1);
   }
}

static int
visualDisplay (unsigned char character, BRL_DriverCommandContext context) {
   int vt = getVirtualTerminal();
   const unsigned char end[] = {0X1B, 0};
   unsigned int state = 0;
   openVisualDisplay();
   writeVisualDisplay(BNI_DISPLAY);
   writeVisualDisplay(character);
   for (;;) {
      character = getByte();
      if (character == end[state]) {
	 if (++state == sizeof(end)) {
	    break;
	 }
      } else {
	 if (state > 0) {
	    int i;
	    for (i=0; i<state; ++i) {
	       writeVisualDisplay(end[i]);
	    }
	    state = 0;
	 }
	 if (character == end[0]) {
	    state = 1;
	 } else {
	    writeVisualDisplay(character);
	 }
      }
   }
   closeVisualDisplay(vt);
   return EOF;
}

static void
brl_identify (void) {
   LogPrint(LOG_NOTICE, "BrailleNote Driver, version 1.0");
   LogPrint(LOG_INFO, "   Copyright (C) 2001 by Dave Mielke <dave@mielke.cc>");
}

static void
adjustStatusCells (BrailleDisplay *brl, const char *parameter) {
   if (*parameter) {
      const int minimum = 0;
      const int maximum = MIN(BRL_MAX_STATUS_CELL_COUNT, brl->x-1);
      if (validateInteger(&statusCells, "status cell count", parameter, &minimum, &maximum)) {
         statusArea = dataArea;
	 dataArea = statusArea + statusCells;
	 brl->x -= statusCells;
	 dataCells -= statusCells;
      }
   }
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
   {
      static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
      makeOutputTable(&dots, &outputTable);
      reverseTranslationTable(&outputTable, &inputTable);
   }

   if (!isSerialDevice(&device)) {
      unsupportedDevice(device);
      return 0;
   }

   if (openSerialDevice(device, &fileDescriptor, &oldSettings)) {
      initializeSerialAttributes(&newSettings);
      while (restartSerialDevice(fileDescriptor, &newSettings, 38400)) {
         unsigned char request[] = {BNO_BEGIN, BNO_DESCRIBE};
         if (safe_write(fileDescriptor, request, sizeof(request)) != -1) {
            unsigned char response[3];
            int offset = 0;
            if (readChunk(fileDescriptor, response, &offset, sizeof(response), 1000, 100)) {
               if (response[0] == BNI_DESCRIBE) {
                  statusCells = response[1];
                  brl->x = response[2];
                  brl->y = 1;
                  if ((statusCells == 5) && (brl->x == 30)) {
                     statusCells -= 2;
                     brl->x += 2;
                  }
                  dataCells = brl->x * brl->y;
                  cellCount = statusCells + dataCells;
                  if ((cellBuffer = malloc(cellCount))) {
                     memset(cellBuffer, 0, cellCount);
                     statusArea = cellBuffer;
                     dataArea = statusArea + statusCells;
                     if ((outputBuffer = malloc(2 + (cellCount * 2)))) {
                        refreshCells();
                        persistentKeyboardMode = KBM_NAVIGATE;
                        temporaryKeyboardMode = persistentKeyboardMode;
                        persistentRoutingOperation = BRL_BLK_ROUTE;
                        temporaryRoutingOperation = persistentRoutingOperation;
                        adjustStatusCells(brl, parameters[PARM_STATUSCELLS]);
                        return 1;
                     } else {
                        LogError("Output buffer allocation");
                     }
                     free(cellBuffer);
                  } else {
                     LogError("Cell buffer allocation");
                  }
               } else {
                  LogPrint(LOG_ERR, "Unexpected BrailleNote description: %02X %02X %02X",
                           response[0], response[1], response[2]);
               }
            }
         } else {
            LogError("Write");
         }
         delay(1000);
      }
      tcsetattr(fileDescriptor, TCSANOW, &oldSettings);
      close(fileDescriptor);
      fileDescriptor = -1;
   }
   return 0;
}

static void
brl_close (BrailleDisplay *brl) {
   tcsetattr(fileDescriptor, TCSADRAIN, &oldSettings);
   close(fileDescriptor);
   fileDescriptor = -1;

   free(outputBuffer);
   outputBuffer = NULL;

   free(cellBuffer);
   cellBuffer = NULL;
}

static void
brl_writeWindow (BrailleDisplay *brl) {
   if (memcmp(dataArea, brl->buffer, dataCells) != 0) {
      memcpy(dataArea, brl->buffer, dataCells);
      refreshCells();
   }
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
   if (memcmp(statusArea, status, statusCells) != 0) {
      memcpy(statusArea, status, statusCells);
      /* refreshCells();
       * Not needed since writebrl will be called shortly.
       */
   }
}

static int
getDecimalInteger (unsigned int *integer, unsigned int width, const char *description) {
   char buffer[width + 1];
   memset(buffer, '0', width);
   buffer[width] = 0;
   for (;;) {
      unsigned char character;
      char prompt[0X40];
      snprintf(prompt, sizeof(prompt), "%s: %s", description, buffer);
      writePrompt(prompt);
      switch (character = getCharacter()) {
         default:
	    continue;
	 case '\r':
	    *integer = atoi(buffer);
	    return 1;
	 case '\b':
	    return 0;
         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
	    memcpy(buffer, buffer+1, width-1);
	    buffer[width-1] = character;
	    break;
      }
   }
}

static int
getHexadecimalCharacter (unsigned char *character) {
   *character = 0X00;
   for (;;) {
      unsigned char digit;
      char prompt[0X40];
      snprintf(prompt, sizeof(prompt), "hex char: %2.2x %c", *character, *character);
      writePrompt(prompt);
      switch (getCharacter()) {
         default:
	    continue;
	 case '\r':
	    return 1;
	 case '\b':
	    return 0;
         case '0':
	    digit = 0X0;
	    break;
         case '1':
	    digit = 0X1;
	    break;
         case '2':
	    digit = 0X2;
	    break;
         case '3':
	    digit = 0X3;
	    break;
         case '4':
	    digit = 0X4;
	    break;
         case '5':
	    digit = 0X5;
	    break;
         case '6':
	    digit = 0X6;
	    break;
         case '7':
	    digit = 0X7;
	    break;
         case '8':
	    digit = 0X8;
	    break;
         case '9':
	    digit = 0X9;
	    break;
         case 'a':
	    digit = 0XA;
	    break;
         case 'b':
	    digit = 0XB;
	    break;
         case 'c':
	    digit = 0XC;
	    break;
         case 'd':
	    digit = 0XD;
	    break;
         case 'e':
	    digit = 0XE;
	    break;
         case 'f':
	    digit = 0XF;
	    break;
      }
      *character = (*character << 4) | digit;
   }
}

static int
getFunctionKey (void) {
   unsigned int keyNumber;
   if (getDecimalInteger(&keyNumber, 2, "function key")) {
      if (!keyNumber) keyNumber = 0X100 - BRL_KEY_FUNCTION;
      return BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + (keyNumber - 1);
   }
   return EOF;
}

static int
interpretNavigation (unsigned char dots, BRL_DriverCommandContext context) {
   switch (dots) {
      default:
         break;
      case BND_0:
         return BRL_CMD_HOME;
      case (BND_1):
	 return BRL_CMD_CHRLT;
      case (BND_1 | BND_2):
	 return BRL_CMD_HWINLT;
      case (BND_2):
	 return BRL_CMD_FWINLT;
      case (BND_2 | BND_3):
	 return BRL_CMD_FWINLTSKIP;
      case (BND_3):
	 return BRL_CMD_LNBEG;
      case (BND_1 | BND_3):
	 return BRL_CMD_LNUP;
      case (BND_1 | BND_2 | BND_3):
	 return BRL_CMD_TOP_LEFT;
      case (BND_4):
	 return BRL_CMD_CHRRT;
      case (BND_4 | BND_5):
	 return BRL_CMD_HWINRT;
      case (BND_5):
	 return BRL_CMD_FWINRT;
      case (BND_5 | BND_6):
	 return BRL_CMD_FWINRTSKIP;
      case (BND_6):
	 return BRL_CMD_LNEND;
      case (BND_4 | BND_6):
	 return BRL_CMD_LNDN;
      case (BND_4 | BND_5 | BND_6):
	 return BRL_CMD_BOT_LEFT;
      case (BND_1 | BND_4):
	 return BRL_CMD_TOP;
      case (BND_2 | BND_5):
	 return BRL_CMD_HOME;
      case (BND_3 | BND_6):
	 return BRL_CMD_BOT;
      case (BND_1 | BND_4 | BND_5):
	 return BRL_CMD_PRDIFLN;
      case (BND_2 | BND_5 | BND_6):
	 return BRL_CMD_NXDIFLN;
      case (BND_1 | BND_2 | BND_4):
	 return BRL_CMD_PRSEARCH;
      case (BND_2 | BND_3 | BND_5):
	 return BRL_CMD_NXSEARCH;
      case (BND_1 | BND_2 | BND_5):
	 return BRL_CMD_ATTRUP;
      case (BND_2 | BND_3 | BND_6):
	 return BRL_CMD_ATTRDN;
      case (BND_2 | BND_4):
	 temporaryRoutingOperation = BRL_BLK_PRINDENT;
	 return BRL_CMD_NOOP;
      case (BND_3 | BND_5):
	 temporaryRoutingOperation = BRL_BLK_NXINDENT;
	 return BRL_CMD_NOOP;
      case (BND_2 | BND_4 | BND_5):
	 return BRL_CMD_WINUP;
      case (BND_3 | BND_5 | BND_6):
	 return BRL_CMD_WINDN;
   }
   return EOF;
}

static int
interpretCharacter (unsigned char dots, BRL_DriverCommandContext context) {
   int mask = 0X00;
   if (context != BRL_CTX_SCREEN) {
      return interpretNavigation(dots, context);
   }
   switch (currentKeyboardMode) {
      case KBM_NAVIGATE:
         return interpretNavigation(dots, context);
      case KBM_INPUT:
	 break;
      case KBM_INPUT_7:
	 mask |= 0X40;
	 break;
      case KBM_INPUT_78:
	 mask |= 0XC0;
	 break;
      case KBM_INPUT_8:
	 mask |= 0X80;
	 break;
   }
   return BRL_BLK_PASSDOTS + (inputTable[dots] | mask);
}

static int
interpretSpaceChord (unsigned char dots, BRL_DriverCommandContext context) {
   switch (dots) {
      default:
      /* These are overridden by the Braille Note itself. */
      case BNC_E: /* exit current operation */
      case BNC_H: /* help for current operation */
      case BNC_O: /* go to options menu */
      case BNC_R: /* repeat current prompt */
      case BNC_U: /* uppercase for computer braille */
      case BNC_Z: /* exit current operation */
      case BNC_PERCENT: /* acknowledge alarm */
      case BNC_6: /* go to task menu */
      case (BND_1 | BND_2 | BND_3 | BND_4 | BND_5 | BND_6): /* go to main menu */
	 break;
      case BNC_SPACE:
	 return interpretCharacter(dots, context);
      case BNC_C:
	 return BRL_CMD_PREFMENU;
      case BNC_D:
	 return BRL_CMD_PREFLOAD;
      case BNC_F:
	 return getFunctionKey();
      case BNC_L:
	 temporaryRoutingOperation = BRL_BLK_SETLEFT;
	 return BRL_CMD_NOOP;
      case BNC_M:
	 return BRL_CMD_MUTE;
      case BNC_N:
         persistentKeyboardMode = KBM_NAVIGATE;
	 temporaryKeyboardMode = persistentKeyboardMode;
	 return BRL_CMD_NOOP;
      case BNC_P:
	 return BRL_CMD_PASTE;
      case BNC_S:
	 return BRL_CMD_SAY_LINE;
      case BNC_V: {
	 unsigned int vt;
	 if (getDecimalInteger(&vt, 2, "virt term num")) {
	    if (!vt) vt = 0X100;
	    return BRL_BLK_SWITCHVT + (vt - 1);
	 }
         return EOF;
      }
      case BNC_W:
	 return BRL_CMD_PREFSAVE;
      case BNC_X: {
	 unsigned char character;
	 if (!getHexadecimalCharacter(&character)) {
	    return EOF;
	 }
	 return BRL_BLK_PASSCHAR + character;
      }
      case BNC_LPAREN:
	 temporaryRoutingOperation = BRL_BLK_CUTBEGIN;
	 return BRL_CMD_NOOP;
      case BNC_LBRACE:
	 temporaryRoutingOperation = BRL_BLK_CUTAPPEND;
	 return BRL_CMD_NOOP;
      case BNC_RPAREN:
	 temporaryRoutingOperation = BRL_BLK_CUTRECT;
	 return BRL_CMD_NOOP;
      case BNC_RBRACE:
	 temporaryRoutingOperation = BRL_BLK_CUTLINE;
	 return BRL_CMD_NOOP;
      case BNC_BAR:
         return BRL_CMD_CSRJMP_VERT;
      case BNC_QUESTION:
         return BRL_CMD_LEARN;
      case (BND_2 | BND_3 | BND_5 | BND_6):
	 return BRL_BLK_PASSKEY + BRL_KEY_TAB;
      case (BND_2 | BND_3):
	 return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT;
      case (BND_5 | BND_6):
	 return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT;
      case (BND_2 | BND_5):
	 return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP;
      case (BND_3 | BND_6):
	 return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN;
      case (BND_2):
	 return BRL_BLK_PASSKEY + BRL_KEY_HOME;
      case (BND_3):
	 return BRL_BLK_PASSKEY + BRL_KEY_END;
      case (BND_5):
	 return BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP;
      case (BND_6):
	 return BRL_BLK_PASSKEY + BRL_KEY_PAGE_DOWN;
      case (BND_3 | BND_5):
	 return BRL_BLK_PASSKEY + BRL_KEY_INSERT;
      case (BND_2 | BND_5 | BND_6):
	 return BRL_BLK_PASSKEY + BRL_KEY_DELETE;
      case (BND_2 | BND_6):
	 return BRL_BLK_PASSKEY + BRL_KEY_ESCAPE;
      case (BND_4):
      case (BND_4 | BND_5):
         temporaryKeyboardMode = KBM_INPUT;
	 goto kbmFinish;
      case (BND_4 | BND_3):
      case (BND_4 | BND_5 | BND_3):
         temporaryKeyboardMode = KBM_INPUT_7;
	 goto kbmFinish;
      case (BND_4 | BND_3 | BND_6):
      case (BND_4 | BND_5 | BND_3 | BND_6):
         temporaryKeyboardMode = KBM_INPUT_78;
	 goto kbmFinish;
      case (BND_4 | BND_6):
      case (BND_4 | BND_5 | BND_6):
         temporaryKeyboardMode = KBM_INPUT_8;
      kbmFinish:
	 if (dots & BND_5) {
	    persistentKeyboardMode = temporaryKeyboardMode;
	 }
	 return BRL_CMD_NOOP;
   }
   return EOF;
}

static int
interpretBackspaceChord (unsigned char dots, BRL_DriverCommandContext context) {
   switch (dots & 0X3F) {
      default:
	 break;
      case BNC_SPACE:
	 return BRL_BLK_PASSKEY + BRL_KEY_BACKSPACE;
	 return EOF;
      case BNC_A:
	 return BRL_CMD_DISPMD | BRL_FLG_TOGGLE_ON;
      case BNC_B:
         return BRL_CMD_SKPBLNKWINS | BRL_FLG_TOGGLE_OFF;
      case BNC_D:
	 temporaryRoutingOperation = BRL_BLK_DESCCHAR;
         return BRL_CMD_NOOP;
      case BNC_F:
         return BRL_CMD_FREEZE | BRL_FLG_TOGGLE_OFF;
      case BNC_H:
	 return BRL_CMD_HELP;
      case BNC_I:
         return BRL_CMD_SKPIDLNS | BRL_FLG_TOGGLE_OFF;
      case BNC_M:
	 temporaryRoutingOperation = BRL_BLK_SETMARK;
	 return BRL_CMD_NOOP;
      case BNC_S:
	 return BRL_CMD_INFO;
      case BNC_T:
	 return BRL_CMD_DISPMD | BRL_FLG_TOGGLE_OFF;
      case BNC_V:
	 return BRL_CMD_SWITCHVT_PREV;
      case BNC_W:
         return BRL_CMD_SLIDEWIN | BRL_FLG_TOGGLE_OFF;
      case BNC_6:
         return BRL_CMD_SIXDOTS | BRL_FLG_TOGGLE_ON;
      case BNC_8:
         return BRL_CMD_SIXDOTS | BRL_FLG_TOGGLE_OFF;
      case (BND_1 | BND_2 | BND_3 | BND_4 | BND_5 | BND_6):
	 return BRL_CMD_RESTARTSPEECH;
   }
   return EOF;
}

static int
interpretEnterChord (unsigned char dots, BRL_DriverCommandContext context) {
   switch (dots) {
      default:
      /* These are overridden by the Braille Note itself. */
      case (BND_1): /* decrease speech volume */
      case (BND_4): /* increase speech volume */
      case (BND_2): /* decrease speech pitch */
      case (BND_5): /* increase speech pitch */
      case (BND_3): /* decrease speech speed */
      case (BND_6): /* increase speech speed */
      case BNC_D: /* display the date */
      case BNC_H: /* hear punctuation in current prompt */
      case BNC_S: /* spell name in current prompt */
      case BNC_T: /* display the time */
	 break;
      case BNC_SPACE:
	 return BRL_BLK_PASSKEY + BRL_KEY_ENTER;
	 return EOF;
      case BNC_B:
         return BRL_CMD_SKPBLNKWINS | BRL_FLG_TOGGLE_ON;
      case BNC_F:
         return BRL_CMD_FREEZE | BRL_FLG_TOGGLE_ON;
      case BNC_I:
         return BRL_CMD_SKPIDLNS | BRL_FLG_TOGGLE_ON;
      case BNC_M:
	 temporaryRoutingOperation = BRL_BLK_GOTOMARK;
	 return BRL_CMD_NOOP;
      case BNC_V:
	 return BRL_CMD_SWITCHVT_NEXT;
      case BNC_W:
         return BRL_CMD_SLIDEWIN | BRL_FLG_TOGGLE_ON;
      case (BND_1 | BND_2 | BND_3 | BND_4 | BND_5 | BND_6):
	 return BRL_CMD_RESTARTBRL;
   }
   return EOF;
}

static int
interpretThumbKeys (unsigned char keys, BRL_DriverCommandContext context) {
   switch (keys) {
      default:
	 break;
      case (BNT_PREVIOUS):
	 return BRL_CMD_FWINLT;
      case (BNT_NEXT):
	 return BRL_CMD_FWINRT;
      case (BNT_BACK):
	 return BRL_CMD_LNUP;
      case (BNT_ADVANCE):
	 return BRL_CMD_LNDN;
      case (BNT_PREVIOUS | BNT_BACK):
	 return BRL_CMD_LNBEG;
      case (BNT_NEXT | BNT_ADVANCE):
	 return BRL_CMD_LNEND;
      case (BNT_PREVIOUS | BNT_ADVANCE):
	 return BRL_CMD_TOP_LEFT;
      case (BNT_PREVIOUS | BNT_NEXT):
	 return BRL_CMD_BOT_LEFT;
      case (BNT_BACK | BNT_ADVANCE):
	 return BRL_CMD_BACK;
      case (BNT_BACK | BNT_NEXT):
	 return BRL_CMD_CSRTRK;
   }
   return EOF;
}

static int
interpretRoutingKey (unsigned char key, BRL_DriverCommandContext context) {
   return currentRoutingOperation + key;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
   unsigned char character;
   int count = read(fileDescriptor, &character, 1);
   int (*handler)(unsigned char, BRL_DriverCommandContext);

   if (count != 1) {
      if (count == -1) {
         LogError("BrailleNote read");
      }
      return EOF;
   }
   switch (character) {
      default:
         return EOF;
      case BNI_CHARACTER:
         handler = interpretCharacter;
	 break;
      case BNI_SPACE:
         handler = interpretSpaceChord;
	 break;
      case BNI_BACKSPACE:
         handler = interpretBackspaceChord;
	 break;
      case BNI_ENTER:
         handler = interpretEnterChord;
	 break;
      case BNI_THUMB:
         handler = interpretThumbKeys;
	 break;
      case BNI_ROUTE:
         handler = interpretRoutingKey;
	 break;
      case BNI_DISPLAY:
         handler = visualDisplay;
	 break;
   }

   delay(1);
   if (read(fileDescriptor, &character, 1) != 1) {
      return EOF;
   }

   currentKeyboardMode = temporaryKeyboardMode;
   temporaryKeyboardMode = persistentKeyboardMode;

   currentRoutingOperation = temporaryRoutingOperation;
   temporaryRoutingOperation = persistentRoutingOperation;

   return handler(character, context);
}

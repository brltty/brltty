/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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
#include <sys/termios.h>

#ifdef HAVE_LINUX_VT_H
#include <linux/vt.h>
#endif /* HAVE_LINUX_VT_H */
static int displayDescriptor = -1;
static int displayTerminal;

#include "Programs/misc.h"

#define BRLNAME "BrailleNote"
#define PREFSTYLE ST_None

typedef enum {
   PARM_STATUSCELLS
} DriverParameter;
#define BRLPARMS "statuscells"

#include "Programs/brl_driver.h"
#include "braille.h"

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
visualDisplay (unsigned char character, DriverCommandContext cmds) {
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
      const int maximum = MIN(StatusCellCount, brl->x-1);
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

   if (openSerialDevice(device, &fileDescriptor, &oldSettings)) {
      memset(&newSettings, 0, sizeof(newSettings));
      newSettings.c_cflag = CS8 | CSTOPB | CLOCAL | CREAD;
      newSettings.c_iflag = IGNPAR;
      while (resetSerialDevice(fileDescriptor, &newSettings, B38400)) {
         unsigned char request[] = {BNO_BEGIN, BNO_DESCRIBE};
         if (safe_write(fileDescriptor, request, sizeof(request)) != -1) {
            if (awaitInput(fileDescriptor, 1000)) {
               unsigned char response[3];
               int offset = 0;
               if (readChunk(fileDescriptor, response, &offset, sizeof(response), 100)) {
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
                           persistentRoutingOperation = CR_ROUTE;
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
      if (!keyNumber) keyNumber = 0X100 - VPK_FUNCTION;
      return VAL_PASSKEY + VPK_FUNCTION + (keyNumber - 1);
   }
   return EOF;
}

static int
interpretNavigation (unsigned char dots, DriverCommandContext cmds) {
   switch (dots) {
      default:
         break;
      case BND_0:
         return CMD_HOME;
      case (BND_1):
	 return CMD_CHRLT;
      case (BND_1 | BND_2):
	 return CMD_HWINLT;
      case (BND_2):
	 return CMD_FWINLT;
      case (BND_2 | BND_3):
	 return CMD_FWINLTSKIP;
      case (BND_3):
	 return CMD_LNBEG;
      case (BND_1 | BND_3):
	 return CMD_LNUP;
      case (BND_1 | BND_2 | BND_3):
	 return CMD_TOP_LEFT;
      case (BND_4):
	 return CMD_CHRRT;
      case (BND_4 | BND_5):
	 return CMD_HWINRT;
      case (BND_5):
	 return CMD_FWINRT;
      case (BND_5 | BND_6):
	 return CMD_FWINRTSKIP;
      case (BND_6):
	 return CMD_LNEND;
      case (BND_4 | BND_6):
	 return CMD_LNDN;
      case (BND_4 | BND_5 | BND_6):
	 return CMD_BOT_LEFT;
      case (BND_1 | BND_4):
	 return CMD_TOP;
      case (BND_2 | BND_5):
	 return CMD_HOME;
      case (BND_3 | BND_6):
	 return CMD_BOT;
      case (BND_1 | BND_4 | BND_5):
	 return CMD_PRDIFLN;
      case (BND_2 | BND_5 | BND_6):
	 return CMD_NXDIFLN;
      case (BND_1 | BND_2 | BND_4):
	 return CMD_PRSEARCH;
      case (BND_2 | BND_3 | BND_5):
	 return CMD_NXSEARCH;
      case (BND_1 | BND_2 | BND_5):
	 return CMD_ATTRUP;
      case (BND_2 | BND_3 | BND_6):
	 return CMD_ATTRDN;
      case (BND_2 | BND_4):
	 temporaryRoutingOperation = CR_PRINDENT;
	 return CMD_NOOP;
      case (BND_3 | BND_5):
	 temporaryRoutingOperation = CR_NXINDENT;
	 return CMD_NOOP;
      case (BND_2 | BND_4 | BND_5):
	 return CMD_WINUP;
      case (BND_3 | BND_5 | BND_6):
	 return CMD_WINDN;
   }
   return EOF;
}

static int
interpretCharacter (unsigned char dots, DriverCommandContext cmds) {
   int mask = 0X00;
   if (cmds != CMDS_SCREEN) {
      return interpretNavigation(dots, cmds);
   }
   switch (currentKeyboardMode) {
      case KBM_NAVIGATE:
         return interpretNavigation(dots, cmds);
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
   return VAL_PASSDOTS + (inputTable[dots] | mask);
}

static int
interpretSpaceChord (unsigned char dots, DriverCommandContext cmds) {
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
	 return interpretCharacter(dots, cmds);
      case BNC_C:
	 return CMD_PREFMENU;
      case BNC_D:
	 return CMD_PREFLOAD;
      case BNC_F:
	 return getFunctionKey();
      case BNC_L:
	 temporaryRoutingOperation = CR_SETLEFT;
	 return CMD_NOOP;
      case BNC_M:
	 return CMD_MUTE;
      case BNC_N:
         persistentKeyboardMode = KBM_NAVIGATE;
	 temporaryKeyboardMode = persistentKeyboardMode;
	 return CMD_NOOP;
      case BNC_P:
	 return CMD_PASTE;
      case BNC_S:
	 return CMD_SAY_LINE;
      case BNC_V: {
	 unsigned int vt;
	 if (getDecimalInteger(&vt, 2, "virt term num")) {
	    if (!vt) vt = 0X100;
	    return CR_SWITCHVT + (vt - 1);
	 }
         return EOF;
      }
      case BNC_W:
	 return CMD_PREFSAVE;
      case BNC_X: {
	 unsigned char character;
	 if (!getHexadecimalCharacter(&character)) {
	    return EOF;
	 }
	 return VAL_PASSCHAR + character;
      }
      case BNC_LPAREN:
	 temporaryRoutingOperation = CR_CUTBEGIN;
	 return CMD_NOOP;
      case BNC_LBRACE:
	 temporaryRoutingOperation = CR_CUTAPPEND;
	 return CMD_NOOP;
      case BNC_RPAREN:
	 temporaryRoutingOperation = CR_CUTRECT;
	 return CMD_NOOP;
      case BNC_RBRACE:
	 temporaryRoutingOperation = CR_CUTLINE;
	 return CMD_NOOP;
      case BNC_BAR:
         return CMD_CSRJMP_VERT;
      case BNC_QUESTION:
         return CMD_LEARN;
      case (BND_2 | BND_3 | BND_5 | BND_6):
	 return VAL_PASSKEY + VPK_TAB;
      case (BND_2 | BND_3):
	 return VAL_PASSKEY + VPK_CURSOR_LEFT;
      case (BND_5 | BND_6):
	 return VAL_PASSKEY + VPK_CURSOR_RIGHT;
      case (BND_2 | BND_5):
	 return VAL_PASSKEY + VPK_CURSOR_UP;
      case (BND_3 | BND_6):
	 return VAL_PASSKEY + VPK_CURSOR_DOWN;
      case (BND_2):
	 return VAL_PASSKEY + VPK_HOME;
      case (BND_3):
	 return VAL_PASSKEY + VPK_END;
      case (BND_5):
	 return VAL_PASSKEY + VPK_PAGE_UP;
      case (BND_6):
	 return VAL_PASSKEY + VPK_PAGE_DOWN;
      case (BND_3 | BND_5):
	 return VAL_PASSKEY + VPK_INSERT;
      case (BND_2 | BND_5 | BND_6):
	 return VAL_PASSKEY + VPK_DELETE;
      case (BND_2 | BND_6):
	 return VAL_PASSKEY + VPK_ESCAPE;
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
	 return CMD_NOOP;
   }
   return EOF;
}

static int
interpretBackspaceChord (unsigned char dots, DriverCommandContext cmds) {
   switch (dots & 0X3F) {
      default:
	 break;
      case BNC_SPACE:
	 return VAL_PASSKEY + VPK_BACKSPACE;
	 return EOF;
      case BNC_A:
	 return CMD_DISPMD | VAL_SWITCHON;
      case BNC_B:
         return CMD_SKPBLNKWINS | VAL_SWITCHOFF;
      case BNC_D:
	 temporaryRoutingOperation = CR_DESCCHAR;
         return CMD_NOOP;
      case BNC_F:
         return CMD_FREEZE | VAL_SWITCHOFF;
      case BNC_H:
	 return CMD_HELP;
      case BNC_I:
         return CMD_SKPIDLNS | VAL_SWITCHOFF;
      case BNC_M:
	 temporaryRoutingOperation = CR_SETMARK;
	 return CMD_NOOP;
      case BNC_S:
	 return CMD_INFO;
      case BNC_T:
	 return CMD_DISPMD | VAL_SWITCHOFF;
      case BNC_V:
	 return CMD_SWITCHVT_PREV;
      case BNC_W:
         return CMD_SLIDEWIN | VAL_SWITCHOFF;
      case BNC_6:
         return CMD_SIXDOTS | VAL_SWITCHON;
      case BNC_8:
         return CMD_SIXDOTS | VAL_SWITCHOFF;
      case (BND_1 | BND_2 | BND_3 | BND_4 | BND_5 | BND_6):
	 return CMD_RESTARTSPEECH;
   }
   return EOF;
}

static int
interpretEnterChord (unsigned char dots, DriverCommandContext cmds) {
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
	 return VAL_PASSKEY + VPK_RETURN;
	 return EOF;
      case BNC_B:
         return CMD_SKPBLNKWINS | VAL_SWITCHON;
      case BNC_F:
         return CMD_FREEZE | VAL_SWITCHON;
      case BNC_I:
         return CMD_SKPIDLNS | VAL_SWITCHON;
      case BNC_M:
	 temporaryRoutingOperation = CR_GOTOMARK;
	 return CMD_NOOP;
      case BNC_V:
	 return CMD_SWITCHVT_NEXT;
      case BNC_W:
         return CMD_SLIDEWIN | VAL_SWITCHON;
      case (BND_1 | BND_2 | BND_3 | BND_4 | BND_5 | BND_6):
	 return CMD_RESTARTBRL;
   }
   return EOF;
}

static int
interpretThumbKeys (unsigned char keys, DriverCommandContext cmds) {
   switch (keys) {
      default:
	 break;
      case (BNT_PREVIOUS):
	 return CMD_FWINLT;
      case (BNT_NEXT):
	 return CMD_FWINRT;
      case (BNT_BACK):
	 return CMD_LNUP;
      case (BNT_ADVANCE):
	 return CMD_LNDN;
      case (BNT_PREVIOUS | BNT_BACK):
	 return CMD_LNBEG;
      case (BNT_NEXT | BNT_ADVANCE):
	 return CMD_LNEND;
      case (BNT_PREVIOUS | BNT_ADVANCE):
	 return CMD_TOP_LEFT;
      case (BNT_PREVIOUS | BNT_NEXT):
	 return CMD_BOT_LEFT;
      case (BNT_BACK | BNT_ADVANCE):
	 return CMD_BACK;
      case (BNT_BACK | BNT_NEXT):
	 return CMD_CSRTRK;
   }
   return EOF;
}

static int
interpretRoutingKey (unsigned char key, DriverCommandContext cmds) {
   return currentRoutingOperation + key;
}

static int
brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds) {
   unsigned char character;
   int count = read(fileDescriptor, &character, 1);
   int (*handler)(unsigned char, DriverCommandContext);

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

   return handler(character, cmds);
}

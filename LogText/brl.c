/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/* LogText/brl.c - Braille display library
 * For Tactilog's LogText
 * Author: Dave Mielke <dave@mielke.cc>
 */

#define BRL_C

#define __EXTENSIONS__  /* for termios.h */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/termios.h>

#include "../brl.h"
#include "../config.h"
#include "../misc.h"
#include "brlconf.h"

#define BRLNAME "LogText"
#define BRLDRIVER "lt"
#define PREFSTYLE ST_Generic
#include "../brl_driver.h"

static int fileDescriptor = -1;
static struct termios oldSettings;
static struct termios newSettings;

#define screenHeight 25
#define screenWidth 80
typedef unsigned char ScreenImage[screenHeight][screenWidth];
static ScreenImage sourceImage;
static ScreenImage targetImage;

typedef enum {
   DEV_OFFLINE,
   DEV_ONLINE,
   DEV_READY
} DeviceStatus;
static DeviceStatus deviceStatus;

static DriverCommandContext currentContext;
static unsigned char currentLine;
static unsigned char cursorRow;
static unsigned char cursorColumn;

static unsigned char inputTable[0X100] = {
   #include "input.h"
};

static unsigned char outputTable[0X100] = {
   #include "output.oslash.h"
};

static void
clearbrl (brldim *brl) {
   brl->y = -1;
   brl->x = -1;
   brl->disp = NULL;
}

static void
identbrl (void) {
   LogPrint(LOG_NOTICE, "LogText Driver");
   LogPrint(LOG_INFO, "   Copyright (C) 2001 by Dave Mielke <dave@mielke.cc>");
}

static void
initbrl (char **parameters, brldim *brl, const char *device) {
   if ((fileDescriptor = open(device, O_RDWR|O_NOCTTY|O_NONBLOCK)) != -1) {
      if (tcgetattr(fileDescriptor, &oldSettings) != -1) {
         memset(&newSettings, 0, sizeof(newSettings));
         newSettings.c_cflag = CS8 | CLOCAL | CREAD;
         newSettings.c_iflag = IGNPAR;
         cfsetispeed(&newSettings, B9600);
         cfsetospeed(&newSettings, B9600);
         if (tcsetattr(fileDescriptor, TCSANOW, &newSettings) != -1) {
            if (tcflush(fileDescriptor, TCIOFLUSH) != -1) {
               brl->y = screenHeight;
               brl->x = screenWidth;
               brl->disp = &sourceImage[0][0];
               memset(sourceImage, 0, sizeof(sourceImage));
               deviceStatus = DEV_ONLINE;
               return;
            } else {
               LogError("LogText flush");
            }
            tcsetattr(fileDescriptor, TCSANOW, &oldSettings);
         } else {
            LogError("LogText attribute set");
         }
      } else {
         LogError("LogText attribute query");
      }
      close(fileDescriptor);
      fileDescriptor = -1;
   } else {
      LogError("LogText open");
   }
   clearbrl(brl);
}

static void
closebrl (brldim *brl) {
   tcsetattr(fileDescriptor, TCSADRAIN, &oldSettings);
   close(fileDescriptor);
   fileDescriptor = -1;
   clearbrl(brl);
}

static void
logData (const unsigned char *data, unsigned int length) {
   unsigned char buffer[(length * 3) + 1];
   unsigned char *out = buffer;
   const unsigned char *in = data;
   while (length--) out += sprintf(out, " %2.2X", *in++);
   LogPrint(LOG_DEBUG, "LogText write:%s", buffer);
}

static int
checkData (const unsigned char *data, unsigned int length) {
   if ((length < 5) || (length != (data[4] + 5))) {
      LogPrint(LOG_ERR, "Bad length: %d", length);
   } else if (data[0] != 255) {
      LogPrint(LOG_ERR, "Bad header: %d", data[0]);
   } else if ((data[1] < 1) || (data[1] > screenHeight)) {
      LogPrint(LOG_ERR, "Bad line: %d", data[1]);
   } else if (data[2] > screenWidth) {
      LogPrint(LOG_ERR, "Bad cursor: %d", data[2]);
   } else if ((data[3] < 1) || (data[3] > screenWidth)) {
      LogPrint(LOG_ERR, "Bad column: %d", data[3]);
   } else if (data[4] > (screenWidth - (data[3] - 1))) {
      LogPrint(LOG_ERR, "Bad count: %d", data[4]);
   } else {
      return 1;
   }
   return 0;
}

static void
sendData (unsigned char line, unsigned char column, unsigned char count) {
   unsigned char data[5 + count];
   unsigned char *target = data;
   unsigned char *source = &targetImage[line][column];
   *target++ = 0XFF;
   *target++ = line + 1;
   *target++ = (line == cursorRow)? cursorColumn+1: 0;
   *target++ = column + 1;
   *target++ = count;
   while (count--) *target++ = outputTable[*source++];
   count = target - data;
   logData(data, count);
   if (checkData(data, count)) {
      if (write(fileDescriptor, data, count) == -1) {
         LogError("LogText write");
      }
   }
}

static void
sendLine (unsigned char line, int force) {
   unsigned char *source = &sourceImage[line][0];
   unsigned char *target = &targetImage[line][0];
   unsigned char start = 0;
   unsigned char count = screenWidth;
   while (count > 0) {
      if (source[count-1] != target[count-1]) break;
      --count;
   }
   while (start < count) {
      if (source[start] != target[start]) break;
      ++start;
   }
   if ((count -= start) || force) {
      LogPrint(LOG_DEBUG, "LogText line: line=%d, column=%d, count=%d", line, start, count);
      memcpy(&target[start], &source[start], count);
      sendData(line, start, count);
   }
}

static void
sendCurrentLine (void) {
   sendLine(currentLine, 0);
}

static void
sendCursorRow (void) {
   sendLine(cursorRow, 1);
}

static void
writebrl (brldim *brl) {
   if (deviceStatus == DEV_READY) {
      sendCurrentLine();
   }
}

static int
isOnline (void) {
   int signals;
   if (ioctl(fileDescriptor, TIOCMGET, &signals) == -1) {
      LogError("TIOCMGET");
   } else if (!(signals & TIOCM_DSR)) {
      if (deviceStatus > DEV_OFFLINE) {
         deviceStatus = DEV_OFFLINE;
         LogPrint(LOG_WARNING, "LogText offline.");
      }
   } else {
      if (deviceStatus < DEV_ONLINE) {
         deviceStatus = DEV_ONLINE;
         LogPrint(LOG_WARNING, "LogText online.");
      }
      return 1;
   }
   return 0;
}

static void
setbrlstat (const unsigned char *status) {
   if (isOnline()) {
      if (status[FirstStatusCell] == FSC_GENERIC) {
         unsigned char row = status[STAT_csrrow];
         unsigned char column = status[STAT_csrcol];
         row = MAX(1, MIN(row, screenHeight)) - 1;
         column = MAX(1, MIN(column, screenWidth)) - 1;
         if (deviceStatus < DEV_READY) {
            memset(targetImage, 0, sizeof(targetImage));
            currentContext = CMDS_SCREEN;
            currentLine = row;
            cursorRow = screenHeight;
            cursorColumn = screenWidth;
            deviceStatus = DEV_READY;
         }
         if ((row != cursorRow) || (column != cursorColumn)) {
            LogPrint(LOG_DEBUG, "cursor moved: [%d,%d] -> [%d,%d]", cursorColumn, cursorRow, column, row);
            cursorRow = row;
            cursorColumn = column;
            sendCursorRow();
         }
      }
   }
}

static unsigned char
readByte (void) {
   unsigned char byte;
   while (read(fileDescriptor, &byte, 1) != 1) delay(1);
   return byte;
}

static int
readbrl (DriverCommandContext cmds) {
   unsigned char byte;
   if (cmds != currentContext) {
      LogPrint(LOG_DEBUG, "Context switch: %d -> %d", currentContext, cmds);
      switch (currentContext = cmds) {
         case CMDS_SCREEN:
            deviceStatus = DEV_ONLINE;
            break;
         default:
            break;
      }
   }
   if (read(fileDescriptor, &byte, 1) == 1) {
      switch (byte) {
         case KEY_COMMAND:
            byte = readByte();
            LogPrint(LOG_DEBUG, "Received character: (0x%2.2X) 0x%2.2X dec=%d", KEY_COMMAND, byte, byte);
            switch (byte) {
               case KEY_COMMAND:
                  /* pressing the escape command twice will pass it through */
                  return VAL_PASSDOTS + inputTable[KEY_COMMAND];
               case KEY_COMMAND_SWITCHVT_PREV:
                  return CMD_SWITCHVT_PREV;
               case KEY_COMMAND_SWITCHVT_NEXT:
                  return CMD_SWITCHVT_NEXT;
               case KEY_COMMAND_SWITCHVT_1:
                  return CR_SWITCHVT + 0;
               case KEY_COMMAND_SWITCHVT_2:
                  return CR_SWITCHVT + 1;
               case KEY_COMMAND_SWITCHVT_3:
                  return CR_SWITCHVT + 2;
               case KEY_COMMAND_SWITCHVT_4:
                  return CR_SWITCHVT + 3;
               case KEY_COMMAND_SWITCHVT_5:
                  return CR_SWITCHVT + 4;
               case KEY_COMMAND_SWITCHVT_6:
                  return CR_SWITCHVT + 5;
               case KEY_COMMAND_PAGE_UP:
                  return VAL_PASSKEY + VPK_PAGE_UP;
               case KEY_COMMAND_PAGE_DOWN:
                  return VAL_PASSKEY + VPK_PAGE_DOWN;
               case KEY_COMMAND_CUT_BEG:
                  return CMD_CUT_BEG;
               case KEY_COMMAND_CUT_END:
                  return CMD_CUT_END;
               case KEY_COMMAND_PASTE:
                  return CMD_PASTE;
               case KEY_COMMAND_PREFMENU:
                  currentLine = 0;
                  cursorRow = 0;
                  cursorColumn = 31;
                  sendCursorRow();
                  return CMD_PREFMENU;
               case KEY_COMMAND_PREFSAVE:
                  return CMD_PREFSAVE;
               case KEY_COMMAND_PREFLOAD:
                  return CMD_PREFLOAD;
               case KEY_COMMAND_FREEZE_ON:
                  return CMD_FREEZE | VAL_SWITCHON;
               case KEY_COMMAND_FREEZE_OFF:
                  return CMD_FREEZE | VAL_SWITCHOFF;
               case KEY_COMMAND_RESTARTBRL:
                  return CMD_RESTARTBRL;
               case KEY_FUNCTION:
                  switch (byte = readByte()) {
                     case KEY_FUNCTION_CURSOR_UP:
                     case KEY_FUNCTION_CURSOR_LEFT:
                     case KEY_FUNCTION_CURSOR_RIGHT:
                     case KEY_FUNCTION_CURSOR_DOWN:
                     default:
                        LogPrint(LOG_WARNING, "Unknown function key: 0X%02X 0X%02X 0X%02X", KEY_COMMAND, KEY_FUNCTION, byte);
                  }
                  break;
               default:
                  LogPrint(LOG_WARNING, "Not an escape command: (0X%2.2X) 0X%2.2X dec=%d", KEY_COMMAND, byte, byte);
            }
            break;
         case KEY_FUNCTION:
            byte = readByte();
            LogPrint(LOG_DEBUG, "Function Key: (0X%2.2X) 0X%2.2X dec=%d", KEY_FUNCTION, byte, byte);
            switch (byte) {
               case KEY_FUNCTION_CURSOR_UP:
                  return VAL_PASSKEY + VPK_CURSOR_UP;
               case KEY_FUNCTION_CURSOR_DOWN:
                  return VAL_PASSKEY + VPK_CURSOR_DOWN;
               case KEY_FUNCTION_CURSOR_LEFT:
                  return VAL_PASSKEY + VPK_CURSOR_LEFT;
               case KEY_FUNCTION_CURSOR_RIGHT:
                  return VAL_PASSKEY + VPK_CURSOR_RIGHT;
               case KEY_FUNCTION_CURSOR_UP_JUMP:
                  return VAL_PASSKEY + VPK_HOME;
               case KEY_FUNCTION_CURSOR_DOWN_JUMP:
                  return VAL_PASSKEY + VPK_END;
               case KEY_FUNCTION_CURSOR_LEFT_JUMP:
                  return VAL_PASSKEY + VPK_PAGE_UP;
               case KEY_FUNCTION_CURSOR_RIGHT_JUMP:
                  return VAL_PASSKEY + VPK_PAGE_DOWN;
               default:
                  LogPrint(LOG_WARNING, "Unknown function key: 0X%2.2X %2.2X", KEY_FUNCTION, byte);
                  break;
            }
            break;
         case KEY_UPDATE:
            byte = readByte();
            LogPrint(LOG_DEBUG, "Request line: 0X%2.2X 0X%2.2X dec=%d", KEY_UPDATE, byte, byte);
            if (!byte) {
               sendCursorRow();
            } else if (byte <= screenHeight) {
               currentLine = byte - 1;
               sendCurrentLine();
            } else {
               LogPrint(LOG_WARNING, "Request for update of line %d.", byte);
            }
            break;
         case KEY_ESCAPE:
            LogPrint(LOG_DEBUG, "KEY_ESCAPE");
            return VAL_PASSKEY + VPK_ESCAPE;
         case KEY_DELETE:
            LogPrint(LOG_DEBUG, "KEY_DELETE");
            return VAL_PASSKEY + VPK_DELETE;
         default: {
            unsigned char dots = inputTable[byte];
            LogPrint(LOG_DEBUG, "Received character: 0X%2.2X dec=%d dots=%2.2X", byte, byte, dots);
            return VAL_PASSDOTS + dots;
         }
      }
   }
   return EOF;
}

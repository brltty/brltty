/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
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

static const char *downloadPath = "logtext-download";

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
   #include "output.h"
};

static void
clearbrl (brldim *brl) {
   brl->y = -1;
   brl->x = -1;
   brl->disp = NULL;
}

static void
brl_identify (void) {
   LogPrint(LOG_NOTICE, "LogText Driver");
   LogPrint(LOG_INFO, "   Copyright (C) 2001 by Dave Mielke <dave@mielke.cc>");
}

static int
makeFifo (const char *path, mode_t mode) {
   struct stat status;
   if (lstat(path, &status) != -1) {
      if (S_ISFIFO(status.st_mode)) return 1;
      LogPrint(LOG_ERR, "Download object not a FIFO: %s", path);
   } else if (errno == ENOENT) {
      mode_t mask = umask(0);
      int result = mkfifo(path, mode);
      int error = errno;
      umask(mask);
      if (result != -1) return 1;
      errno = error;
      LogError("Download FIFO creation");
   }
   return 0;
}

static int
makeDownloadFifo (void) {
   return makeFifo(downloadPath, S_IRUSR|S_IWUSR|S_IWGRP|S_IWOTH);
}

static void
brl_initialize (char **parameters, brldim *brl, const char *device) {
   makeDownloadFifo();
   if (openSerialDevice(device, &fileDescriptor, &oldSettings)) {
      memset(&newSettings, 0, sizeof(newSettings));
      newSettings.c_cflag = CS8 | CLOCAL | CREAD;
      newSettings.c_iflag = IGNPAR;
      if (resetSerialDevice(fileDescriptor, &newSettings, B9600)) {
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
      }
      close(fileDescriptor);
      fileDescriptor = -1;
   }
   clearbrl(brl);
}

static void
brl_close (brldim *brl) {
   tcsetattr(fileDescriptor, TCSADRAIN, &oldSettings);
   close(fileDescriptor);
   fileDescriptor = -1;
   clearbrl(brl);
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

static int
sendBytes (const unsigned char *bytes, size_t count) {
   if (write(fileDescriptor, bytes, count) == -1) {
      LogError("LogText write");
      return 0;
   }
   return 1;
}

static int
sendData (unsigned char line, unsigned char column, unsigned char count) {
   unsigned char data[5 + count];
   unsigned char *target = data;
   unsigned char *source = &targetImage[line][column];
   *target++ = 0XFF;
   *target++ = line + 1;
   *target++ = (line == cursorRow)? cursorColumn+1: 0;
   *target++ = column + 1;
   *target++ = count;
   LogBytes("Output dots", source, count);
   while (count--) *target++ = outputTable[*source++];
   count = target - data;
   LogBytes("LogText write", data, count);
   if (checkData(data, count)) {
      if (sendBytes(data, count)) {
         return 1;
      }
   }
   return 0;
}

static int
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
      if (!sendData(line, start, count)) {
         return 0;
      }
   }
   return 1;
}

static int
sendCurrentLine (void) {
   return sendLine(currentLine, 0);
}

static int
sendCursorRow (void) {
   return sendLine(cursorRow, 1);
}

static int
handleUpdate (unsigned char line) {
   LogPrint(LOG_DEBUG, "Request line: (0X%2.2X) 0X%2.2X dec=%d", KEY_UPDATE, line, line);
   if (!line) return sendCursorRow();
   if (line <= screenHeight) {
      currentLine = line - 1;
      return sendCurrentLine();
   }
   LogPrint(LOG_WARNING, "Invalid line request: %d", line);
   return 1;
}

static void
brl_writeWindow (brldim *brl) {
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
brl_writeStatus (const unsigned char *status) {
   if (isOnline()) {
      if (status[FirstStatusCell] == FSC_GENERIC) {
         unsigned char row = status[STAT_CSRROW];
         unsigned char column = status[STAT_CSRCOL];
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

static int
readKey (void) {
   unsigned char key;
   unsigned char arg;
   if (read(fileDescriptor, &key, 1) != 1) return EOF;
   switch (key) {
      default:
         arg = 0;
         break;
      case KEY_FUNCTION:
      case KEY_FUNCTION2:
      case KEY_UPDATE:
         while (read(fileDescriptor, &arg, 1) != 1) delay(1);
         break;
   }
   {
      int result = COMPOUND_KEY(key, arg);
      LogPrint(LOG_DEBUG, "Key read: %4.4X", result);
      return result;
   }
}

/*askUser
static unsigned char *selectedLine;

static void
replaceCharacters (const unsigned char *address, size_t count) {
   while (count--) selectedLine[cursorColumn++] = inputTable[*address++];
}

static void
insertCharacters (const unsigned char *address, size_t count) {
   memmove(&selectedLine[cursorColumn+count], &selectedLine[cursorColumn], screenWidth-cursorColumn-count);
   replaceCharacters(address, count);
}

static void
deleteCharacters (size_t count) {
   memmove(&selectedLine[cursorColumn], &selectedLine[cursorColumn+count], screenWidth-cursorColumn-count);
   memset(&selectedLine[screenWidth-count], inputTable[' '], count);
}

static void
clearCharacters (void) {
   cursorColumn = 0;
   deleteCharacters(screenWidth);
}

static void
selectLine (unsigned char line) {
   selectedLine = &sourceImage[cursorRow = line][0];
   clearCharacters();
   deviceStatus = DEV_ONLINE;
}

static unsigned char *
askUser (const unsigned char *prompt) {
   unsigned char from;
   unsigned char to;
   selectLine(screenHeight-1);
   LogPrint(LOG_DEBUG, "Prompt: %s", prompt);
   replaceCharacters(prompt, strlen(prompt));
   from = to = ++cursorColumn;
   sendCursorRow();
   while (1) {
      int key = readKey();
      if (key == EOF) {
         delay(1);
         continue;
      }
      if ((key & KEY_MASK) == KEY_UPDATE) {
         handleUpdate(key >> KEY_SHIFT);
         continue;
      }
      if (isgraph(key)) {
         if (to < screenWidth) {
            unsigned char character = key & KEY_MASK;
            insertCharacters(&character, 1);
            ++to;
         } else {
            ringBell();
         }
      } else {
         switch (key) {
            case 0X0D:
               if (to > from) {
                  size_t length = to - from;
                  unsigned char *response = malloc(length+1);
                  if (response) {
                     response[length] = 0;
                     do {
                        response[--length] = outputTable[selectedLine[--to]];
                     } while (to > from);
                     LogPrint(LOG_DEBUG, "Response: %s", response);
                     return response;
                  } else {
                     LogError("Download file path allocation");
                  }
               }
               return NULL;
            case 0X08:
               if (cursorColumn > from) {
                  --cursorColumn;
                  deleteCharacters(1);
                  --to;
               } else {
                  ringBell();
               }
               break;
            case 0X7F:
               if (cursorColumn < to) {
                  deleteCharacters(1);
                  --to;
               } else {
                  ringBell();
               }
               break;
            case KEY_FUNCTION_CURSOR_LEFT:
               if (cursorColumn > from) {
                  --cursorColumn;
               } else {
                  ringBell();
               }
               break;
            case KEY_FUNCTION_CURSOR_LEFT_JUMP:
               if (cursorColumn > from) {
                  cursorColumn = from;
               } else {
                  ringBell();
               }
               break;
            case KEY_FUNCTION_CURSOR_RIGHT:
               if (cursorColumn < to) {
                  ++cursorColumn;
               } else {
                  ringBell();
               }
               break;
            case KEY_FUNCTION_CURSOR_RIGHT_JUMP:
               if (cursorColumn < to) {
                  cursorColumn = to;
               } else {
                  ringBell();
               }
               break;
            default:
               ringBell();
               break;
         }
      }
      sendCursorRow();
   }
}
*/

static void
downloadFile (void) {
   if (makeDownloadFifo()) {
      int file = open(downloadPath, O_RDONLY);
      if (file != -1) {
         struct stat status;
         if (fstat(file, &status) != -1) {
            unsigned char buffer[0X400];
            const unsigned char *address = buffer;
            int count = 0;
            while (1) {
               const unsigned char *newline;
               if (!count) {
                  count = read(file, buffer, sizeof(buffer));
                  if (!count) {
                     static const char fileTrailer[] = {0X1A};
                     sendBytes(fileTrailer, sizeof(fileTrailer));
                     break;
                  }
                  if (count == -1) {
                     LogError("Download file read");
                     break;
                  }
                  address = buffer;
               }
               if ((newline = memchr(address, '\n', count))) {
                  static const char lineTrailer[] = {0X0D, 0X0A};
                  size_t length = newline - address;
                  if (!sendBytes(address, length++)) break;
                  if (!sendBytes(lineTrailer, sizeof(lineTrailer))) break;
                  address += length;
                  count -= length;
               } else {
                  if (!sendBytes(address, count)) break;
                  count = 0;
               }
            }
         } else {
            LogError("Download file status");
         }
         if (close(file) == -1) {
            LogError("Download file close");
         }
      } else {
         LogError("Download file open");
      }
   } else {
      LogPrint(LOG_WARNING, "Download path not specified.");
   }
}

static int
brl_read (DriverCommandContext cmds) {
   int key = readKey();
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
   if (key != EOF) {
      switch (key) {
         case KEY_FUNCTION_ENTER:
            return VAL_PASSKEY + VPK_RETURN;
         case KEY_FUNCTION_TAB:
            return VAL_PASSKEY + VPK_TAB;
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
         case KEY_FUNCTION_F1:
            return VAL_PASSKEY + VPK_FUNCTION + 0;
         case KEY_FUNCTION_F2:
            return VAL_PASSKEY + VPK_FUNCTION + 1;
         case KEY_FUNCTION_F3:
            return VAL_PASSKEY + VPK_FUNCTION + 2;
         case KEY_FUNCTION_F4:
            return VAL_PASSKEY + VPK_FUNCTION + 3;
         case KEY_FUNCTION_F5:
            return VAL_PASSKEY + VPK_FUNCTION + 4;
         case KEY_FUNCTION_F6:
            return VAL_PASSKEY + VPK_FUNCTION + 5;
         case KEY_FUNCTION_F7:
            return VAL_PASSKEY + VPK_FUNCTION + 6;
         case KEY_FUNCTION_F9:
            return VAL_PASSKEY + VPK_FUNCTION + 8;
         case KEY_FUNCTION_F10:
            return VAL_PASSKEY + VPK_FUNCTION + 9;
         case KEY_COMMAND: {
            int command;
            while ((command = readKey()) == EOF) delay(1);
            LogPrint(LOG_DEBUG, "Received command: (0x%2.2X) 0x%4.4X", KEY_COMMAND, command);
            switch (command) {
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
               case KEY_COMMAND_SWITCHVT_7:
                  return CR_SWITCHVT + 6;
               case KEY_COMMAND_SWITCHVT_8:
                  return CR_SWITCHVT + 7;
               case KEY_COMMAND_SWITCHVT_9:
                  return CR_SWITCHVT + 8;
               case KEY_COMMAND_SWITCHVT_10:
                  return CR_SWITCHVT + 9;
               case KEY_COMMAND_PAGE_UP:
                  return VAL_PASSKEY + VPK_PAGE_UP;
               case KEY_COMMAND_PAGE_DOWN:
                  return VAL_PASSKEY + VPK_PAGE_DOWN;
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
               case KEY_COMMAND_DOWNLOAD:
                  downloadFile();
                  break;
               default:
                  LogPrint(LOG_WARNING, "Unknown command: (0X%2.2X) 0X%4.4X", KEY_COMMAND, command);
                  break;
            }
            break;
         }
         default:
            switch (key & KEY_MASK) {
               case KEY_UPDATE:
                  handleUpdate(key >> KEY_SHIFT);
                  break;
               case KEY_FUNCTION:
                  LogPrint(LOG_WARNING, "Unknown function: (0X%2.2X) 0X%4.4X", KEY_COMMAND, key>>KEY_SHIFT);
                  break;
               default: {
                  unsigned char dots = inputTable[key];
                  LogPrint(LOG_DEBUG, "Received character: 0X%2.2X dec=%d dots=%2.2X", key, key, dots);
                  return VAL_PASSDOTS + dots;
               }
            }
            break;
      }
   }
   return EOF;
}

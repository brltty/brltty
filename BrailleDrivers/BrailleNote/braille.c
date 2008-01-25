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

/* BrailleNote/braille.c - Braille display library
 * For Pulse Data International's Braille Note series
 * Author: Dave Mielke <dave@mielke.cc>
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "misc.h"

typedef enum {
  PARM_STATUSCELLS
} DriverParameter;
#define BRLPARMS "statuscells"

#define BRL_HAVE_STATUS_CELLS
#define BRL_HAVE_PACKET_IO
#include "brl_driver.h"
#include "braille.h"
#include "tbl.h"

static int logInputPackets = 0;
static int logOutputPackets = 0;

#include "io_serial.h"
static SerialDevice *serialDevice = NULL;
static const int serialBaud = 38400;
static int charactersPerSecond;

typedef union {
  unsigned char bytes[3];
  struct {
    unsigned char code;
    union {
      unsigned char dotKeys;
      unsigned char thumbKeys;
      unsigned char routingKey;
      struct {
        unsigned char statusCells;
        unsigned char textCells;
      } description;
    } values;
  } data;
} ResponsePacket;

#ifdef HAVE_LINUX_VT_H
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#endif /* HAVE_LINUX_VT_H */
static int displayDescriptor = -1;
static int displayTerminal;

static unsigned char *cellBuffer = NULL;
static unsigned int cellCount = 0;
static unsigned char *statusArea;
static int statusCells;
static unsigned char *dataArea;
static int dataCells;

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

static int
readBytes (unsigned char *buffer, int count, int wait) {
  const int timeout = 100;
  return serialReadData(serialDevice, buffer, count,
                        (wait? timeout: 0), timeout);
}

static int
readByte (unsigned char *byte, int wait) {
  int count = readBytes(byte, 1, wait);
  if (count > 0) return 1;

  if (count == 0) errno = EAGAIN;
  return 0;
}

static int
readPacket (unsigned char *packet, int size) {
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    if (!readByte(&byte, (offset > 0))) {
      if (offset > 0) LogBytes(LOG_WARNING, "Partial Packet", packet, offset);
      return 0;
    }

    if (offset < size) {
      if (offset == 0) {
        switch (byte) {
          case BNI_DISPLAY:
            length = 1;
            break;

          case BNI_CHARACTER:
          case BNI_SPACE:
          case BNI_BACKSPACE:
          case BNI_ENTER:
          case BNI_THUMB:
          case BNI_ROUTE:
            length = 2;
            break;

          case BNI_DESCRIBE:
            length = 3;
            break;

          default:
            LogBytes(LOG_WARNING, "Unknown Packet", &byte, 1);
            offset = 0;
            length = 0;
            continue;
        }
      }

      packet[offset] = byte;
    } else {
      if (offset == size) LogBytes(LOG_WARNING, "Truncated Packet", packet, offset);
      LogBytes(LOG_WARNING, "Discarded Byte", &byte, 1);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        continue;
      }

      if (logInputPackets) LogBytes(LOG_DEBUG, "Input Packet", packet, offset);
      return length;
    }
  }
}

static int
getPacket (ResponsePacket *packet) {
  return readPacket(packet->bytes, sizeof(*packet));
}

static int
writePacket (BrailleDisplay *brl, const unsigned char *packet, int size) {
  unsigned char buffer[1 + (size * 2)];
  unsigned char *byte = buffer;

  *byte++ = BNO_BEGIN;

  while (size > 0) {
    if ((*byte++ = *packet++) == BNO_BEGIN) *byte++ = BNO_BEGIN;
    --size;
  }

  {
    int count = byte - buffer;
    if (logOutputPackets) LogBytes(LOG_DEBUG, "Output Packet", buffer, count);

    {
      int ok = serialWriteData(serialDevice, buffer, count) != -1;
      if (ok) brl->writeDelay += (count * 1000 / charactersPerSecond) + 1;
      return ok;
    }
  }
}

static int
refreshCells (BrailleDisplay *brl) {
  unsigned char buffer[1 + cellCount];
  unsigned char *byte = buffer;

  *byte++ = BNO_WRITE;

  {
    int i;
    for (i=0; i<cellCount; ++i) *byte++ = outputTable[cellBuffer[i]];
  }

  return writePacket(brl, buffer, byte-buffer);
}

static void
writePrompt (BrailleDisplay *brl, const char *prompt) {
  int length = strlen(prompt);
  int index = 0;
  if (length > dataCells) length = dataCells;
  while (index < length) {
    dataArea[index] = textTable[(unsigned char)prompt[index]];
     ++index;
  }
  while (index < dataCells) dataArea[index++] = 0;
  refreshCells(brl);
}

static unsigned char
getByte (void) {
  unsigned char byte;
  while (!serialAwaitInput(serialDevice, 1000000000));
  serialReadData(serialDevice, &byte, 1, 0, 0);
  return byte;
}

static unsigned char
getCharacter (BrailleDisplay *brl) {
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
    refreshCells(brl);
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
visualDisplay (BrailleDisplay *brl, unsigned char byte, BRL_DriverCommandContext context) {
  int vt = getVirtualTerminal();
  const unsigned char end[] = {0X1B, 0};
  unsigned int state = 0;
  openVisualDisplay();
  writeVisualDisplay(BNI_DISPLAY);
  for (;;) {
    unsigned char character = getByte();
    if (character == end[state]) {
      if (++state == sizeof(end)) break;
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
adjustStatusCells (BrailleDisplay *brl, const char *parameter) {
  if (*parameter) {
    const int minimum = 0;
    const int maximum = MIN(BRL_MAX_STATUS_CELL_COUNT, brl->x-1);
    if (validateInteger(&statusCells, parameter, &minimum, &maximum)) {
      statusArea = dataArea;
      dataArea = statusArea + statusCells;
      brl->x -= statusCells;
      dataCells -= statusCells;
    } else {
      LogPrint(LOG_WARNING, "%s: %s", "invalid status cell count", parameter);
    }
  }
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  {
     static const DotsTable dots = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
     makeOutputTable(dots, outputTable);
     reverseTranslationTable(outputTable, inputTable);
  }

  if (!isSerialDevice(&device)) {
     unsupportedDevice(device);
     return 0;
  }

  if ((serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(serialDevice, serialBaud)) {
      unsigned char request[] = {BNO_DESCRIBE};
      charactersPerSecond = serialBaud / 10;
      if (writePacket(brl, request, sizeof(request)) != -1) {
        while (serialAwaitInput(serialDevice, 100)) {
          ResponsePacket response;
          int size = getPacket(&response);
          if (size) {
            if (response.data.code == BNI_DESCRIBE) {
              statusCells = response.data.values.description.statusCells;
              brl->x = response.data.values.description.textCells;
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
                refreshCells(brl);
                persistentKeyboardMode = KBM_NAVIGATE;
                temporaryKeyboardMode = persistentKeyboardMode;
                persistentRoutingOperation = BRL_BLK_ROUTE;
                temporaryRoutingOperation = persistentRoutingOperation;
                adjustStatusCells(brl, parameters[PARM_STATUSCELLS]);
                return 1;
              } else {
                LogError("cell buffer allocation");
              }
            } else {
              LogBytes(LOG_WARNING, "Unexpected Packet", response.bytes, size);
            }
          }
        }
      }
    }
    serialCloseDevice(serialDevice);
    serialDevice = NULL;
  }
  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  serialCloseDevice(serialDevice);
  serialDevice = NULL;

  free(cellBuffer);
  cellBuffer = NULL;
}

static ssize_t
brl_readPacket (BrailleDisplay *brl, void *buffer, size_t size) {
  int count = readPacket(buffer, size);
  if (!count) count = -1;
  return count;
}

static ssize_t
brl_writePacket (BrailleDisplay *brl, const void *packet, size_t length) {
  return writePacket(brl, packet, length)? length: -1;
}

static int
brl_reset (BrailleDisplay *brl) {
  return 0;
}

static int
brl_writeWindow (BrailleDisplay *brl) {
  if (memcmp(dataArea, brl->buffer, dataCells) != 0) {
    memcpy(dataArea, brl->buffer, dataCells);
    refreshCells(brl);
  }
  return 1;
}

static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
  if (memcmp(statusArea, status, statusCells) != 0) {
    memcpy(statusArea, status, statusCells);
    /* refreshCells();
     * Not needed since writebrl will be called shortly.
     */
  }
  return 1;
}

static int
getDecimalInteger (BrailleDisplay *brl, unsigned int *integer, unsigned int width, const char *description) {
  char buffer[width + 1];
  memset(buffer, '0', width);
  buffer[width] = 0;
  for (;;) {
    unsigned char character;
    char prompt[0X40];
    snprintf(prompt, sizeof(prompt), "%s: %s", description, buffer);
    writePrompt(brl, prompt);
    switch (character = getCharacter(brl)) {
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
getHexadecimalCharacter (BrailleDisplay *brl, unsigned char *character) {
  *character = 0X00;
  for (;;) {
    unsigned char digit;
    char prompt[0X40];
    snprintf(prompt, sizeof(prompt), "hex char: %2.2x %c", *character, *character);
    writePrompt(brl, prompt);
    switch (getCharacter(brl)) {
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
getFunctionKey (BrailleDisplay *brl) {
  unsigned int keyNumber;
  if (getDecimalInteger(brl, &keyNumber, 2, "function key")) {
    if (!keyNumber) keyNumber = 0X100 - BRL_KEY_FUNCTION;
    return BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + (keyNumber - 1);
  }
  return EOF;
}

static int
interpretNavigation (BrailleDisplay *brl, unsigned char dots, BRL_DriverCommandContext context) {
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
interpretCharacter (BrailleDisplay *brl, unsigned char dots, BRL_DriverCommandContext context) {
  int mask = 0X00;
  if (context != BRL_CTX_SCREEN) return interpretNavigation(brl, dots, context);
  switch (currentKeyboardMode) {
    case KBM_NAVIGATE:
      return interpretNavigation(brl, dots, context);
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
interpretSpaceChord (BrailleDisplay *brl, unsigned char dots, BRL_DriverCommandContext context) {
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
      return interpretCharacter(brl, dots, context);
    case BNC_C:
      return BRL_CMD_PREFMENU;
    case BNC_D:
      return BRL_CMD_PREFLOAD;
    case BNC_F:
      return getFunctionKey(brl);
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
      if (getDecimalInteger(brl, &vt, 2, "virt term num")) {
        if (!vt) vt = 0X100;
        return BRL_BLK_SWITCHVT + (vt - 1);
      }
      return EOF;
    }
    case BNC_W:
      return BRL_CMD_PREFSAVE;
    case BNC_X: {
      unsigned char character;
      if (!getHexadecimalCharacter(brl, &character)) return EOF;
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
      if (dots & BND_5) persistentKeyboardMode = temporaryKeyboardMode;
      return BRL_CMD_NOOP;
  }
  return EOF;
}

static int
interpretBackspaceChord (BrailleDisplay *brl, unsigned char dots, BRL_DriverCommandContext context) {
  switch (dots & 0X3F) {
    default:
      break;
    case BNC_SPACE:
      return BRL_BLK_PASSKEY + BRL_KEY_BACKSPACE;
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
interpretEnterChord (BrailleDisplay *brl, unsigned char dots, BRL_DriverCommandContext context) {
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
interpretThumbKeys (BrailleDisplay *brl, unsigned char keys, BRL_DriverCommandContext context) {
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
interpretRoutingKey (BrailleDisplay *brl, unsigned char key, BRL_DriverCommandContext context) {
  return currentRoutingOperation + key;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  ResponsePacket packet;
  int size;

  while ((size = getPacket(&packet))) {
    int (*handler)(BrailleDisplay *, unsigned char, BRL_DriverCommandContext);
    unsigned char data;

    switch (packet.data.code) {
      case BNI_CHARACTER:
        handler = interpretCharacter;
        data = packet.data.values.dotKeys;
        break;

      case BNI_SPACE:
        handler = interpretSpaceChord;
        data = packet.data.values.dotKeys;
        break;

      case BNI_BACKSPACE:
        handler = interpretBackspaceChord;
        data = packet.data.values.dotKeys;
        break;

      case BNI_ENTER:
        handler = interpretEnterChord;
        data = packet.data.values.dotKeys;
        break;

      case BNI_THUMB:
        handler = interpretThumbKeys;
        data = packet.data.values.thumbKeys;
        break;

      case BNI_ROUTE:
        handler = interpretRoutingKey;
        data = packet.data.values.routingKey;
        break;

      case BNI_DISPLAY:
        handler = visualDisplay;
        data = 0;
        break;

      default:
        LogBytes(LOG_WARNING, "Unexpected Packet", packet.bytes, size);
        continue;
    }

    currentKeyboardMode = temporaryKeyboardMode;
    temporaryKeyboardMode = persistentKeyboardMode;

    currentRoutingOperation = temporaryRoutingOperation;
    temporaryRoutingOperation = persistentRoutingOperation;

    return handler(brl, data, context);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}

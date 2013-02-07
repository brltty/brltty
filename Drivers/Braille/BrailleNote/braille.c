/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

/* BrailleNote/braille.c - Braille display library
 * For Pulse Data International's Braille Note series
 * Author: Dave Mielke <dave@mielke.cc>
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "log.h"
#include "device.h"

#define BRL_HAVE_PACKET_IO
#include "brl_driver.h"
#include "brldefs-bn.h"
#include "ttb.h"

BEGIN_KEY_NAME_TABLE(all)
  KEY_NAME_ENTRY(BN_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(BN_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(BN_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(BN_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(BN_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(BN_KEY_Dot6, "Dot6"),

  KEY_NAME_ENTRY(BN_KEY_Space, "Space"),
  KEY_NAME_ENTRY(BN_KEY_Backspace, "Backspace"),
  KEY_NAME_ENTRY(BN_KEY_Enter, "Enter"),

  KEY_NAME_ENTRY(BN_KEY_Previous, "Previous"),
  KEY_NAME_ENTRY(BN_KEY_Back, "Back"),
  KEY_NAME_ENTRY(BN_KEY_Advance, "Advance"),
  KEY_NAME_ENTRY(BN_KEY_Next, "Next"),

  KEY_SET_ENTRY(BN_SET_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(all),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

static GioEndpoint *gioEndpoint = NULL;

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

static int
readPacket (unsigned char *packet, int size) {
  int offset = 0;
  int length = 0;

  while (1) {
    unsigned char byte;

    if (!gioReadByte(gioEndpoint, &byte, (offset > 0))) {
      if (offset > 0) logPartialPacket(packet, offset);
      return 0;
    }

    if (offset < size) {
      if (offset == 0) {
        switch (byte) {
          case BN_RSP_DISPLAY:
            length = 1;
            break;

          case BN_RSP_CHARACTER:
          case BN_RSP_SPACE:
          case BN_RSP_BACKSPACE:
          case BN_RSP_ENTER:
          case BN_RSP_THUMB:
          case BN_RSP_ROUTE:
            length = 2;
            break;

          case BN_RSP_DESCRIBE:
            length = 3;
            break;

          default:
            logUnknownPacket(byte);
            offset = 0;
            length = 0;
            continue;
        }
      }

      packet[offset] = byte;
    } else {
      if (offset == size) logTruncatedPacket(packet, offset);
      logDiscardedByte(byte);
    }

    if (++offset == length) {
      if (offset > size) {
        offset = 0;
        length = 0;
        continue;
      }

      logInputPacket(packet, offset);
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

  *byte++ = BN_REQ_BEGIN;

  while (size > 0) {
    if ((*byte++ = *packet++) == BN_REQ_BEGIN) *byte++ = BN_REQ_BEGIN;
    --size;
  }

  return writeBraillePacket(brl, gioEndpoint, buffer, byte-buffer);
}

static int
refreshCells (BrailleDisplay *brl) {
  unsigned char buffer[1 + cellCount];
  unsigned char *byte = buffer;

  *byte++ = BN_REQ_WRITE;
  byte = translateOutputCells(byte, cellBuffer, cellCount);

  return writePacket(brl, buffer, byte-buffer);
}

static unsigned char
getByte (void) {
  unsigned char byte;
  while (!gioAwaitInput(gioEndpoint, 1000000000));
  gioReadByte(gioEndpoint, &byte, 0);
  return byte;
}

static int
getVirtualTerminal (void) {
  int vt = -1;
#ifdef HAVE_LINUX_VT_H
  FILE *console = getConsole();
  if (console) {
    int consoleDescriptor = fileno(console);
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
  FILE *console = getConsole();
  if (console) {
    int consoleDescriptor = fileno(console);
    logMessage(LOG_DEBUG, "switching to virtual terminal %d", vt);
    if (ioctl(consoleDescriptor, VT_ACTIVATE, vt) != -1) {
      if (ioctl(consoleDescriptor, VT_WAITACTIVE, vt) != -1) {
        logMessage(LOG_INFO, "switched to virtual terminal %d", vt);
      } else {
        logSystemError("virtual console wait");
      }
    } else {
      logSystemError("virtual console activate");
    }
  }
#endif /* HAVE_LINUX_VT_H */
}

static void
openVisualDisplay (void) {
#ifdef HAVE_LINUX_VT_H
  if (displayDescriptor == -1) {
    FILE *console = getConsole();
    if (console) {
      int consoleDescriptor = fileno(console);
      if (ioctl(consoleDescriptor, VT_OPENQRY, &displayTerminal) != -1) {
        char path[0X20];
        snprintf(path, sizeof(path), "/dev/tty%d", displayTerminal);
        if ((displayDescriptor = open(path, O_WRONLY)) != -1) {
          logMessage(LOG_INFO, "visual display is %s", path);
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

static int
writeVisualDisplay (unsigned char c) {
  if (displayDescriptor != -1) {
    if (write(displayDescriptor, &c, 1) == -1) {
      logSystemError("write");
      return 0;
    }
  }

  return 1;
}

static int
doVisualDisplay (void) {
  int vt = getVirtualTerminal();
  const unsigned char end[] = {ESC, 0};
  unsigned int state = 0;
  openVisualDisplay();
  writeVisualDisplay(BN_RSP_DISPLAY);
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

static int
writeIdentifyRequest (BrailleDisplay *brl) {
  static const unsigned char request[] = {BN_REQ_DESCRIBE};

  return writePacket(brl, request, sizeof(request));
}

static size_t
readResponse (BrailleDisplay *brl, void *packet, size_t size) {
  return readPacket(packet, size);
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  const ResponsePacket *response = packet;

  return (response->data.code == BN_RSP_DESCRIBE)? BRL_RSP_DONE: BRL_RSP_UNEXPECTED;
}

static int
connectResource (const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 38400
  };

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.serial.parameters = &serialParameters;

  if ((gioEndpoint = gioConnectResource(identifier, &descriptor))) {
    return 1;
  }

  return 0;
}

static void
disconnectResource (void) {
  gioDisconnectResource(gioEndpoint);
  gioEndpoint = NULL;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if (connectResource(device)) {
    ResponsePacket response;

    if (probeBrailleDisplay(brl, 0, gioEndpoint, 100,
                              writeIdentifyRequest,
                              readResponse, &response, sizeof(response),
                              isIdentityResponse)) {
      statusCells = response.data.values.description.statusCells;
      brl->textColumns = response.data.values.description.textCells;
      brl->textRows = 1;

      if ((statusCells == 5) && (brl->textColumns == 30)) {
        statusCells -= 2;
        brl->textColumns += 2;
      }

      dataCells = brl->textColumns * brl->textRows;
      cellCount = statusCells + dataCells;

      {
        const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);
        brl->keyBindings = ktd->bindings;
        brl->keyNameTables = ktd->names;
      }

      makeOutputTable(dotsTable_ISO11548_1);
      makeInputTable();

      if ((cellBuffer = malloc(cellCount))) {
        memset(cellBuffer, 0, cellCount);
        statusArea = cellBuffer;
        dataArea = statusArea + statusCells;
        refreshCells(brl);
        return 1;
      } else {
        logSystemError("cell buffer allocation");
      }
    }

    disconnectResource();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  disconnectResource();

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
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (cellsHaveChanged(dataArea, brl->buffer, dataCells, NULL, NULL, NULL)) {
    refreshCells(brl);
  }
  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  ResponsePacket packet;
  int size;

  while ((size = getPacket(&packet))) {
    switch (packet.data.code) {
      case BN_RSP_ROUTE:
        enqueueKey(BN_SET_RoutingKeys, packet.data.values.routingKey);
        break;

      case BN_RSP_DISPLAY:
        doVisualDisplay();
        break;

      default: {
        unsigned char set = BN_SET_NavigationKeys;
        unsigned char keys = packet.data.values.dotKeys & 0X3F;
        unsigned char base = BN_KEY_Dot1;
        unsigned char modifier = 0;

        switch (packet.data.code) {
          case BN_RSP_CHARACTER:
            if (keys) break;

          case BN_RSP_SPACE:
            modifier = BN_KEY_Space;
            break;

          case BN_RSP_BACKSPACE:
            modifier = BN_KEY_Backspace;
            break;

          case BN_RSP_ENTER:
            modifier = BN_KEY_Enter;
            break;

          case BN_RSP_THUMB:
            keys = packet.data.values.thumbKeys & 0X0F;
            base = BN_KEY_Previous;
            break;

          default:
            logUnexpectedPacket(packet.bytes, size);
            continue;
        }

        if (modifier) enqueueKeyEvent(set, modifier, 1);
        enqueueKeys(keys, set, base);
        if (modifier) enqueueKeyEvent(set, modifier, 0);
        break;
      }
    }
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}

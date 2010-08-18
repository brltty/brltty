/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

/* CombiBraille/braille.c - Braille display library
 * For Tieman B.V.'s CombiBraille (serial interface only)
 * Was maintained by Nikhil Nair <nn201@cus.cam.ac.uk>
 * $Id: braille.c,v 1.3 1996/09/24 01:04:29 nn201 Exp $
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "timing.h"
#include "misc.h"

#define BRL_STATUS_FIELDS sfCursorAndWindowColumn, sfCursorAndWindowRow, sfStateDots
#define BRL_HAVE_STATUS_CELLS
#include "brl_driver.h"
#include "brldefs-cb.h"
#include "braille.h"
#include "io_serial.h"

BEGIN_KEY_NAME_TABLE(all)
  KEY_NAME_ENTRY(CB_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(CB_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(CB_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(CB_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(CB_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(CB_KEY_Dot6, "Dot6"),

  KEY_NAME_ENTRY(CB_KEY_Thumb1, "Thumb1"),
  KEY_NAME_ENTRY(CB_KEY_Thumb2, "Thumb2"),
  KEY_NAME_ENTRY(CB_KEY_Thumb3, "Thumb3"),
  KEY_NAME_ENTRY(CB_KEY_Thumb4, "Thumb4"),
  KEY_NAME_ENTRY(CB_KEY_Thumb5, "Thumb5"),

  KEY_NAME_ENTRY(CB_KEY_Status1, "Status1"),
  KEY_NAME_ENTRY(CB_KEY_Status2, "Status2"),
  KEY_NAME_ENTRY(CB_KEY_Status3, "Status3"),
  KEY_NAME_ENTRY(CB_KEY_Status4, "Status4"),
  KEY_NAME_ENTRY(CB_KEY_Status5, "Status5"),
  KEY_NAME_ENTRY(CB_KEY_Status6, "Status6"),

  KEY_SET_ENTRY(CB_SET_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(all),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

SerialDevice *CB_serialDevice;			/* file descriptor for Braille display */
int CB_charactersPerSecond;			/* file descriptor for Braille display */

static TranslationTable outputTable;	/* dot mapping table (output) */
static unsigned char *prevdata;

#define MAX_STATUS_CELLS 5
static unsigned char status[MAX_STATUS_CELLS];
static unsigned char oldstatus[MAX_STATUS_CELLS];

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  prevdata = NULL;		/* clear pointers */

  if ((CB_serialDevice = serialOpenDevice(device))) {
    if (serialRestartDevice(CB_serialDevice, BAUDRATE)) {
      CB_charactersPerSecond = BAUDRATE / 10;

      if (serialSetFlowControl(CB_serialDevice, SERIAL_FLOW_HARDWARE)) {
        static const unsigned char init_seq[] = {0X1B, '?'};

        if (serialWriteData(CB_serialDevice, init_seq, sizeof(init_seq)) == sizeof(init_seq)) {
          static const unsigned char init_ack[] = {0X1B, '?'};
          unsigned int n = 0;
          unsigned char c;
          signed char id = -1;

          hasTimedOut(0);		/* initialise timeout testing */
          do {
            approximateDelay(20);
            if (serialReadData(CB_serialDevice, &c, 1, 0, 0) != 1) continue;

            if (n == sizeof(init_ack)) {
              id = c;
              break;
            }

          testCharacter:
            if (c != init_ack[n]) {
              if (n == 0) continue;

              n = 0;
              goto testCharacter;
            }

            n += 1;
          } while (!hasTimedOut(ACK_TIMEOUT));

          if (id != -1) {
            typedef struct {
              char identifier;
              char textColumns;
            } ModelEntry;

            static const ModelEntry modelTable[] = {
              { .identifier = 0, .textColumns = 20 },
              { .identifier = 1, .textColumns = 40 },
              { .identifier = 2, .textColumns = 80 },
              { .identifier = 7, .textColumns = 20 },
              { .identifier = 8, .textColumns = 40 },
              { .identifier = 9, .textColumns = 80 },
              { .textColumns = 0 }
            };
            const ModelEntry *model = modelTable;

            while (model->textColumns) {
              if (id == model->identifier) break;
              model += 1;
            }
              
            if (model->textColumns) {
              brl->textColumns = model->textColumns;
              brl->textRows = 1;

              brl->statusColumns = 5;
              brl->statusRows = 1;

              {
                const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);
                brl->keyBindings = ktd->bindings;
                brl->keyNameTables = ktd->names;
              }

              {
                static const DotsTable dots = {0X01, 0X02, 0X04, 0X80, 0X40, 0X20, 0X08, 0X10};
                makeOutputTable(dots, outputTable);
              }

              /* Allocate space for buffers */
              prevdata = mallocWrapper(brl->textColumns);

              return 1;
            } else {
              LogPrint(LOG_ERR, "detected unknown CombiBraille model with ID %02X", id);
            }
          }
        }
      }
    }

    serialCloseDevice(CB_serialDevice);
    CB_serialDevice = NULL;
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  if (prevdata) {
    free(prevdata);
    prevdata = NULL;
  }

  if (CB_serialDevice) {
    serialCloseDevice(CB_serialDevice);
    CB_serialDevice = NULL;
  }
}

static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *s) {
  memcpy(status, s, brl->statusColumns);
  return 1;
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  /* Only refresh display if the data has changed: */
  if ((memcmp(brl->buffer, prevdata, brl->textColumns) != 0) ||
      (memcmp(status, oldstatus, brl->statusColumns) != 0)) {
    static const unsigned char header[] = {0X1B, 'B'};
    unsigned char buffer[sizeof(header) + ((brl->statusColumns + brl->textColumns) * 2)];
    unsigned char *byte = buffer;
    int i;			/* loop counter */

    /* save new braille data */
    memcpy(prevdata, brl->buffer, brl->textColumns);
    memcpy(oldstatus, status, brl->statusColumns);

    byte = mempcpy(byte, header, sizeof(header));

    for (i=0; i<brl->statusColumns; i+=1) {
      const unsigned char c = outputTable[status[i]];
      if (c == 0X1B) *byte++ = c;
      *byte++ = c;
    }

    for (i=0; i<brl->textColumns; i+=1) {
      const unsigned char c = outputTable[brl->buffer[i]];
      if (c == 0X1B) *byte++ = c;
      *byte++ = c;
    }

    {
      const size_t size = byte - buffer;
      serialWriteData(CB_serialDevice, buffer, size);
      brl->writeDelay += (size * 1000 / CB_charactersPerSecond) + 1;
    }
  }

  return 1;
}

static int
getKey (void) {
  static short ptr = 0;		/* input queue pointer */
  static unsigned char q[4];	/* input queue */
  unsigned char c;		/* character buffer */

  while (serialReadData (CB_serialDevice, &c, 1, 0, 0) == 1)
    {
      if (ptr == 0 && c != 27)
	continue;
      if (ptr == 1 && c != 'K' && c != 'C')
	{
	  ptr = 0;
	  continue;
	}
      q[ptr++] = c;
      if (ptr < 3 || (ptr == 3 && q[1] == 'K' && !q[2]))
	continue;
      ptr = 0;
      if (q[1] == 'K')
	return (q[2] ? q[2] : q[3] | 0x0060);
      return ((int) q[2] | 0x80);
    }
  return EOF;
}

static void
putKey (unsigned char set, unsigned char key) {
  enqueueKeyEvent(set, key, 1);
  enqueueKeyEvent(set, key, 0);
}

static void
putKeys (unsigned char bits, unsigned char keys, unsigned char count) {
  const unsigned char set = CB_SET_NavigationKeys;
  unsigned int key = 0;

  while (key < count) {
    if (bits & (1 << key)) enqueueKeyEvent(set, keys+key, 1);
    key += 1;
  }

  while (key) {
    key -= 1;
    if (bits & (1 << key)) enqueueKeyEvent(set, keys+key, 0);
  }
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  int key;

  while ((key = getKey()) != EOF) {
    if (key >= 0X86) {
      putKey(CB_SET_RoutingKeys, (key - 0X86));
    } else if (key >= 0X80) {
      putKey(CB_SET_NavigationKeys, (CB_KEY_Status1 + (key - 0X80)));
    } else if (key >= 0X60) {
      putKeys(key, CB_KEY_Thumb1, 5);
    } else if (key < 0X40) {
      putKeys(key, CB_KEY_Dot6, 6);
    }
  }

  return EOF;
}

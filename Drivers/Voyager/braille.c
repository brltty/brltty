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
#define VERSION "0.20"
#define DATE "June 2004"
#define COPYRIGHT \
"   Copyright (C) 2004 by Stéphane Doyon  <s.doyon@videotron.ca>"

/* Voyager/braille.c - Braille display driver for Tieman Voyager displays.
 *
 * Written by Stéphane Doyon  <s.doyon@videotron.ca>
 *
 * It is being tested on Voyager 44, should also support Voyager 70.
 * It is designed to be compiled in BRLTTY version 3.5.
 *
 * History:
 * 0.20, June 2004:
 *       Add statuscells parameter.
 *       Rename brlinput parameter to inputmode.
 *       Change default inputmode to no.
 *       Chorded functions work without chording when inputmode is no.
 *       Move complex routing key combinations to front/dot keys.
 *       Duplicate status key bindings on front/dot keys.
 *       Execute on first release rather than on all released.
 * 0.10, March 2004: Use BRLTTY core repeat functions. Add brlinput parameter
 *   and toggle to disallow braille typing.
 * 0.01, January 2004: fork from the original driver which relied on an
 *   in-kernel USB driver.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#include "Programs/usb.h"
#include "Programs/misc.h"
#include "Programs/message.h"

typedef enum {
  PARM_INPUTMODE,
  PARM_STATUSCELLS
} DriverParameter;
#define BRLPARMS "inputmode", "statuscells"

#define BRLSTAT ST_VoyagerStyle
#define BRL_HAVE_FIRMNESS
#include "Programs/brl_driver.h"
#include "Programs/tbl.h"

/* Workarounds for control transfer flakiness (at least in this demo model) */
#define CONTROL_RETRIES 6

/* control message request codes */
#define CTL_REQ_SET_DISPLAY_STATE    0
#define CTL_REQ_SET_DISPLAY_VOLTAGE  1
#define CTL_REQ_GET_DISPLAY_VOLTAGE  2
#define CTL_REQ_GET_SERIAL_NUMBER    3
#define CTL_REQ_GET_HARDWARE_VERSION 4
#define CTL_REQ_GET_FIRMWARE_VERSION 5
#define CTL_REQ_GET_DISPLAY_LENGTH   6
#define CTL_REQ_WRITE_BRAILLE        7
#define CTL_REQ_GET_DISPLAY_CURRENT  8
#define CTL_REQ_BEEP                 9
#define CTL_REQ_GET_KEYS            11
#define CTL_REQ_SET_DISPLAY_LENGTH  12
#define CTL_REQ_READ_MEMORY        128
#define CTL_REQ_WRITE_MEMORY       129
#define CTL_REQ_READ_CODE          130
#define CTL_REQ_WRITE_IO           131
#define CTL_REQ_READ_IO            132

/* Global variables */
static UsbChannel *usb = NULL;
static char firstRead; /* Flag to reinitialize brl_readCommand() function state. */
static int inputMode;
static TranslationTable outputTable;
static unsigned char *currentCells = NULL; /* buffer to prepare new pattern */
static unsigned char *previousCells = NULL; /* previous pattern displayed */

#define MAXIMUM_CELLS 70 /* arbitrary max for allocations */
static unsigned char totalCells;
static unsigned char textOffset;
static unsigned char textCells;
static unsigned char statusOffset;
static unsigned char statusCells;
#define IS_TEXT_RANGE(key1,key2) (((key1) >= textOffset) && ((key2) < (textOffset + textCells)))
#define IS_TEXT_KEY(key) IS_TEXT_RANGE((key), (key))
#define IS_STATUS_KEY(key) (((key) >= statusOffset) && ((key) < (statusOffset + statusCells)))


static int
tellDisplay (uint8_t request, uint16_t value, uint16_t index,
	     const unsigned char *buffer, uint16_t size) {
  int try = 0;
  while (1) {
    int ret = usbControlWrite(usb->device, USB_RECIPIENT_ENDPOINT, USB_TYPE_VENDOR,
                              request, value, index, buffer, size, 100);
    if (ret != -1) return ret;
    if (errno != EPIPE) break;
    if (++try > CONTROL_RETRIES) break;
    LogPrint(LOG_WARNING, "Voyager request 0X%X (try %d) failed: %s",
             request, try, strerror(errno));
  }
  LogPrint(LOG_ERR, "Voyager request 0X%X error: %s",
           request, strerror(errno));
  return -1;
}

static int
askDisplay (uint8_t request, uint16_t value, uint16_t index,
	    unsigned char *buffer, uint16_t size) {
  int try = 0;
  while (1) {
    int ret = usbControlRead(usb->device, USB_RECIPIENT_ENDPOINT, USB_TYPE_VENDOR,
                             request, value, index, buffer, size, 100);
    if (ret != -1) return ret;
    if (errno != EPIPE) break;
    if (++try > CONTROL_RETRIES) break;
    LogPrint(LOG_WARNING, "Voyager request 0X%X (try %d) failed: %s",
             request, try, strerror(errno));
  }
  LogPrint(LOG_ERR, "Voyager request 0X%X error: %s",
           request, strerror(errno));
  return -1;
}

static int
setDisplayState (uint16_t state) {
  return tellDisplay(CTL_REQ_SET_DISPLAY_STATE, state, 0, NULL, 0) != -1;
}

static int
writeBraille (unsigned char *cells, uint16_t count, uint16_t start) {
  return tellDisplay(CTL_REQ_WRITE_BRAILLE, 0, start, cells, count) != -1;
}

static int
soundBeep (uint16_t duration) {
  return tellDisplay(CTL_REQ_BEEP, duration, 0, NULL, 0) != -1;
}

#define MAX_STRING_LENGTH 0XFF
#define RAW_STRING_SIZE (MAX_STRING_LENGTH * 2) + 2
#define STRING_SIZE (MAX_STRING_LENGTH + 1)
static unsigned char *
decodeString (unsigned char *buffer) {
  static unsigned char string[STRING_SIZE];
  int length = (buffer[0] - 2) / 2;
  int index;

  if (length < 0) {
    length = 0;
  } else if (length >= STRING_SIZE) {
    length = STRING_SIZE - 1;
  }

  for (index=0; index<length; index++)
    string[index] = buffer[2+2*index];
  string[index] = 0;

  return string;
}

static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "Tieman Voyager User-space Driver: version " VERSION " (" DATE ")");
  LogPrint(LOG_INFO, COPYRIGHT);
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  if (!isUsbDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  /* Open the braille display device */
  {
    static const UsbChannelDefinition definitions[] = {
      {0X0798, 0X0001, 1, 0, 0, 1, 0, 0},
      {0}
    };

    LogPrint(LOG_DEBUG, "Attempting open: %s", device);
    usb = usbFindChannel(definitions, (void *)device);
  }

  if (usb) {
    /* log the serial number of the display */
    {
      unsigned char buffer[RAW_STRING_SIZE];
      int size = askDisplay(CTL_REQ_GET_SERIAL_NUMBER, 0, 0, buffer, sizeof(buffer));
      if (size != -1)
        LogPrint(LOG_INFO, "Voyager Serial Number: %s",
                 decodeString(buffer));
    }

    /* log the hardware version of the display */
    {
      unsigned char buffer[2];
      int size = askDisplay(CTL_REQ_GET_HARDWARE_VERSION, 0, 0, buffer, sizeof(buffer));
      if (size != -1)
        LogPrint(LOG_INFO, "Voyager Hardware: %u.%u",
                 buffer[0], buffer[1]);
    }

    /* log the firmware version of the display */
    {
      unsigned char buffer[RAW_STRING_SIZE];
      int size = askDisplay(CTL_REQ_GET_FIRMWARE_VERSION, 0, 0, buffer, sizeof(buffer));
      if (size != -1)
        LogPrint(LOG_INFO, "Voyager Firmware: %s",
                 decodeString(buffer));
    }

    /* find out how big the display is */
    totalCells = 0;
    {
      unsigned char data[2];
      int size = askDisplay(CTL_REQ_GET_DISPLAY_LENGTH, 0, 0, data, sizeof(data));
      if (size != -1) {
        switch (data[1]) {
          case 48:
            totalCells = 44;
            brl->helpPage = 0;
            break;

          case 72:
            totalCells = 70;
            brl->helpPage = 1;
            break;

          default:
            LogPrint(LOG_ERR, "Unsupported Voyager length code: %u", data[1]);
            break;
        }
      }
    }

    if (totalCells) {
      LogPrint(LOG_INFO, "Voyager Length: %u", totalCells);

      /* position the text and status cells */
      textCells = totalCells;
      textOffset = statusOffset = 0;
      {
        int cells = 3;
        const char *word = parameters[PARM_STATUSCELLS];
        if (word && *word) {
          int maximum = textCells / 2;
          int minimum = -maximum;
          int value;
          if (validateInteger(&value, "status cells specification", word, &minimum, &maximum)) {
            cells = value;
          }
        }

        if (cells) {
          if (cells < 0) {
            statusOffset = textCells + cells;
            cells = -cells;
          } else {
            textOffset = cells + 1;
          }
          textCells -= cells + 1;
        }
        statusCells = cells;
      }

      /* currentCells holds the status cells and the text cells.
       * We export directly to BRLTTY only the text cells.
       */
      brl->x = textCells;		/* initialize size of display */
      brl->y = 1;		/* always 1 */

      /* start the input packet monitor */
      {
        int try = 0;
        int ret;
        while (1) {
          ret = usbBeginInput(usb->device, usb->definition.inputEndpoint, 8);
          if (ret != 0) break;
          if (errno != EPIPE) break;
          if (++try > CONTROL_RETRIES) break;
          LogPrint(LOG_WARNING, "begin input (try %d) failed: %s",
                   try, strerror(errno));
        }
        if (ret == 0)
          LogPrint(LOG_ERR, "begin input error: %s", strerror(errno));
      }

      if ((currentCells = malloc(totalCells))) {
        if ((previousCells = malloc(totalCells))) {
          /* Force rewrite of display */
          memset(currentCells, 0, totalCells); /* no dots */
          memset(previousCells, 0XFF, totalCells); /* all dots */

          if (setDisplayState(1)) {
            soundBeep(200);

            {
              static const DotsTable dots
                = {0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80};
              makeOutputTable(&dots, &outputTable);
            }

            inputMode = 0;
            if (*parameters[PARM_INPUTMODE])
              validateYesNo(&inputMode, "Allow braille input",
                            parameters[PARM_INPUTMODE]);

            firstRead = 1;
            return 1;
          }

          free(previousCells);
          previousCells = NULL;
        }

        free(currentCells);
        currentCells = NULL;
      }
    }

    usbCloseChannel(usb);
    usb = NULL;
  }
  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  if (usb) {
    usbCloseChannel(usb);
    usb = NULL;
  }

  if (currentCells) {
    free(currentCells);
    currentCells = NULL;
  }

  if (previousCells) {
    free(previousCells);
    previousCells = NULL;
  }
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *cells) {
  memcpy(currentCells+statusOffset, cells, statusCells);
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  unsigned char buffer[totalCells];

  memcpy(currentCells+textOffset, brl->buffer, textCells);

  /* If content hasn't changed, do nothing. */
  if (memcmp(previousCells, currentCells, totalCells) == 0) return;

  /* remember current content */
  memcpy(previousCells, currentCells, totalCells);

  /* translate to voyager dot pattern coding */
  {
    int i;
    for (i=0; i<totalCells; i++)
      buffer[i] = outputTable[currentCells[i]];
  }

  /* The firmware supports multiples of 8 cells, so there are extra cells
   * in the firmware's imagination that don't actually exist physically.
   */
  switch (totalCells) {
    case 44: {
      /* Two ghost cells at the beginning of the display,
       * plus two more after the sixth physical cell.
       */
      unsigned char hbuf[46];
      memcpy(hbuf, buffer, 6);
      hbuf[6] = hbuf[7] = 0;
      memcpy(hbuf+8, buffer+6, 38);
      writeBraille(hbuf, 46, 2);
      break;
    }

    case 70:
      /* Two ghost cells at the beginning of the display. */
      writeBraille(buffer, 70, 2);
      break;
  }
}


/* Names and codes for display keys */

/* The top round keys behind the routing keys, numbered assuming they
 * are to be used for braille input.
 */
#define DOT1 0X01
#define DOT2 0X02
#define DOT3 0X04
#define DOT4 0X08
#define DOT5 0X10
#define DOT6 0X20
#define DOT7 0X40
#define DOT8 0X80
#define DOT_KEYS (DOT1 | DOT2 | DOT3 | DOT4 | DOT5 | DOT6 | DOT7 | DOT8)

/* The front keys. Codes are shifted by 8 bits so they can be combined
 * with the codes for the top keys.
 */
#define K_A    0X0100 /* Leftmost */
#define K_B    0X0200 /* Second from the left */
#define K_RL   0X0400 /* The round key to the left of the central pad */
#define K_UP   0X0800 /* Up position of central pad */
#define K_DOWN 0X1000 /* Down position of central pad */
#define K_RR   0X2000 /* The round key to the right of the central pad */
#define K_C    0X4000 /* Second from the right */
#define K_D    0X8000 /* Rightmost */
#define FRONT_KEYS (K_A | K_B | K_RL | K_UP | K_DOWN | K_RR | K_C | K_D)
#define SPACE_BAR (K_B | K_C)

/* OK what follows is pretty hairy. I got tired of individually maintaining
 * the sources and help files so here's my first attempt at "automatic"
 * generation of help files. This is my first shot at it, so be kind with
 * me.
 */

/* These macros include an ordering hint for the help file and the help
 * text. GENHLP is not defined during compilation, so at compilation the
 * macros are expanded in a way that just drops the help-related
 * information.
 */
#define KEY(keys, command) case keys: cmd = command; break;
#ifdef GENHLP
/* To generate the help files we do gcc -DGENHLP -E (and heavily post-process
 * the result). So these macros expand to something that is easily
 * searched/grepped for and "easily" post-processed.
 */
#define HLP(where, keys, description) <HLP> where: keys : description </HLP>
#else /* GENHLP */
#define HLP(where, keys, description)
#endif /* GENHLP */

#define HKEY(where, keys, command, description) \
  HLP(where, #keys, description) \
  KEY(keys, command)

#define PHKEY(where, prefix, keys, command, description) \
  HLP(where, prefix "+" #keys, description) \
  KEY(keys, command)

#define CKEY(where, dots, command, description) \
  HLP(where, "Chord-" #dots, description) \
  KEY(dots, command)

#define HKEY2(where, keys1, keys2, command1, command2, description) \
  HLP(where, #keys1 "/" #keys2, description) \
  KEY(keys1, command1); \
  KEY(keys2, command2)

#define PHKEY2(where, prefix, keys1, keys2, command1, command2, description) \
  HLP(where, prefix "+ " #keys1 "/" #keys2, description) \
  KEY(keys1, command1); \
  KEY(keys2, command2)

static int
brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds) {
  /* Structure to remember which keys are pressed */
  typedef struct {
    unsigned int control; /* Front and dot keys */
    unsigned char routing[MAXIMUM_CELLS];
  } Keys;

  /* Static variables to remember the state between calls. */
  /* Remember which keys are pressed.
   * Updated immediately whenever a key is pressed.
   * Cleared after analysis whenever a key is released.
   */
  static Keys activeKeys;
  static Keys pressedKeys;
  /* For a key binding that triggers two commands */
  static int pendingCommand;

  /* Ordered list of pressed routing keys by number */
  unsigned char routingKeys[MAXIMUM_CELLS];
  /* number of entries in routingKeys */
  int routingCount = 0;

  /* recognized command */
  int cmd = CMD_NOOP;
  int keyPressed = 0;

  /* read buffer: packet[0] for DOT keys, packet[1] for keys A B C D UP DOWN
   * RL RR, packet[2]-packet[7] list pressed routing keys by number, maximum
   * 6 keys, list ends with 0.
   * All 0s is sent when all keys released.
   */
  unsigned char packet[8];

  if (firstRead) {
    /* initialize state */
    firstRead = 0;
    memset(&activeKeys, 0, sizeof(activeKeys));
    memset(&pressedKeys, 0, sizeof(pressedKeys));
    pendingCommand = EOF;
  }

  if (pendingCommand != EOF) {
    cmd = pendingCommand;
    pendingCommand = EOF;
    return cmd;
  }

  {
    int size = usbReapInput(usb->device, usb->definition.inputEndpoint,
                            packet, sizeof(packet), 0, 0);
    if (size < 0) {
      if (errno == EAGAIN) {
        /* no input */
        size = 0;
      } else if (errno == ENODEV) {
        /* Display was disconnected */
        return CMD_RESTARTBRL;
      } else {
        LogPrint(LOG_ERR, "Voyager read error: %s", strerror(errno));
        firstRead = 1;
        return EOF;
      }
    } else if ((size > 0) && (size < sizeof(packet))) {
      /* The display handles read requests of only and exactly 8bytes */
      LogPrint(LOG_NOTICE, "Short read: %d", size);
      firstRead = 1;
      return EOF;
    }

    if (size == 0) {
      /* no new key */
      return EOF;
    }
  }

  /* one or more keys were pressed or released */
  {
    Keys keys;
    int i;
    memset(&keys, 0, sizeof(keys));

    /* We combine dot and front key info in keystate */
    keys.control = (packet[1] << 8) | packet[0];
    if (keys.control & ~pressedKeys.control) keyPressed = 1;
    
    for (i=2; i<8; i++) {
      unsigned char key = packet[i];
      if (!key) break;

      if ((key < 1) || (key > totalCells)) {
        LogPrint(LOG_NOTICE, "Invalid routing key number: %u", key);
        continue;
      }
      key--;

      keys.routing[key] = 1;
      if (!pressedKeys.routing[key]) keyPressed = 1;
    }

    pressedKeys = keys;
    if (keyPressed) activeKeys = keys;
  }

  {
    int key;
    for (key=0; key<totalCells; key++)
      if (activeKeys.routing[key])
        routingKeys[routingCount++] = key;
  }

  if (routingCount == 0) {
    /* No routing keys */

    if (!(activeKeys.control & ~FRONT_KEYS)) {
      /* Just front keys */

      if (cmds == CMDS_PREFS) {
	switch (activeKeys.control) {
          HKEY2(891, K_DOWN, K_UP,
                CMD_MENU_PREV_SETTING, CMD_MENU_NEXT_SETTING,
                "Select previous/next setting");
          HKEY(892, K_RL|K_RR, CMD_PREFMENU, "Exit menu");
          HKEY(892, K_RL|K_UP, CMD_PREFLOAD, "Discard changes");
          HKEY(892, K_RL|K_DOWN, CMD_PREFSAVE, "Save changes and exit menu");
	}
      }

      if (cmd == CMD_NOOP) {
	switch (activeKeys.control) {
	  HKEY2(101, K_A, K_D,
                CMD_FWINLT, CMD_FWINRT,
                "Go backward/forward one window");
	  HKEY2(501, K_RL|K_A, K_RL|K_D,
                CMD_LNBEG, CMD_LNEND,
                "Go to beginning/end of line");
	  HKEY2(501, K_RR|K_A, K_RR|K_D,
                CMD_CHRLT, CMD_CHRRT,
                "Go left/right one character");
	  HKEY2(501, K_UP|K_A, K_UP|K_D,
                CMD_FWINLTSKIP, CMD_FWINRTSKIP,
                "Go to previous/next non-blank window");
	  HKEY2(501, K_DOWN|K_A, K_DOWN|K_D,
                CMD_PRSEARCH, CMD_NXSEARCH,
                "Search screen backward/forward for cut text");

	  HKEY2(101, K_B, K_C,
                CMD_LNUP, CMD_LNDN,
                "Go up/down one line");
	  HKEY2(101, K_A|K_B, K_A|K_C,
                CMD_TOP_LEFT, CMD_BOT_LEFT,
                "Go to top-left/bottom-left corner");
	  HKEY2(101, K_D|K_B, K_D|K_C,
                CMD_TOP, CMD_BOT,
                "Go to top/bottom line");
	  HKEY2(501, K_RL|K_B, K_RL|K_C,
                CMD_ATTRUP, CMD_ATTRDN,
		"Go to previous/next line with different highlighting");
	  HKEY2(501, K_RR|K_B, K_RR|K_C,
                CMD_PRDIFLN, CMD_NXDIFLN,
                "Go to previous/next line with different content");
	  HKEY2(501, K_UP|K_B, K_UP|K_C,
                CMD_PRPGRPH, CMD_NXPGRPH,
                "Go to previous/next paragraph (blank line separation)");
	  HKEY2(501, K_DOWN|K_B, K_DOWN|K_C,
                CMD_PRPROMPT, CMD_NXPROMPT,
                "Go to previous/next prompt (same prompt as current line)");

	  HKEY(101, K_RL, CMD_BACK, 
               "Go back (undo unexpected cursor tracking motion)");
	  HKEY(101, K_RR, CMD_HOME, "Go to cursor");
	  HKEY(101, K_RL|K_RR, CMD_CSRTRK, "Cursor tracking (toggle)");

	  HKEY2(101, K_UP, K_DOWN,
                VAL_PASSKEY + VPK_CURSOR_UP,
                VAL_PASSKEY + VPK_CURSOR_DOWN,
                "Move cursor up/down (arrow keys)");
	  HKEY(210, K_RL|K_UP, CMD_DISPMD, "Show attributes (toggle)");
	  HKEY(210, K_RL|K_DOWN, CMD_SIXDOTS, "Six dots (toggle)");
	  HKEY(210, K_RR|K_UP, CMD_AUTOREPEAT, "Autorepeat (toggle)");
	  HKEY(210, K_RR|K_DOWN, CMD_AUTOSPEAK, "Autospeak (toggle)");

	  HKEY(602, K_B|K_C, VAL_PASSDOTS+0, "Space bar")
	  HKEY(302, K_A|K_D, CMD_CSRJMP_VERT,
	       "Route cursor to current line");
	}
      }
    } else if ((!inputMode || (activeKeys.control & SPACE_BAR)) &&
               !(activeKeys.control & FRONT_KEYS & ~SPACE_BAR)) {
      /* Dots combined with B or C or both but no other front keys.
       * This is a chorded character typed in braille.
       */
      switch (activeKeys.control & DOT_KEYS) {
        HLP(601, "Chord-1478", "Input mode (toggle)")
        case DOT1|DOT4|DOT7|DOT8:
          if (!keyPressed) cmd = CMD_NOOP | ((inputMode = !inputMode)? VAL_TOGGLE_ON: VAL_TOGGLE_OFF);
          break;

	CKEY(210, DOT1, CMD_ATTRVIS, "Attribute underlining (toggle)");
	CKEY(610, DOT1|DOT2, VAL_PASSKEY + VPK_BACKSPACE, "Backspace key");
	CKEY(210, DOT1|DOT4, CMD_CSRVIS, "Cursor visibility (toggle)");
	CKEY(610, DOT1|DOT4|DOT5, VAL_PASSKEY + VPK_DELETE, "Delete key");
	CKEY(610, DOT1|DOT5, VAL_PASSKEY + VPK_ESCAPE, "Escape key");
	CKEY(210, DOT1|DOT2|DOT4, CMD_FREEZE, "Freeze screen (toggle)");
	CKEY(201, DOT1|DOT2|DOT5, CMD_HELP, "Help screen (toggle)");
	CKEY(610, DOT2|DOT4, VAL_PASSKEY + VPK_INSERT, "Insert key");
	CKEY(201, DOT1|DOT2|DOT3, CMD_LEARN, "Learn mode (toggle)");
	case DOT1|DOT2|DOT3|DOT4|DOT5|DOT6|DOT7|DOT8:
	CKEY(205, DOT1|DOT3|DOT4, CMD_PREFMENU, "Preferences menu (toggle)");
	CKEY(408, DOT1|DOT2|DOT3|DOT4, CMD_PASTE, "Paste cut text");
	CKEY(206, DOT1|DOT2|DOT3|DOT5, CMD_PREFLOAD, "Reload preferences from disk");
	CKEY(201, DOT2|DOT3|DOT4, CMD_INFO, "Status line (toggle)");
	CKEY(610, DOT2|DOT3|DOT4|DOT5, VAL_PASSKEY + VPK_TAB, "Tab key");
	CKEY(206, DOT2|DOT4|DOT5|DOT6, CMD_PREFSAVE, "Write preferences to disk");
	CKEY(610, DOT4|DOT6, VAL_PASSKEY + VPK_RETURN, "Return key");
	CKEY(610, DOT2, VAL_PASSKEY+VPK_PAGE_UP, "Page up");
	CKEY(610, DOT5, VAL_PASSKEY+VPK_PAGE_DOWN, "Page down");
	CKEY(610, DOT3, VAL_PASSKEY+VPK_HOME, "Home key");
	CKEY(610, DOT6, VAL_PASSKEY+VPK_END, "End key");
	CKEY(610, DOT7, VAL_PASSKEY+VPK_CURSOR_LEFT, "Left arrow");
	CKEY(610, DOT8, VAL_PASSKEY+VPK_CURSOR_RIGHT, "Right arrow");
      }
    } else if (!(activeKeys.control & ~DOT_KEYS)) {
      /* Just dot keys */
      /* This is a character typed in braille */
      cmd = VAL_PASSDOTS;
      if (activeKeys.control & DOT1) cmd |= B1;
      if (activeKeys.control & DOT2) cmd |= B2;
      if (activeKeys.control & DOT3) cmd |= B3;
      if (activeKeys.control & DOT4) cmd |= B4;
      if (activeKeys.control & DOT5) cmd |= B5;
      if (activeKeys.control & DOT6) cmd |= B6;
      if (activeKeys.control & DOT7) cmd |= B7;
      if (activeKeys.control & DOT8) cmd |= B8;
    }
  } else if (!activeKeys.control) {
    /* Just routing keys */
    if (routingCount == 1) {
      if (IS_TEXT_KEY(routingKeys[0])) {
        HLP(301,"CRt#", "Route cursor to cell")
        cmd = CR_ROUTE + routingKeys[0] - textOffset;
      } else {
        int key = statusOffset? totalCells - 1 - routingKeys[0]:
                                routingKeys[0] - statusOffset;
        switch (key) {
          case 0:
            HLP(881, "CRs1", "Help screen (toggle)")
            cmd = CMD_HELP;
            break;

          case 1:
            HLP(882, "CRs2", "Preferences menu (toggle)")
            cmd = CMD_PREFMENU;
            break;

          case 2:
            HLP(883, "CRs3", "Learn mode (toggle)")
            cmd = CMD_LEARN;
            break;

          case 3:
            HLP(884, "CRs4", "Route cursor to current line")
            cmd = CMD_CSRJMP_VERT;
            break;
        }
      }
    } else if ((routingCount == 2) &&
               IS_TEXT_RANGE(routingKeys[0], routingKeys[1])) {
      HLP(405, "CRtx+CRty", "Cut text from x through y")
      cmd = CR_CUTBEGIN + routingKeys[0] - textOffset;
      pendingCommand = CR_CUTLINE + routingKeys[1] - textOffset;
    }
  } else if (activeKeys.control & (K_UP|K_RL|K_RR)) {
    /* Some routing keys combined with UP RL or RR (actually any key
     * combination that has at least one of those).
     * Treated special because we use absolute routing key numbers
     * (counting the status cell keys).
     */
    if (routingCount == 1) {
      switch (activeKeys.control) {
        case K_UP:
          HLP(692, "UP+ CRa<CELLS-1>/CRa<CELLS>",
              "Switch to previous/next virtual console")
          if (routingKeys[0] == totalCells-1) {
            cmd = CMD_SWITCHVT_NEXT;
          } else if (routingKeys[0] == totalCells-2) {
            cmd = CMD_SWITCHVT_PREV;
          } else {
            HLP(691, "UP+CRa#", "Switch to virtual console #")
            cmd = CR_SWITCHVT + routingKeys[0];
          }
          break;

        PHKEY(501,"CRa#", K_RL,
              CR_SETMARK + routingKeys[0],
              "Remember current position as mark #");
        PHKEY(501,"CRa#", K_RR,
              CR_GOTOMARK + routingKeys[0],
              "Go to mark #");
      }
    }
  } else if ((routingCount == 1) && IS_TEXT_KEY(routingKeys[0])) {
    /* One text routing key with some other keys */
    switch (activeKeys.control) {
      PHKEY(501, "CRt#", K_DOWN,
            CR_SETLEFT + routingKeys[0] - textOffset,
            "Go right # cells");
      PHKEY(401, "CRt#", K_A,
            CR_CUTBEGIN + routingKeys[0] - textOffset,
            "Mark beginning of region to cut");
      PHKEY(401, "CRt#", K_A|K_B,
            CR_CUTAPPEND + routingKeys[0] - textOffset,
            "Mark beginning of cut region for append");
      PHKEY(401, "CRt#", K_D,
            CR_CUTRECT + routingKeys[0] - textOffset,
            "Mark bottom-right of rectangular region and cut");
      PHKEY(401, "CRt#", K_D|K_C,
            CR_CUTLINE + routingKeys[0] - textOffset,
            "Mark end of linear region and cut");
      PHKEY2(501, "CRt#", K_B, K_C,
             CR_PRINDENT + routingKeys[0] - textOffset,
             CR_NXINDENT + routingKeys[0] - textOffset,
             "Go to previous/next line indented no more than #");
    }
  }

  if (keyPressed) {
    /* key was pressed, start the autorepeat delay */
    cmd |= VAL_REPEAT_DELAY;
  } else {
    /* key was released, clear state */
    memset(&activeKeys, 0, sizeof(activeKeys));
  }

  return cmd;
}

/* Voltage: from 0->300V to 255->200V.
 * Presumably this is voltage for dot firmness.
 * Presumably 0 makes dots hardest, 255 makes them softest.
 * We are told 265V is normal operating voltage but we don't know the scale.
 */
static void
brl_firmness (BrailleDisplay *brl, BrailleFirmness setting) {
  unsigned char value = 0XFF - (setting * 0XFF / BF_MAXIMUM);
  LogPrint(LOG_DEBUG, "Setting voltage: %02X", value);
  tellDisplay(CTL_REQ_SET_DISPLAY_VOLTAGE, value, 0, NULL, 0);

  /* log the display voltage */
  {
    unsigned char buffer[2];
    int size = askDisplay(CTL_REQ_GET_DISPLAY_VOLTAGE, 0, 0, buffer, sizeof(buffer));
    if (size != -1)
      LogBytes("Display Voltage", buffer, size);
  }

  /* log the display current */
  {
    unsigned char buffer[2];
    int size = askDisplay(CTL_REQ_GET_DISPLAY_CURRENT, 0, 0, buffer, sizeof(buffer));
    if (size != -1)
      LogBytes("Display Current", buffer, size);
  }
}

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
#define VERSION "0.2"
#define DATE "August, 2004"
#define COPYRIGHT "Copyright Samuel Thibault <samuel.thibault@ens-lyon.org>"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "Programs/brl.h"
#include "Programs/misc.h"
#include "Programs/scr.h"
#include "Programs/message.h"

#include <errno.h>
#include <ctype.h>
#include <locale.h>

#include <stdarg.h>
#if defined(HAVE_PKG_CURSES)
#include <curses.h>
#elif defined(HAVE_PKG_NCURSES)
#include <ncurses.h>
#else /* HAVE_PKG_ */
#error unknown curses package
#endif /* HAVE_PKG_ */

#ifdef HAVE_ICONV_H
#include <iconv.h>
static iconv_t conversionDescriptor = NULL;
#endif /* HAVE_ICONV_H */

typedef enum {
  PARM_TERM,
  PARM_BAUD,
  PARM_LINES,
  PARM_COLS,
#ifdef HAVE_ICONV_H
  PARM_CSET,
#endif /* HAVE_ICONV_H */
  PARM_LOCALE
} DriverParameter;

#ifdef HAVE_ICONV_H
#define BRLPARMS "term", "baud", "lines", "cols", "cset", "locale"
#else /* HAVE_ICONV_H */
#define BRLPARMS "term", "baud", "lines", "cols", "locale"
#endif /* HAVE_ICONV_H */

#define BRL_HAVE_KEY_CODES
#include "Programs/brl_driver.h"
#include "braille.h"
#include "Programs/serial.h"

#define MAX_WINDOW_HEIGHT 3
#define MAX_WINDOW_WIDTH 80
#define MAX_WINDOW_SIZE (MAX_WINDOW_HEIGHT * MAX_WINDOW_WIDTH)

static int ttyDescriptor = -1;
static struct termios ttySettings;
static FILE *ttyStream = NULL;
static SCREEN *ttyScreen = NULL;
static char *classificationLocale = NULL;

static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "TTY Driver: version " VERSION " (" DATE ")");
  LogPrint(LOG_INFO,   COPYRIGHT);
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  char *ttyType = "vt100";
  int ttyBaud = 9600;
  int windowHeight = 1;
  int windowWidth = 40;

#ifdef HAVE_ICONV_H
  char *characterSet = "ISO8859-1";
#endif /* HAVE_ICONV_H */

  if (*parameters[PARM_TERM])
    ttyType = parameters[PARM_TERM];

  {
    int baud = ttyBaud;
    if (validateSerialBaud(&baud, "TTY baud", parameters[PARM_BAUD], NULL))
      ttyBaud = baud;
  }

  {
    static const int minimum = 1;
    static const int maximum = MAX_WINDOW_HEIGHT;
    int height = windowHeight;
    if (validateInteger(&height, "window height (lines)",
                        parameters[PARM_LINES], &minimum, &maximum))
      windowHeight = height;
  }

  {
    static const int minimum = 1;
    static const int maximum = MAX_WINDOW_WIDTH;
    int width = windowWidth;
    if (validateInteger(&width, "window width (columns)",
                        parameters[PARM_COLS], &minimum, &maximum))
      windowWidth = width;
  }

#ifdef HAVE_ICONV_H
  if (*parameters[PARM_CSET])
    characterSet = parameters[PARM_CSET];
#endif /* HAVE_ICONV_H */

  if (*parameters[PARM_LOCALE])
    classificationLocale = parameters[PARM_LOCALE];

  if (
#ifdef HAVE_ICONV_H
      (conversionDescriptor = iconv_open(characterSet, "ISO8859-1")) != (iconv_t)-1
#else /* HAVE_ICONV_H */
      1
#endif /* HAVE_ICONV_H */
      ) {
    if (openSerialDevice(device, &ttyDescriptor, &ttySettings)) {
      struct termios newSettings;
      initializeSerialAttributes(&newSettings);

      if (restartSerialDevice(ttyDescriptor, &newSettings, ttyBaud)) {
        if ((ttyStream = fdopen(ttyDescriptor, "a+"))) {
          if ((ttyScreen = newterm(ttyType, ttyStream, ttyStream))) {
            cbreak();
            noecho();
            nonl();

            nodelay(stdscr, TRUE);
            intrflush(stdscr, FALSE);
            keypad(stdscr, TRUE);

            clear();
            refresh();

            brl->x = windowWidth;
            brl->y = windowHeight; 

            LogPrint(LOG_INFO, "TTY: type=%s baud=%d size=%dx%d",
                     ttyType, ttyBaud, windowWidth, windowHeight);
            return 1;
          } else {
            LogError("newterm");
          }

          /* fclose() closes the file descriptor */
          putSerialAttributes(ttyDescriptor, &ttySettings);
          ttyDescriptor = -1;

          fclose(ttyStream);
          ttyStream = NULL;
        } else {
          LogError("fdopen");
        }

        if (ttyDescriptor != -1) {
          putSerialAttributes(ttyDescriptor, &ttySettings);
        }
      }

      if (ttyDescriptor != -1) {
        close(ttyDescriptor);
        ttyDescriptor = -1;
      }
    }

#ifdef HAVE_ICONV_H
    iconv_close(conversionDescriptor);
#endif /* HAVE_ICONV_H */
  } else {
    LogError("iconv_open");
  }

#ifdef HAVE_ICONV_H
  conversionDescriptor = NULL;
#endif /* HAVE_ICONV_H */

  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  if (ttyScreen) {
    endwin();
    delscreen(ttyScreen);
    ttyScreen = NULL;
  }

  if (ttyStream) {
    putSerialAttributes(ttyDescriptor, &ttySettings);
    ttyDescriptor = -1;

    fclose(ttyStream);
    ttyStream = NULL;
  }

  if (ttyDescriptor != -1) {
    putSerialAttributes(ttyDescriptor, &ttySettings);
    close(ttyDescriptor);
    ttyDescriptor = -1;
  }

#ifdef HAVE_ICONV_H
  if (conversionDescriptor) {
    iconv_close(conversionDescriptor);
    conversionDescriptor = NULL;
  }
#endif /* HAVE_ICONV_H */
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  static unsigned char previousContent[MAX_WINDOW_SIZE];
  char *previousLocale;

  if (memcmp(previousContent, brl->buffer, brl->x*brl->y) == 0) return;
  memcpy(previousContent, brl->buffer, brl->x*brl->y);

  if (classificationLocale) {
    previousLocale = setlocale(LC_CTYPE, NULL);
    setlocale(LC_CTYPE, classificationLocale);
  } else {
    previousLocale = NULL;
  }

  clear();
  {
    int row;
    for (row=0; row<brl->y; row++) {
      int column;
      for (column=0; column<brl->x; column++) {
        char c = brl->buffer[row*brl->x+column];
#ifdef HAVE_ICONV_H
        char *pc = &c;
        size_t sc = 1;
        char d;
        char *pd = &d;
        size_t sd = 1;
        if (iconv(conversionDescriptor, &pc, &sc, &pd, &sd) >= 0)
          addch(d);
        else
#endif /* HAVE_ICONV_H */
          addch(c);
      }

      addch('\n');
    }
  }

  if (previousLocale) setlocale(LC_CTYPE, previousLocale);
/*move(y,x);*/
  refresh();
}

static void
brl_writeStatus(BrailleDisplay *brl, const unsigned char *cells) {
}

int
brl_keyToCommand (BrailleDisplay *brl, BRL_DriverCommandContext context, int key) {
#define KEY(key,cmd) case (key): return (cmd)
  switch (key) {
    KEY(EOF, EOF);
    default:
      if (key <= 0XFF) return BRL_BLK_PASSCHAR|key;
      LogPrint(LOG_WARNING, "Unknown key: %d", key);
      return BRL_CMD_NOOP;

    KEY(KEY_LEFT, BRL_CMD_FWINLT);
    KEY(KEY_RIGHT, BRL_CMD_FWINRT);
    KEY(KEY_UP, BRL_CMD_LNUP);
    KEY(KEY_DOWN, BRL_CMD_LNDN);

    KEY(KEY_PPAGE, BRL_CMD_TOP);
    KEY(KEY_NPAGE, BRL_CMD_BOT);
    KEY(KEY_HOME, BRL_CMD_TOP_LEFT);
    KEY(KEY_END, BRL_CMD_BOT_LEFT);
    KEY(KEY_IC, BRL_CMD_HOME);
    KEY(KEY_DC, BRL_CMD_CSRTRK);

    KEY(KEY_F(1), BRL_CMD_HELP);
    KEY(KEY_F(2), BRL_CMD_LEARN);
    KEY(KEY_F(3), BRL_CMD_INFO);
    KEY(KEY_F(4), BRL_CMD_PREFMENU);

    KEY(KEY_F(5), BRL_CMD_PRDIFLN);
    KEY(KEY_F(6), BRL_CMD_NXDIFLN);
    KEY(KEY_F(7), BRL_CMD_ATTRUP);
    KEY(KEY_F(8), BRL_CMD_ATTRDN);

    KEY(KEY_F(9), BRL_CMD_LNBEG);
    KEY(KEY_F(10), BRL_CMD_CHRLT);
    KEY(KEY_F(11), BRL_CMD_CHRRT);
    KEY(KEY_F(12), BRL_CMD_LNEND);

    KEY(KEY_F(17), BRL_CMD_PRPROMPT);
    KEY(KEY_F(18), BRL_CMD_NXPROMPT);
    KEY(KEY_F(19), BRL_CMD_PRPGRPH);
    KEY(KEY_F(20), BRL_CMD_NXPGRPH);

    KEY(KEY_BACKSPACE, BRL_BLK_PASSKEY|BRL_KEY_BACKSPACE);
  }
#undef KEY
}

static int
brl_readKey (BrailleDisplay *brl) {
  int key = getch();
  if (key == ERR) return EOF;
  LogPrint(LOG_DEBUG, "key %d", key);
  return key;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  int command = brl_keyToCommand(brl, context, brl_readKey(brl));
  if (command != EOF) LogPrint(LOG_DEBUG, "cmd %04X", command);
  return command;
}

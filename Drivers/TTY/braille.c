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

#define TT_VERSION "0.2"
#define TT_DATE "August, 2004"
#define TT_COPYRIGHT "Copyright Samuel Thibault <samuel.thibault@ens-lyon.org>"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include <limits.h>
#ifndef MB_MAX_LEN
#define MB_MAX_LEN 16
#endif /* MB_MAX_LEN */

#ifdef HAVE_ICONV_H
#include <iconv.h>
static iconv_t conversionDescriptor = NULL;
#endif /* HAVE_ICONV_H */

#if defined(HAVE_PKG_CURSES)
#define USE_CURSES
#include <stdarg.h>
#include <curses.h>
#elif defined(HAVE_PKG_NCURSES)
#define USE_CURSES
#include <ncurses.h>
#elif defined(HAVE_PKG_NCURSESW)
#define USE_CURSES
#include <ncurses.h>
#else /* HAVE_PKG_ */
#warning curses package either unspecified or unsupported
#define addstr(string) puts(string)
#define addch(character) putchar(character)
#define getch() getchar()
#endif /* HAVE_PKG_CURSES */

#include "Programs/misc.h"

#ifdef USE_CURSES
#define BRLPARM_TERM "term",
#else /* USE_CURSES */
#define BRLPARM_TERM
#endif /* USE_CURSES */

#ifdef HAVE_ICONV_H
#define BRLPARM_CHARSET "charset",
#else /* HAVE_ICONV_H */
#define BRLPARM_CHARSET
#endif /* HAVE_ICONV_H */

typedef enum {
  PARM_BAUD,

#ifdef USE_CURSES
  PARM_TERM,
#endif /* USE_CURSES */
  PARM_HEIGHT,
  PARM_WIDTH,

#ifdef HAVE_ICONV_H
  PARM_CHARSET,
#endif /* HAVE_ICONV_H */

  PARM_LOCALE
} DriverParameter;
#define BRLPARMS "baud", BRLPARM_TERM "height", "width", BRLPARM_CHARSET "locale"

#define BRL_HAVE_KEY_CODES
#define BRL_HAVE_VISUAL_DISPLAY
#include "Programs/brl_driver.h"
#include "braille.h"
#include "Programs/serial.h"

#define MAX_WINDOW_HEIGHT 3
#define MAX_WINDOW_WIDTH 80
#define MAX_WINDOW_SIZE (MAX_WINDOW_HEIGHT * MAX_WINDOW_WIDTH)

static SerialDevice *ttyDevice = NULL;
static FILE *ttyStream = NULL;
static char *classificationLocale = NULL;

#ifdef USE_CURSES
static SCREEN *ttyScreen = NULL;
#endif /* USE_CURSES */

static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "TTY Driver: version " TT_VERSION " (" TT_DATE ")");
  LogPrint(LOG_INFO,   "   "TT_COPYRIGHT);
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  int ttyBaud = 9600;
  char *ttyType = "vt100";
  int windowHeight = 1;
  int windowWidth = 40;

#ifdef HAVE_ICONV_H
  char *characterSet = "ISO8859-1";
#endif /* HAVE_ICONV_H */

  {
    int baud = ttyBaud;
    if (serialValidateBaud(&baud, "TTY baud", parameters[PARM_BAUD], NULL))
      ttyBaud = baud;
  }

#ifdef USE_CURSES
  if (*parameters[PARM_TERM])
    ttyType = parameters[PARM_TERM];
#endif /* USE_CURSES */

  {
    static const int minimum = 1;
    static const int maximum = MAX_WINDOW_HEIGHT;
    int height = windowHeight;
    if (validateInteger(&height, "window height (lines)",
                        parameters[PARM_HEIGHT], &minimum, &maximum))
      windowHeight = height;
  }

  {
    static const int minimum = 1;
    static const int maximum = MAX_WINDOW_WIDTH;
    int width = windowWidth;
    if (validateInteger(&width, "window width (columns)",
                        parameters[PARM_WIDTH], &minimum, &maximum))
      windowWidth = width;
  }

#ifdef HAVE_ICONV_H
  if (*parameters[PARM_CHARSET])
    characterSet = parameters[PARM_CHARSET];
#endif /* HAVE_ICONV_H */

  if (*parameters[PARM_LOCALE])
    classificationLocale = parameters[PARM_LOCALE];

#ifdef HAVE_ICONV_H
  if ((conversionDescriptor = iconv_open(characterSet, "ISO8859-1")) != (iconv_t)-1) {
#endif /* HAVE_ICONV_H */
    if ((ttyDevice = serialOpenDevice(device))) {
      if (serialRestartDevice(ttyDevice, ttyBaud)) {
        if ((ttyStream = serialGetStream(ttyDevice))) {
#ifdef USE_CURSES
          if ((ttyScreen = newterm(ttyType, ttyStream, ttyStream))) {
            cbreak();
            noecho();
            nonl();

            nodelay(stdscr, TRUE);
            intrflush(stdscr, FALSE);
            keypad(stdscr, TRUE);

            clear();
            refresh();
#endif /* USE_CURSES */

            brl->x = windowWidth;
            brl->y = windowHeight; 

            LogPrint(LOG_INFO, "TTY: type=%s baud=%d size=%dx%d",
                     ttyType, ttyBaud, windowWidth, windowHeight);
            return 1;
#ifdef USE_CURSES
          } else {
            LogError("newterm");
          }
#endif /* USE_CURSES */

          ttyStream = NULL;
        }
      }

      serialCloseDevice(ttyDevice);
      ttyDevice = NULL;
    }

#ifdef HAVE_ICONV_H
    iconv_close(conversionDescriptor);
  } else {
    LogError("iconv_open");
  }
  conversionDescriptor = NULL;
#endif /* HAVE_ICONV_H */

  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
#ifdef USE_CURSES
  if (ttyScreen) {
    endwin();
    delscreen(ttyScreen);
    ttyScreen = NULL;
  }
#endif /* USE_CURSES */

  if (ttyDevice) {
    ttyStream = NULL;
    serialCloseDevice(ttyDevice);
    ttyDevice = NULL;
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
}

static void
brl_writeVisual (BrailleDisplay *brl) {
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

#ifdef USE_CURSES
  clear();
#endif /* USE_CURSES */
  {
    int row;
    for (row=0; row<brl->y; row++) {
      int column;
      for (column=0; column<brl->x; column++) {
        char c = brl->buffer[row*brl->x+column];
#ifdef HAVE_ICONV_H
        char *pc = &c;
        size_t sc = 1;
        char d[MB_MAX_LEN+1];
        char *pd = d;
        size_t sd = MB_MAX_LEN;
        if (iconv(conversionDescriptor, &pc, &sc, &pd, &sd) >= 0) {
          *pd = 0;
          addstr(d);
	} else
#endif /* HAVE_ICONV_H */
          addch(c);
      }

      addch('\n');
    }
  }

  if (previousLocale) setlocale(LC_CTYPE, previousLocale);
#ifdef USE_CURSES
/*move(y,x);*/
  refresh();
#endif /* USE_CURSES */
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

#ifdef USE_CURSES
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
#endif /* USE_CURSES */
  }
#undef KEY
}

static int
brl_readKey (BrailleDisplay *brl) {
  int key = getch();

#ifdef USE_CURSES
  if (key == ERR) return EOF;
#endif /* USE_CURSES */

  LogPrint(LOG_DEBUG, "key %d", key);
  return key;
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  int command = brl_keyToCommand(brl, context, brl_readKey(brl));
  if (command != EOF) LogPrint(LOG_DEBUG, "cmd %04X", command);
  return command;
}

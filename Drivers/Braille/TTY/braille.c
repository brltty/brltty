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

#include "prologue.h"

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
#include <ncursesw/ncurses.h>
#else /* HAVE_PKG_ */
#warning curses package either unspecified or unsupported
#define addstr(string) serialWriteData(ttyDevice, string, strlen(string))
#define addch(character) do { unsigned char __c = (character); serialWriteData(ttyDevice, &__c, 1); } while(0)
#define getch() my_getch()
#endif /* HAVE_PKG_CURSES */

#include "log.h"
#include "parse.h"
#include "charset.h"

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

  PARM_LINES,
  PARM_COLUMNS,

#ifdef HAVE_ICONV_H
  PARM_CHARSET,
#endif /* HAVE_ICONV_H */

  PARM_LOCALE
} DriverParameter;
#define BRLPARMS "baud", BRLPARM_TERM "lines", "columns", BRLPARM_CHARSET "locale"

#define BRL_HAVE_KEY_CODES
#include "brl_driver.h"
#include "braille.h"
#include "io_serial.h"

#define MAX_WINDOW_LINES 3
#define MAX_WINDOW_COLUMNS 80
#define MAX_WINDOW_SIZE (MAX_WINDOW_LINES * MAX_WINDOW_COLUMNS)

static SerialDevice *ttyDevice = NULL;
static FILE *ttyStream = NULL;
static char *classificationLocale = NULL;

#ifdef USE_CURSES
static SCREEN *ttyScreen = NULL;
#else /* USE_CURSES */
static inline int
my_getch (void) {
  unsigned char c;
  if (serialReadData(ttyDevice, &c, 1, 0, 0) == 1) return c;
  return EOF;
}
#endif /* USE_CURSES */

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  unsigned int ttyBaud = 9600;
  char *ttyType = "vt100";
  int windowLines = 1;
  int windowColumns = 40;

#ifdef HAVE_ICONV_H
  const char *characterSet = getLocaleCharset();
#endif /* HAVE_ICONV_H */

  if (!isSerialDevice(&device)) {
    unsupportedDevice(device);
    return 0;
  }

  {
    unsigned int baud = ttyBaud;
    if (serialValidateBaud(&baud, "TTY baud", parameters[PARM_BAUD], NULL))
      ttyBaud = baud;
  }

#ifdef USE_CURSES
  if (*parameters[PARM_TERM])
    ttyType = parameters[PARM_TERM];
#endif /* USE_CURSES */

  {
    static const int minimum = 1;
    static const int maximum = MAX_WINDOW_LINES;
    int lines = windowLines;
    if (validateInteger(&lines, parameters[PARM_LINES], &minimum, &maximum)) {
      windowLines = lines;
    } else {
      logMessage(LOG_WARNING, "%s: %s", "invalid line count", parameters[PARM_LINES]);
    }
  }

  {
    static const int minimum = 1;
    static const int maximum = MAX_WINDOW_COLUMNS;
    int columns = windowColumns;
    if (validateInteger(&columns, parameters[PARM_COLUMNS], &minimum, &maximum)) {
      windowColumns = columns;
    } else {
      logMessage(LOG_WARNING, "%s: %s", "invalid column count", parameters[PARM_COLUMNS]);
    }
  }

#ifdef HAVE_ICONV_H
  if (*parameters[PARM_CHARSET])
    characterSet = parameters[PARM_CHARSET];
#endif /* HAVE_ICONV_H */

  if (*parameters[PARM_LOCALE])
    classificationLocale = parameters[PARM_LOCALE];

#ifdef HAVE_ICONV_H
  if ((conversionDescriptor = iconv_open(characterSet, "WCHAR_T")) != (iconv_t)-1) {
#endif /* HAVE_ICONV_H */
    if ((ttyDevice = serialOpenDevice(device))) {
      if (serialRestartDevice(ttyDevice, ttyBaud)) {
#ifdef USE_CURSES
        if ((ttyStream = serialGetStream(ttyDevice))) {
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

            brl->textColumns = windowColumns;
            brl->textRows = windowLines; 

            logMessage(LOG_INFO, "TTY: type=%s baud=%u size=%dx%d",
                       ttyType, ttyBaud, windowColumns, windowLines);
            return 1;
#ifdef USE_CURSES
          } else {
            logSystemError("newterm");
          }

          ttyStream = NULL;
        }
#endif /* USE_CURSES */
      }

      serialCloseDevice(ttyDevice);
      ttyDevice = NULL;
    }

#ifdef HAVE_ICONV_H
    iconv_close(conversionDescriptor);
  } else {
    logSystemError("iconv_open");
  }
  conversionDescriptor = NULL;
#endif /* HAVE_ICONV_H */

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
#ifdef USE_CURSES
  if (ttyScreen) {
    endwin();
#ifndef __MINGW32__
    delscreen(ttyScreen);
#endif /* __MINGW32__ */
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
writeText (const wchar_t *buffer, int columns) {
  int column;
  for (column=0; column<columns; column++) {
    wchar_t c = buffer[column];
#ifdef HAVE_ICONV_H
    char *pc = (char*) &c;
    size_t sc = sizeof(wchar_t);
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
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  static unsigned char previousContent[MAX_WINDOW_SIZE];
  static int previousCursor = -1;
  char *previousLocale;

  if (!cellsHaveChanged(previousContent, brl->buffer, brl->textColumns*brl->textRows, NULL, NULL, NULL) &&
      (brl->cursor == previousCursor)) {
    return 1;
  }

  previousCursor = brl->cursor;

  if (classificationLocale) {
    previousLocale = setlocale(LC_CTYPE, NULL);
    setlocale(LC_CTYPE, classificationLocale);
  } else {
    previousLocale = NULL;
  }

#ifdef USE_CURSES
  clear();
#else /* USE_CURSES */
  addstr("\r\n");
#endif /* USE_CURSES */

  {
    int row;
    for (row=0; row<brl->textRows; row++) {
      writeText(&text[row*brl->textColumns], brl->textColumns);
      if (row < brl->textRows-1)
        addstr("\r\n");
    }
  }

#ifdef USE_CURSES
  if ((brl->cursor >= 0) && (brl->cursor < (brl->textColumns * brl->textRows)))
    move(brl->cursor/brl->textColumns, brl->cursor%brl->textColumns);
  else
    move(brl->textRows, 0);
  refresh();
#else /* USE_CURSES */
  if ((brl->textRows == 1) && (brl->cursor >= 0) && (brl->cursor < brl->textColumns)) {
    addch('\r');
    writeText(text, brl->cursor);
  } else {
    addstr("\r\n");
  }
#endif /* USE_CURSES */

  if (previousLocale) setlocale(LC_CTYPE, previousLocale);
  return 1;
}

int
brl_keyToCommand (BrailleDisplay *brl, KeyTableCommandContext context, int key) {
#define KEY(key,cmd) case (key): return (cmd)
  switch (key) {
    KEY(EOF, EOF);
    default:
      if (key <= 0XFF) return BRL_BLK_PASSCHAR|key;
      logMessage(LOG_WARNING, "Unknown key: %d", key);
      return BRL_CMD_NOOP;

#ifdef USE_CURSES
    KEY(KEY_LEFT, BRL_CMD_FWINLT);
    KEY(KEY_RIGHT, BRL_CMD_FWINRT);
    KEY(KEY_UP, BRL_CMD_LNUP);
    KEY(KEY_DOWN, BRL_CMD_LNDN);

    KEY(KEY_HOME, BRL_CMD_TOP_LEFT);
    KEY(KEY_A1, BRL_CMD_TOP_LEFT);
    KEY(KEY_END, BRL_CMD_BOT_LEFT);
    KEY(KEY_C1, BRL_CMD_BOT_LEFT);
    KEY(KEY_IC, BRL_CMD_HOME);
    KEY(KEY_B2, BRL_CMD_HOME);
    KEY(KEY_DC, BRL_CMD_CSRTRK);
    KEY(KEY_PPAGE, BRL_CMD_TOP);
    KEY(KEY_A3, BRL_CMD_TOP);
    KEY(KEY_NPAGE, BRL_CMD_BOT);
    KEY(KEY_C3, BRL_CMD_BOT);

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

  logMessage(LOG_DEBUG, "key %d", key);
  return key;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  int command = brl_keyToCommand(brl, context, brl_readKey(brl));
  if (command != EOF) logMessage(LOG_DEBUG, "cmd %04X", command);
  return command;
}

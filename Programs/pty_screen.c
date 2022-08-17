/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2022 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* unimplemented output actions
 * cbt=\E[Z
 * cr=\r
 * cuu1=\EM
 * ht=^I
 * hts=\EH
 * ind=\n
 * is2=\E)0
 * nel=\EE
 * op=\E[39;49m
 * rc=\E8
 * ri=\EM
 * rmir=\E[4l
 * rs2=\Ec
 * sc=\E7
 * smir=\E[4h
 * tbc=\E[3g
 */

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "strfmt.h"
#include "pty_screen.h"
#include "get_curses.h"
#include "ascii.h"

static const unsigned char screenLogLevel = LOG_NOTICE;
static unsigned char logOutputActions = 0;
static unsigned char logUnexpectedOutput = 0;
static unsigned char logInsertedBytes = 0;

void
ptyLogOutputActions (int yes) {
  logOutputActions = yes;
}

void
ptyLogUnexpectedOutput (int yes) {
  logUnexpectedOutput = yes;
}

void
ptyLogInsertedBytes (int yes) {
  logInsertedBytes = yes;
}

static const char ptyScreenType[] = "vt100";

const char *
ptyGetScreenType (void) {
  return ptyScreenType;
}

static unsigned char keypadTransmitMode = 0;
static unsigned char bracketedPasteMode = 0;

static int scrollRegionTop;
static int scrollRegionBottom;

static unsigned char hasColor = 0;
static unsigned char foregroundColor;
static unsigned char backgroundColor;

void
ptyBeginScreen (void) {
  initscr();
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);

  cbreak();
  noecho();
  scrollok(stdscr, TRUE);

  keypadTransmitMode = 0;
  bracketedPasteMode = 0;

  scrollRegionTop = getbegy(stdscr);
  scrollRegionBottom = getmaxy(stdscr) - 1;

  if ((hasColor = has_colors())) {
    start_color();
  }
}

void
ptyEndScreen (void) {
  endwin();
}

void
ptyRefreshScreen (void) {
  refresh();
}

void
ptyProcessInputCharacter (int fd) {
  int character = getch();
  const char *sequence = NULL;
  char buffer[0X20];

  if (character < 0X100) {
    buffer[0] = character;
    buffer[1] = 0;
    sequence = buffer;
  } else {
    #define KEY(name, seq) case KEY_##name: sequence = seq; break;
    switch (character) {
      KEY(BACKSPACE, "\x7F")
      KEY(BTAB     , "\x1B[Z")
      KEY(B2       , "\x1B[G")

      KEY(UP       , "\x1B[A")
      KEY(DOWN     , "\x1B[B")
      KEY(RIGHT    , "\x1B[C")
      KEY(LEFT     , "\x1B[D")

      KEY(HOME     , "\x1B[1~")
      KEY(IC       , "\x1B[2~")
      KEY(DC       , "\x1B[3~")
      KEY(END      , "\x1B[4~")
      KEY(PPAGE    , "\x1B[5~")
      KEY(NPAGE    , "\x1B[6~")

      KEY(F( 0)    , "\x1B[10~")
      KEY(F( 1)    , "\x1BOP")
      KEY(F( 2)    , "\x1BOQ")
      KEY(F( 3)    , "\x1BOR")
      KEY(F( 4)    , "\x1BOS")
      KEY(F( 5)    , "\x1B[15~")
      KEY(F( 6)    , "\x1B[17~")
      KEY(F( 7)    , "\x1B[18~")
      KEY(F( 8)    , "\x1B[19~")
      KEY(F( 9)    , "\x1B[20~")
      KEY(F(10)    , "\x1B[21~")
      KEY(F(11)    , "\x1B[23~")
      KEY(F(12)    , "\x1B[24~")
      KEY(F(13)    , "\x1B[25~")
      KEY(F(14)    , "\x1B[26~")
      KEY(F(15)    , "\x1B[28~")
      KEY(F(16)    , "\x1B[29~")
      KEY(F(17)    , "\x1B[31~")
      KEY(F(18)    , "\x1B[32~")
      KEY(F(19)    , "\x1B[33~")
      KEY(F(20)    , "\x1B[34~")
    }
    #undef KEY

    if (keypadTransmitMode) {
      if (sequence) {
        switch (character) {
          case KEY_UP: case KEY_DOWN: case KEY_LEFT: case KEY_RIGHT:
            strcpy(buffer, sequence);
            buffer[1] = 'O';
            sequence = buffer;
            break;
        }
      }
    }
  }

  if (sequence) {
    write(fd, sequence, strlen(sequence));
  } else {
    beep();
  }
}

static int
getCursorRow () {
  return getcury(stdscr);
}

static int
getCursorColumn () {
  return getcurx(stdscr);
}

static int
isWithinScrollRegion (int row) {
  if (row < scrollRegionTop) return 0;
  if (row > scrollRegionBottom) return 0;
  return 1;
}

static void
moveCursorUp (int amount) {
  int oldRow = getCursorRow();
  int newRow = oldRow - amount;

  if (isWithinScrollRegion(oldRow)) {
    int delta = newRow - scrollRegionTop;

    if (delta < 0) {
      scrl(delta);
      newRow = scrollRegionTop;
    }
  }

  if (newRow != oldRow) move(newRow, getCursorColumn());
}

static void
moveCursorDown (int amount) {
  int oldRow = getCursorRow();
  int newRow = oldRow + amount;

  if (isWithinScrollRegion(oldRow)) {
    int delta = newRow - scrollRegionBottom;

    if (delta > 0) {
      scrl(delta);
      newRow = scrollRegionBottom;
    }
  }

  if (newRow != oldRow) move(newRow, getCursorColumn());
}

static void
moveCursorLeft (int amount) {
  move(getCursorRow(), getCursorColumn()-amount);
}

static void
moveCursorRight (int amount) {
  move(getCursorRow(), getCursorColumn()+amount);
}

typedef enum {
  OPS_BASIC,
  OPS_ESCAPE,
  OPS_BRACKET,
  OPS_NUMBER,
  OPS_DIGIT,
  OPS_ACTION,
} OutputParserState;

static OutputParserState outputParserState;
static unsigned char outputParserQuestionMark;

void
ptyResetOutputParser (void) {
  outputParserState = OPS_BASIC;
}

static int outputParserNumber;
static int outputParserNumberArray[9];
static unsigned char outputParserNumberCount;

static void
addOutputParserNumber (int number) {
  if (outputParserNumberCount < ARRAY_COUNT(outputParserNumberArray)) {
    outputParserNumberArray[outputParserNumberCount++] = number;
  }
}

static unsigned char outputByteBuffer[0X40];
static unsigned char outputByteCount;

static void
logOutputAction (const char *name) {
  if (logOutputActions) {
    char prefix[0X100];
    STR_BEGIN(prefix, sizeof(prefix));
    STR_PRINTF("%s", name);

    for (int i=0; i<outputParserNumberCount; i+=1) {
      STR_PRINTF(" %d", outputParserNumberArray[i]);
    }

    STR_END;
    logBytes(screenLogLevel, "%s", outputByteBuffer, outputByteCount, prefix);
  }
}

typedef int OutputByteParser (unsigned char byte);
static int handleUnexpectedOutputByte (unsigned char byte);

static int
parseOutputByte_BASIC (unsigned char byte) {
  outputParserQuestionMark = 0;
  outputParserNumberCount = 0;

  switch (byte) {
    case ASCII_BEL:
      logOutputAction("bel");
      beep();
      return 0;

    case ASCII_BS:
      logOutputAction("cub1");
      moveCursorLeft(1);
      return 0;

    case ASCII_LF:
      logOutputAction("cud1");
      moveCursorDown(1);
      return 0;

    case ASCII_ESC:
      outputParserState = OPS_ESCAPE;
      return 0;

    default: {
      if (logInsertedBytes) {
        logMessage(screenLogLevel, "addch 0X%02X", byte);
      }

      addch(byte);
      return 0;
    }
  }
}

static int
parseOutputByte_ESCAPE (unsigned char byte) {
  switch (byte) {
    case '[':
      outputParserState = OPS_BRACKET;
      return 0;

    case '=':
      logOutputAction("smkx");
      keypadTransmitMode = 1;
      goto basic;

    case '>':
      logOutputAction("rmkx");
      keypadTransmitMode = 0;
      goto basic;

    case 'g':
      logOutputAction("flash");
      flash();
      goto basic;
  }

  return handleUnexpectedOutputByte(byte);

basic:
  outputParserState = OPS_BASIC;
  return 0;
}

static int
parseOutputByte_BRACKET (unsigned char byte) {
  outputParserState = OPS_NUMBER;
  if (outputParserQuestionMark) return 1;
  if (byte != '?') return 1;

  outputParserQuestionMark = 1;
  outputParserState = OPS_BRACKET;
  return 0;
}

static int
parseOutputByte_NUMBER (unsigned char byte) {
  if (iswdigit(byte)) {
    outputParserNumber = 0;
    outputParserState = OPS_DIGIT;
  } else {
    outputParserState = OPS_ACTION;
  }

  return 1;
}

static int
parseOutputByte_DIGIT (unsigned char byte) {
  if (iswdigit(byte)) {
    outputParserNumber *= 10;
    outputParserNumber += byte - '0';
    return 0;
  }

  addOutputParserNumber(outputParserNumber);
  outputParserNumber = 0;
  if (byte == ';') return 0;

  outputParserState = OPS_ACTION;
  return 1;
}

static int
getOutputActionCount (void) {
  if (outputParserNumberCount == 0) return 1;
  return outputParserNumberArray[0];
}

static int
performBracketAction_h (unsigned char byte) {
  if (outputParserNumberCount == 1) {
    switch (outputParserNumberArray[0]) {
      case 34:
        logOutputAction("cnorm");
        curs_set(1);
        return 1;
    }
  }

  return 0;
}

static int
performBracketAction_l (unsigned char byte) {
  if (outputParserNumberCount == 1) {
    switch (outputParserNumberArray[0]) {
      case 34:
        logOutputAction("cvvis");
        curs_set(2);
        return 1;
    }
  }

  return 0;
}

static int
performBracketAction_m (unsigned char byte) {
  if (outputParserNumberCount == 0) {
    logOutputAction("sgr0");
    attrset(0);
    return 1;
  }

  if (outputParserNumberCount == 1) {
    int number = outputParserNumberArray[0];

    switch (number) {
      case 1:
        logOutputAction("bold");
        attron(A_BOLD);
        return 1;

      case 5:
        logOutputAction("blink");
        attron(A_BLINK);
        return 1;

      case 7:
        logOutputAction("rev");
        attron(A_REVERSE);
        return 1;

      case 3:
        logOutputAction("smso");
        attron(A_STANDOUT);
        return 1;

      case 23:
        logOutputAction("rmso");
        attroff(A_STANDOUT);
        return 1;

      case 4:
        logOutputAction("smul");
        attron(A_UNDERLINE);
        return 1;

      case 24:
        logOutputAction("rmul");
        attroff(A_UNDERLINE);
        return 1;

      default: {
        int color = (number % 10) - '0';

        if (color <= 0X7) {
          switch (number / 10) {
            case 3:
              logOutputAction("setaf");
              foregroundColor = color;
              // FIXME: set foreground color
              return 1;

            case 4:
              logOutputAction("setab");
              backgroundColor = color;
              // FIXME: set background color
              return 1;
          }
        }

        break;
      }
    }
  }

  return 0;
}

static int
performBracketAction (unsigned char byte) {
  switch (byte) {
    case 'A':
      logOutputAction("cuu");
      moveCursorUp(getOutputActionCount());
      return 1;

    case 'B':
      logOutputAction("cud");
      moveCursorDown(getOutputActionCount());
      return 1;

    case 'C':
      logOutputAction("cuf");
      moveCursorRight(getOutputActionCount());
      return 1;

    case 'D':
      logOutputAction("cub");
      moveCursorLeft(getOutputActionCount());
      return 1;

    case 'H': {
      if (outputParserNumberCount == 0) {
        addOutputParserNumber(1);
        addOutputParserNumber(1);
      } else if (outputParserNumberCount != 2) {
        break;
      }

      int *row = &outputParserNumberArray[0];
      int *column = &outputParserNumberArray[1];

      if ((*row -= 1) < 0) break;
      if ((*column -= 1) < 0) break;

      logOutputAction("cup");
      move(*row, *column);
      return 1;
    }

    case 'J':
      logOutputAction("ed");
      clrtobot();
      return 1;

    case 'K':
      logOutputAction("el");
      clrtoeol();
      return 1;

    case 'L': {
      logOutputAction("il");
      int count = getOutputActionCount();
      while (count--) insertln();
      return 1;
    }

    case 'M': {
      logOutputAction("dl");
      int count = getOutputActionCount();
      while (count--) deleteln();
      return 1;
    }

    case 'P': {
      logOutputAction("dch");
      int count = getOutputActionCount();
      while (count--) delch();
      return 1;
    }

    case '@': {
      logOutputAction("ic");
      int count = getOutputActionCount();
      while (count--) insch(' ');
      return 1;
    }

    case 'h':
      return performBracketAction_h(byte);

    case 'l':
      return performBracketAction_l(byte);

    case 'm':
      return performBracketAction_m(byte);

    case 'r': {
      if (outputParserNumberCount != 2) break;

      int *top = &outputParserNumberArray[0];
      int *bottom = &outputParserNumberArray[1];

      if ((*top -= 1) < 0) break;
      if ((*bottom -= 1) < 0) break;

      scrollRegionTop = *top;
      scrollRegionBottom = *bottom;

      logOutputAction("csr");
      setscrreg(*top, *bottom);
      return 1;
    }
  }

  return 0;
}

static int
performQuestionMarkAction_h (unsigned char byte) {
  if (outputParserNumberCount == 1) {
    switch (outputParserNumberArray[0]) {
      case 1:
        logOutputAction("smkx");
        keypadTransmitMode = 1;
        return 1;

      case 25:
        logOutputAction("cnorm");
        curs_set(1);
        return 1;

      case 2004:
        logOutputAction("smbp");
        bracketedPasteMode = 1;
        return 1;
    }
  }

  return 0;
}

static int
performQuestionMarkAction_l (unsigned char byte) {
  if (outputParserNumberCount == 1) {
    switch (outputParserNumberArray[0]) {
      case 1:
        logOutputAction("rmkx");
        keypadTransmitMode = 0;
        return 1;

      case 25:
        logOutputAction("civis");
        curs_set(0);
        return 1;

      case 2004:
        logOutputAction("rmbp");
        bracketedPasteMode = 0;
        return 1;
    }
  }

  return 0;
}

static int
performQuestionMarkAction (unsigned char byte) {
  switch (byte) {
    case 'h':
      return performQuestionMarkAction_h(byte);

    case 'l':
      return performQuestionMarkAction_l(byte);
  }

  return 0;
}

static int
parseOutputByte_ACTION (unsigned char byte) {
  int performed;

  if (outputParserQuestionMark) {
    performed = performQuestionMarkAction(byte);
  } else {
    performed = performBracketAction(byte);
  }

  if (!performed) return handleUnexpectedOutputByte(byte);
  outputParserState = OPS_BASIC;
  return 0;
}

typedef struct {
  OutputByteParser *parseOutputByte;
  const char *name;
} OutputParserStateEntry;

#define OPS(state) \
[OPS_##state] = { \
  .parseOutputByte = parseOutputByte_##state, \
  .name = #state, \
}

static const OutputParserStateEntry outputParserStateTable[] = {
  OPS(BASIC),
  OPS(ESCAPE),
  OPS(BRACKET),
  OPS(NUMBER),
  OPS(DIGIT),
  OPS(ACTION),
};
#undef OPS

static int
handleUnexpectedOutputByte (unsigned char byte) {
  if (logUnexpectedOutput) {
    logBytes(screenLogLevel,
      "unexpected pty output byte: %s[0X%02X]",
      outputByteBuffer, outputByteCount,
      outputParserStateTable[outputParserState].name, byte
    );
  }

  beep();
  outputParserState = OPS_BASIC;

  return 0;
}

int
ptyParseOutputByte (unsigned char byte) {
  if (outputParserState == OPS_BASIC) outputByteCount = 0;

  if (outputByteCount < ARRAY_COUNT(outputByteBuffer)) {
    outputByteBuffer[outputByteCount++] = byte;
  }

  while (outputParserStateTable[outputParserState].parseOutputByte(byte));
  return outputParserState == OPS_BASIC;
}

int
ptyParseOutputBytes (const unsigned char *bytes, size_t count) {
  int wantRefresh = 0;

  const unsigned char *byte = bytes;
  const unsigned char *end = byte + count;

  while (byte < end) {
    wantRefresh = ptyParseOutputByte(*byte++);
  }

  return wantRefresh;
}

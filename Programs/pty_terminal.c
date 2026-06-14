/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2026 by The BRLTTY Developers.
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
 * enacs=\E(B\E)0 - enable alternate charset mode
 * hts=\EH - set tab
 * kmous=\E[M - mouse event
 * tbc=\E[3g - clear all tabs
 */

#include "prologue.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "log.h"
#include "strfmt.h"
#include "pty_terminal.h"
#include "pty_screen.h"
#include "pty_clipboard.h"
#include "scr_types.h"
#include "color.h"
#include "get_term.h"
#include "ascii.h"
#include "utf8.h"
#include "async_handle.h"
#include "async_alarm.h"

static unsigned char terminalLogLevel = LOG_DEBUG;
static unsigned char logInput = 0;
static unsigned char logOutput = 0;
static unsigned char logSequences = 0;
static unsigned char logUnexpected = 0;

void
ptySetTerminalLogLevel (unsigned char level) {
  terminalLogLevel = level;
  ptySetScreenLogLevel(level);
}

void
ptySetLogTerminalInput (int yes) {
  logInput = yes;
}

void
ptySetLogTerminalOutput (int yes) {
  logOutput = yes;
}

void
ptySetLogTerminalSequences (int yes) {
  logSequences = yes;
}

void
ptySetLogUnexpectedTerminalIO (int yes) {
  logUnexpected = yes;
}

static int
testTerminalType (const char *type) {
  int status;
  if (setupterm(type, STDOUT_FILENO, &status) == OK) return 1;

  switch (status) {
    case 1:
      logMessage(LOG_WARNING, "hardcopy terminal: %s", type);
      break;

    case 0:
      logMessage(LOG_WARNING, "unrecognized terminal type: %s", type);
      break;

    case -1:
      logMessage(LOG_WARNING, "terminfo database not installed");
      break;

    default:
      logMessage(LOG_WARNING, "unexpected setupterm error status: %d", status);
      break;
  }

  return 0;
}

const char *
ptyGetTerminalType (void) {
  static const char *terminalType = NULL;

  if (!terminalType) {
    static const char *const terminalTypes[] = {
      "tmux-256color",
      "screen-256color",
      "tmux",
      "screen",
      NULL
    };

    for (const char *const *type=terminalTypes; *type; type+=1) {
      if (testTerminalType(*type)) {
        terminalType = *type;
        break;
      }
    }

    if (terminalType) {
      logMessage(LOG_INFO, "selected terminal type: %s", terminalType);
    } else {
      logMessage(LOG_ERR, "can't use any eligible terminal type");
    }
  }

  return terminalType;
}

static unsigned char insertMode = 0;
static unsigned char keypadTransmitMode = 0;   /* DECKPAM: application keypad   */
static unsigned char cursorKeyMode = 0;        /* DECCKM: application cursor keys */
static unsigned char bracketedPasteMode = 0;
static unsigned char absoluteCursorAddressingMode = 0;

/* Character-set state for VT100 line drawing. An app designates G0/G1 as the
 * DEC special graphics set ("ESC ( 0" / "ESC ) 0") and switches between them
 * with SO/SI; while a graphics set is active, letters render as box-drawing
 * glyphs. We translate those to the Unicode equivalents so braille shows real
 * lines rather than "qqqq". */
static unsigned char charsetShiftedOut = 0;    /* SO active -> G1 selected */
static unsigned char g0GraphicsCharset = 0;    /* G0 designated as graphics */
static unsigned char g1GraphicsCharset = 0;    /* G1 designated as graphics */
static unsigned char charsetDesignator = 0;    /* pending ESC ( ) * + introducer */

/* The most recently printed glyph, for the "repeat" control (CSI b / REP). */
static wchar_t lastTextCharacter = ' ';

/* OSC (operating system command) capture, for OSC 52 clipboard support. Only
 * OSC strings (introduced by "ESC ]") are buffered; other string controls
 * (DCS/PM/APC) are consumed without buffering. */
static unsigned char outputStringIsOsc = 0;
static char oscBuffer[0X10000];
static unsigned int oscLength = 0;
static unsigned char oscOverflow = 0;

static Utf8StreamDecoder textDecoder;

static void ptyResetTerminalInput (PtyObject *pty);

int
ptyBeginTerminal (PtyObject *pty, int driverDirectives) {
  insertMode = 0;
  keypadTransmitMode = 0;
  cursorKeyMode = 0;
  bracketedPasteMode = 0;
  absoluteCursorAddressingMode = 0;

  charsetShiftedOut = 0;
  g0GraphicsCharset = 0;
  g1GraphicsCharset = 0;
  charsetDesignator = 0;
  lastTextCharacter = ' ';

  outputStringIsOsc = 0;
  oscLength = 0;
  oscOverflow = 0;

  memset(&textDecoder, 0, sizeof(textDecoder));
  ptyResetTerminalInput(pty);

  return ptyBeginScreen(pty, driverDirectives);
}

void
ptyEndTerminal (void) {
  ptyResetTerminalInput(NULL);
  ptyEndScreen();
}

void
ptyResizeTerminal (unsigned int height, unsigned int width) {
  ptyResizeScreen(height, width);
}

static void
soundAlert (void) {
  beep();
}

static void
showAlert (void) {
  flash();
}

/* Keyboard input parser.
 *
 * Rather than letting curses decode keys (which depends on the *host* terminal's
 * terminfo, silently drops modified keys it has no capability for, and strands
 * bytes in its own buffer behind a select() loop), we read the host's raw bytes
 * and parse the escape sequences ourselves - the approach tmux takes in
 * tty-keys.c. Recognized cursor/navigation/function keys are re-encoded for the
 * child by ptyWriteInputCharacter() (honoring the child's application-cursor
 * mode and any modifiers); everything else - printable text, UTF-8, control
 * bytes, Alt/meta combinations, and any sequence we don't recognize - is passed
 * through to the child verbatim so nothing is ever lost.
 */

/* How long to wait for the rest of an escape sequence before deciding a lone
 * ESC really was the Escape key (and flushing any half-finished sequence). */
#define INPUT_ESCAPE_TIMEOUT 100

typedef enum {
  IPS_GROUND,  /* between sequences */
  IPS_ESCAPE,  /* seen ESC */
  IPS_CSI,     /* seen ESC [ , collecting parameters */
  IPS_SS3,     /* seen ESC O , expecting one final byte */
} InputParserState;

static PtyObject *inputPty = NULL;
static InputParserState inputState = IPS_GROUND;
static unsigned char inputBuffer[0X20];
static unsigned int inputCount = 0;
static AsyncHandle inputTimeoutAlarm = NULL;

static int
forwardInputBytes (const unsigned char *bytes, size_t count) {
  return ptyWriteInputData(inputPty, bytes, count);
}

static void
resetInputParser (void) {
  inputState = IPS_GROUND;
  inputCount = 0;
}

static int
appendInputByte (unsigned char byte) {
  if (inputCount < ARRAY_COUNT(inputBuffer)) {
    inputBuffer[inputCount++] = byte;
    return 1;
  }

  return 0;
}

/* Translate the modifier flags into ScreenKey bits and emit the key. The
 * xtermModifier is the standard "1 + bitmask" parameter (shift=1, alt=2,
 * ctrl=4); values below 2 mean no modifier. */
static void
emitMappedKey (ScreenKey base, unsigned int xtermModifier) {
  ScreenKey key = base;

  if (xtermModifier >= 2) {
    unsigned int bits = xtermModifier - 1;
    if (bits & 1) key |= SCR_KEY_SHIFT;
    if (bits & 2) key |= SCR_KEY_ALT_LEFT;
    if (bits & 4) key |= SCR_KEY_CONTROL;
  }

  ptyWriteInputCharacter(inputPty, key, cursorKeyMode);
}

static ScreenKey
finalLetterToKey (unsigned char final) {
  switch (final) {
    case 'A': return SCR_KEY_CURSOR_UP;
    case 'B': return SCR_KEY_CURSOR_DOWN;
    case 'C': return SCR_KEY_CURSOR_RIGHT;
    case 'D': return SCR_KEY_CURSOR_LEFT;
    case 'H': return SCR_KEY_HOME;
    case 'F': return SCR_KEY_END;
    case 'P': return SCR_KEY_F1;
    case 'Q': return SCR_KEY_F2;
    case 'R': return SCR_KEY_F3;
    case 'S': return SCR_KEY_F4;
    default:  return 0;
  }
}

static ScreenKey
tildeNumberToKey (unsigned int number) {
  switch (number) {
    case  1: case 7: return SCR_KEY_HOME;
    case  2: return SCR_KEY_INSERT;
    case  3: return SCR_KEY_DELETE;
    case  4: case 8: return SCR_KEY_END;
    case  5: return SCR_KEY_PAGE_UP;
    case  6: return SCR_KEY_PAGE_DOWN;
    case 11: return SCR_KEY_F1;
    case 12: return SCR_KEY_F2;
    case 13: return SCR_KEY_F3;
    case 14: return SCR_KEY_F4;
    case 15: return SCR_KEY_F5;
    case 17: return SCR_KEY_F6;
    case 18: return SCR_KEY_F7;
    case 19: return SCR_KEY_F8;
    case 20: return SCR_KEY_F9;
    case 21: return SCR_KEY_F10;
    case 23: return SCR_KEY_F11;
    case 24: return SCR_KEY_F12;
    case 25: return SCR_KEY_F13;
    case 26: return SCR_KEY_F14;
    case 28: return SCR_KEY_F15;
    case 29: return SCR_KEY_F16;
    case 31: return SCR_KEY_F17;
    case 32: return SCR_KEY_F18;
    case 33: return SCR_KEY_F19;
    case 34: return SCR_KEY_F20;
    default: return 0;
  }
}

/* Interpret a complete "ESC [ ... final" sequence in inputBuffer. */
static void
handleControlSequence (void) {
  unsigned char final = inputBuffer[inputCount - 1];

  unsigned int parameters[2] = {0, 0};
  unsigned int parameterCount = 0;
  int haveDigit = 0;
  int unsupported = 0;

  for (unsigned int i=2; i<(inputCount-1); i+=1) {
    unsigned char byte = inputBuffer[i];

    if ((byte >= '0') && (byte <= '9')) {
      if (parameterCount < ARRAY_COUNT(parameters)) {
        parameters[parameterCount] = (parameters[parameterCount] * 10) + (byte - '0');
      }
      haveDigit = 1;
    } else if (byte == ';') {
      if (parameterCount < ARRAY_COUNT(parameters)) parameterCount += 1;
      haveDigit = 0;
    } else {
      /* private markers ('?', '>', '<') or intermediates - not a key */
      unsupported = 1;
      break;
    }
  }

  if (haveDigit && (parameterCount < ARRAY_COUNT(parameters))) parameterCount += 1;

  if (!unsupported) {
    ScreenKey base = 0;
    unsigned int modifier = (parameterCount >= 2)? parameters[1]: 0;

    if (final == '~') {
      base = tildeNumberToKey((parameterCount >= 1)? parameters[0]: 0);
    } else {
      base = finalLetterToKey(final);
    }

    if (base) {
      emitMappedKey(base, modifier);
      return;
    }
  }

  /* Unrecognized: forward verbatim so the child still gets it. */
  forwardInputBytes(inputBuffer, inputCount);
}

static void
handleSingleShift3 (unsigned char final) {
  ScreenKey base = finalLetterToKey(final);

  if (base) {
    emitMappedKey(base, 0);
  } else {
    forwardInputBytes(inputBuffer, inputCount);
  }
}

static void
processInputByte (unsigned char byte) {
  switch (inputState) {
    case IPS_GROUND:
      if (byte == ASCII_ESC) {
        inputCount = 0;
        appendInputByte(byte);
        inputState = IPS_ESCAPE;
      } else {
        forwardInputBytes(&byte, 1);
      }
      break;

    case IPS_ESCAPE:
      if (byte == '[') {
        appendInputByte(byte);
        inputState = IPS_CSI;
      } else if (byte == 'O') {
        appendInputByte(byte);
        inputState = IPS_SS3;
      } else if (byte == ASCII_ESC) {
        /* ESC ESC: emit one Escape and start a fresh sequence. */
        unsigned char esc = ASCII_ESC;
        forwardInputBytes(&esc, 1);
        inputCount = 0;
        appendInputByte(byte);
      } else {
        /* Alt/Meta + byte: the child's encoding is exactly ESC then the byte. */
        unsigned char sequence[2] = {ASCII_ESC, byte};
        forwardInputBytes(sequence, sizeof(sequence));
        resetInputParser();
      }
      break;

    case IPS_CSI:
      if ((byte >= 0X40) && (byte <= 0X7E)) {       /* final byte */
        appendInputByte(byte);
        handleControlSequence();
        resetInputParser();
      } else if ((byte >= 0X20) && (byte <= 0X3F)) { /* parameter / intermediate */
        if (!appendInputByte(byte)) {                /* overflow - give up cleanly */
          forwardInputBytes(inputBuffer, inputCount);
          resetInputParser();
        }
      } else {
        /* Stray byte inside a CSI: flush what we have and reprocess it. */
        forwardInputBytes(inputBuffer, inputCount);
        resetInputParser();
        processInputByte(byte);
      }
      break;

    case IPS_SS3:
      if ((byte >= 0X40) && (byte <= 0X7E)) {
        appendInputByte(byte);
        handleSingleShift3(byte);
        resetInputParser();
      } else {
        forwardInputBytes(inputBuffer, inputCount);
        resetInputParser();
        processInputByte(byte);
      }
      break;
  }
}

ASYNC_ALARM_CALLBACK(handleInputEscapeTimeout) {
  asyncDiscardHandle(inputTimeoutAlarm);
  inputTimeoutAlarm = NULL;

  /* The rest of a sequence never arrived: a lone ESC was the Escape key, and a
   * half-finished CSI/SS3 is passed through as-is. */
  if (inputState != IPS_GROUND) {
    forwardInputBytes(inputBuffer, inputCount);
    resetInputParser();
  }
}

static void
cancelInputEscapeTimeout (void) {
  if (inputTimeoutAlarm) {
    asyncCancelRequest(inputTimeoutAlarm);
    inputTimeoutAlarm = NULL;
  }
}

static void
armInputEscapeTimeout (void) {
  if (inputTimeoutAlarm) {
    asyncResetAlarmIn(inputTimeoutAlarm, INPUT_ESCAPE_TIMEOUT);
  } else {
    asyncNewRelativeAlarm(&inputTimeoutAlarm, INPUT_ESCAPE_TIMEOUT,
                          handleInputEscapeTimeout, NULL);
  }
}

static void
ptyResetTerminalInput (PtyObject *pty) {
  cancelInputEscapeTimeout();
  resetInputParser();
  inputPty = pty;
}

int
ptyProcessTerminalInput (PtyObject *pty) {
  inputPty = pty;

  unsigned char buffer[0X200];
  ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer));

  if (count > 0) {
    if (logInput) logBytes(terminalLogLevel, "input", buffer, count);
    for (ssize_t i=0; i<count; i+=1) processInputByte(buffer[i]);

    /* Wait briefly for the tail of an unfinished sequence; otherwise resolve
     * immediately so a stalled ESC isn't held back. */
    if (inputState == IPS_GROUND) {
      cancelInputEscapeTimeout();
    } else {
      armInputEscapeTimeout();
    }

    return 1;
  }

  if (count == 0) return 0;                          /* end of input */
  if ((errno == EINTR) || (errno == EAGAIN)) return 1;
  return 0;
}

static unsigned char outputByteBuffer[0X40];
static unsigned char outputByteCount;

static void
logUnexpectedSequence (void) {
  if (logUnexpected) {
    logBytes(
      terminalLogLevel, "unexpected sequence",
      outputByteBuffer, outputByteCount
    );
  }
}

typedef enum {
  OPS_BASIC,
  OPS_ESCAPE,
  OPS_BRACKET,
  OPS_NUMBER,
  OPS_DIGIT,
  OPS_ACTION,
  OPS_STRING,
  OPS_STRING_ESCAPE,
  OPS_DISCARD,
  OPS_TEXT,
} OutputParserState;

static OutputParserState outputParserState;
static unsigned char outputParserQuestionMark;
static unsigned char outputParserGreaterThan;

static unsigned int outputParserNumber;
static unsigned int outputParserNumberArray[9];
static unsigned char outputParserNumberCount;

static void
addOutputParserNumber (unsigned int number) {
  if (outputParserNumberCount < ARRAY_COUNT(outputParserNumberArray)) {
    outputParserNumberArray[outputParserNumberCount++] = number;
  }
}

static unsigned int
getOutputActionCount (void) {
  if (outputParserNumberCount == 0) return 1;
  return outputParserNumberArray[0];
}

static void
logOutputAction (const char *name, const char *description) {
  if (logSequences) {
    char prefix[0X100];
    STR_BEGIN(prefix, sizeof(prefix));
    STR_PRINTF("action: %s", name);

    for (unsigned int i=0; i<outputParserNumberCount; i+=1) {
      STR_PRINTF(" %u", outputParserNumberArray[i]);
    }

    if (description && *description) STR_PRINTF(" (%s)", description);
    STR_END;
    logBytes(terminalLogLevel, "%s", outputByteBuffer, outputByteCount, prefix);
  }
}

typedef enum {
  OBP_DONE,
  OBP_CONTINUE,
  OBP_REPROCESS,
  OBP_UNEXPECTED,
} OutputByteParserResult;

typedef OutputByteParserResult OutputByteParser (unsigned char byte);

/* Map a byte from the DEC special graphics set to its Unicode equivalent so a
 * braille reader sees real box-drawing/line glyphs instead of plain letters. */
static wchar_t
mapGraphicsCharacter (wchar_t character) {
  static const wchar_t table[] = {
    /* 0x60 */ 0X25C6, 0X2592, 0X2409, 0X240C, 0X240D, 0X240A, 0X00B0, 0X00B1,
    /* 0x68 */ 0X2424, 0X240B, 0X2518, 0X2510, 0X250C, 0X2514, 0X253C, 0X23BA,
    /* 0x70 */ 0X23BB, 0X2500, 0X23BC, 0X23BD, 0X251C, 0X2524, 0X2534, 0X252C,
    /* 0x78 */ 0X2502, 0X2264, 0X2265, 0X03C0, 0X2260, 0X00A3, 0X00B7,
  };

  if ((character >= 0X60) && (character <= 0X7E)) {
    wchar_t mapped = table[character - 0X60];
    if (mapped) return mapped;
  }

  return character;
}

static int
graphicsCharsetActive (void) {
  return charsetShiftedOut? g1GraphicsCharset: g0GraphicsCharset;
}

static void
addTextCharacter (wchar_t character) {
  if (graphicsCharsetActive()) character = mapGraphicsCharacter(character);

  if (logOutput) {
    logMessage(terminalLogLevel, "output: U+%04X", (unsigned int)character);
  }

  if (insertMode) ptyInsertCharacters(1);
  ptyAddCharacter(character);
  lastTextCharacter = character;
}

/* Reply to a device query by writing the response to the child as if typed. */
static void
sendDeviceResponse (const char *response) {
  if (inputPty) ptyWriteInputData(inputPty, response, strlen(response));
}

static OutputByteParserResult
parseOutputByte_BASIC (unsigned char byte) {
  outputParserQuestionMark = 0;
  outputParserGreaterThan = 0;
  outputParserNumberCount = 0;

  switch (byte) {
    case ASCII_ESC:
      outputParserState = OPS_ESCAPE;
      return OBP_CONTINUE;

    case ASCII_BEL:
      logOutputAction("bel", "audible alert");
      soundAlert();
      return OBP_DONE;

    case ASCII_BS:
      logOutputAction("cub1", "cursor left 1");
      ptyMoveCursorLeft(1);
      return OBP_DONE;

    case ASCII_HT:
      logOutputAction("ht", "tab forward");
      ptyTabForward();
      return OBP_DONE;

    case ASCII_LF:
      if (ptyAmWithinScrollRegion()) {
        logOutputAction("ind", "move down 1");
        ptyMoveDown1();
      } else {
        logOutputAction("cud1", "cursor down 1");
        ptyMoveCursorDown(1);
      }
      return OBP_DONE;

    case ASCII_CR:
      logOutputAction("cr", "carriage return");
      ptySetCursorColumn(0);
      return OBP_DONE;

    case ASCII_SO:
      logOutputAction("smacs", "shift out to G1");
      charsetShiftedOut = 1;
      return OBP_DONE;

    case ASCII_SI:
      logOutputAction("rmacs", "shift in to G0");
      charsetShiftedOut = 0;
      return OBP_DONE;

    default: {
      wchar_t character;
      int reprocess;
      Utf8DecodeResult result = putUtf8StreamByte(&textDecoder, byte, &character, &reprocess);

      switch (result) {
        case UTF8_DECODE_PENDING:
          outputParserState = OPS_TEXT;
          return OBP_CONTINUE;

        case UTF8_DECODE_DONE:
        case UTF8_DECODE_INVALID:
          /* A lone/invalid lead byte never asks to be reprocessed here. */
          addTextCharacter(character);
          return OBP_DONE;
      }

      return OBP_DONE;
    }
  }
}

static OutputByteParserResult
parseOutputByte_TEXT (unsigned char byte) {
  wchar_t character;
  int reprocess;
  Utf8DecodeResult result = putUtf8StreamByte(&textDecoder, byte, &character, &reprocess);

  switch (result) {
    case UTF8_DECODE_PENDING:
      return OBP_CONTINUE;

    case UTF8_DECODE_DONE:
      addTextCharacter(character);
      outputParserState = OPS_BASIC;
      return OBP_DONE;

    case UTF8_DECODE_INVALID:
      /* The multi-byte sequence was truncated; emit the replacement
       * character for it and, when the offending byte may start a new
       * character, reprocess it from the base state.
       */
      addTextCharacter(character);
      outputParserState = OPS_BASIC;
      return reprocess? OBP_REPROCESS: OBP_DONE;
  }

  return OBP_DONE;
}

static OutputByteParserResult
parseOutputByte_ESCAPE (unsigned char byte) {
  switch (byte) {
    case '[':
      outputParserState = OPS_BRACKET;
      return OBP_CONTINUE;

    case '=':
      logOutputAction("smkx", "keypad transmit on");
      keypadTransmitMode = 1;
      return OBP_DONE;

    case '>':
      logOutputAction("rmkx", "keypad transmit off");
      keypadTransmitMode = 0;
      return OBP_DONE;

    case 'E':
      logOutputAction("nel", "new line");
      ptySetCursorColumn(0);
      ptyMoveDown1();
      return OBP_DONE;

    case 'M':
      if (ptyAmWithinScrollRegion()) {
        logOutputAction("ri", "move up 1");
        ptyMoveUp1();
      } else {
        logOutputAction("cuu1", "cursor up 1");
        ptyMoveCursorUp(1);
      }
      return OBP_DONE;

    case 'c':
      logOutputAction("clear", "clear screen");
      ptySetCursorPosition(0, 0);
      ptyClearToEndOfDisplay();
      return OBP_DONE;

    case 'g':
      logOutputAction("flash", "visual alert");
      showAlert();
      return OBP_DONE;

    case '7':
      logOutputAction("sc", "save cursor position");
      ptySaveCursorPosition();
      return OBP_DONE;

    case '8':
      logOutputAction("rc", "restore cursor position");
      ptyRestoreCursorPosition();
      return OBP_DONE;

    case ']': /* OSC - operating system command */
      /* OSC runs until a string terminator (BEL or ESC \). We capture it so
       * OSC 52 can drive the system clipboard; other OSCs (titles, hyperlinks)
       * are captured but ignored. */
      outputStringIsOsc = 1;
      oscLength = 0;
      oscOverflow = 0;
      outputParserState = OPS_STRING;
      return OBP_CONTINUE;

    case 'P': /* DCS - device control string */
    case '^': /* PM  - privacy message */
    case '_': /* APC - application program command */
      /* We don't act on these, but must consume the whole string so its
       * payload doesn't leak onto the screen as text. */
      outputStringIsOsc = 0;
      outputParserState = OPS_STRING;
      return OBP_CONTINUE;

    case '\\': /* ST - lone string terminator */
      return OBP_DONE;

    case '(': /* designate G0 character set (e.g. enacs "\E(B") */
    case ')': /* designate G1 character set (e.g. is2 "\E)0")   */
    case '*': /* designate G2 character set */
    case '+': /* designate G3 character set */
    case '#': /* DEC line-size / alignment (e.g. DECALN "\E#8") */
      /* These introducers are followed by exactly one selector byte. We record
       * a G0/G1 graphics designation (for line drawing) and consume the
       * selector so it isn't printed as text. */
      charsetDesignator = byte;
      outputParserState = OPS_DISCARD;
      return OBP_CONTINUE;
  }

  return OBP_UNEXPECTED;
}

static OutputByteParserResult
parseOutputByte_DISCARD (unsigned char byte) {
  /* The byte is the selector following ESC ( ) * + #. "0" selects the DEC
   * special graphics (line-drawing) set; anything else (e.g. "B" = ASCII) is
   * a non-graphics set. We only track G0 and G1. */
  switch (charsetDesignator) {
    case '(': g0GraphicsCharset = (byte == '0'); break;
    case ')': g1GraphicsCharset = (byte == '0'); break;
    default: break;
  }

  return OBP_DONE;
}

static void
appendOscByte (unsigned char byte) {
  if (oscOverflow) return;

  if (oscLength >= (sizeof(oscBuffer) - 1)) {
    oscOverflow = 1;
    return;
  }

  oscBuffer[oscLength++] = byte;
}

static int
base64Value (unsigned char byte) {
  if ((byte >= 'A') && (byte <= 'Z')) return byte - 'A';
  if ((byte >= 'a') && (byte <= 'z')) return byte - 'a' + 26;
  if ((byte >= '0') && (byte <= '9')) return byte - '0' + 52;
  if (byte == '+') return 62;
  if (byte == '/') return 63;
  return -1;
}

/* Decode base64 text into a freshly allocated buffer (caller frees). */
static unsigned char *
decodeBase64 (const char *text, size_t *length) {
  size_t textLength = strlen(text);
  unsigned char *data = malloc(((textLength / 4) + 1) * 3 + 1);
  if (!data) return NULL;

  size_t outLength = 0;
  int quad[4];
  unsigned int quadCount = 0;

  for (size_t i=0; i<textLength; i+=1) {
    if (text[i] == '=') break;

    int value = base64Value(text[i]);
    if (value < 0) continue; /* skip whitespace and stray bytes */

    quad[quadCount++] = value;
    if (quadCount == 4) {
      data[outLength++] = (quad[0] << 2) | (quad[1] >> 4);
      data[outLength++] = (quad[1] << 4) | (quad[2] >> 2);
      data[outLength++] = (quad[2] << 6) | quad[3];
      quadCount = 0;
    }
  }

  if (quadCount >= 2) {
    data[outLength++] = (quad[0] << 2) | (quad[1] >> 4);
    if (quadCount >= 3) data[outLength++] = (quad[1] << 4) | (quad[2] >> 2);
  }

  *length = outLength;
  return data;
}

static void
handleClipboardRequest (char *payload) {
  /* payload is "<targets>;<data>" (the part after "52;"). */
  char *data = strchr(payload, ';');
  if (!data) return;
  data += 1;

  /* "?" is a read request; decline it so a program can't read the user's
   * clipboard (a well-known OSC 52 privacy concern). */
  if (strcmp(data, "?") == 0) return;

  size_t length;
  unsigned char *bytes = decodeBase64(data, &length);

  if (bytes) {
    if (length > 0) ptySetSystemClipboard((const char *)bytes, length);
    free(bytes);
  }
}

static void
handleOscString (void) {
  if (oscOverflow) return;
  oscBuffer[oscLength] = 0;

  if (strncmp(oscBuffer, "52;", 3) == 0) {
    logOutputAction("osc52", "clipboard");
    handleClipboardRequest(oscBuffer + 3);
  }
}

static void
finishStringControl (void) {
  if (outputStringIsOsc) {
    handleOscString();
    outputStringIsOsc = 0;
  }
}

static OutputByteParserResult
parseOutputByte_STRING (unsigned char byte) {
  switch (byte) {
    case ASCII_BEL: /* OSC-style string terminator */
      finishStringControl();
      return OBP_DONE;

    case ASCII_CAN: /* cancel - abort the string without acting on it */
    case ASCII_SUB:
      outputStringIsOsc = 0;
      return OBP_DONE;

    case ASCII_ESC: /* possible start of ST (ESC \) */
      outputParserState = OPS_STRING_ESCAPE;
      return OBP_CONTINUE;

    default: /* consume the string payload */
      if (outputStringIsOsc) appendOscByte(byte);
      return OBP_CONTINUE;
  }
}

static OutputByteParserResult
parseOutputByte_STRING_ESCAPE (unsigned char byte) {
  if (byte == '\\') { /* ST - end of string */
    finishStringControl();
    return OBP_DONE;
  }

  /* Not a string terminator after all - stay within the string.
   * Reprocess this byte so another ESC can still introduce an ST.
   */
  outputParserState = OPS_STRING;
  return OBP_REPROCESS;
}

static OutputByteParserResult
parseOutputByte_BRACKET (unsigned char byte) {
  outputParserState = OPS_NUMBER;
  if (outputParserQuestionMark) return OBP_REPROCESS;

  switch (byte) {
    case '?':
      outputParserQuestionMark = 1;
      outputParserState = OPS_BRACKET;
      return OBP_CONTINUE;

    case '>': /* secondary device attributes (and kitty keyboard) marker */
      outputParserGreaterThan = 1;
      return OBP_CONTINUE;

    case '=': /* tertiary DA / other private parameter markers. We don't */
    case '<': /* implement these, but consume the marker so the rest of  */
      return OBP_CONTINUE; /* the sequence is parsed and not leaked.      */
  }

  return OBP_REPROCESS;
}

static OutputByteParserResult
parseOutputByte_NUMBER (unsigned char byte) {
  if (iswdigit(byte)) {
    outputParserNumber = 0;
    outputParserState = OPS_DIGIT;
  } else {
    outputParserState = OPS_ACTION;
  }

  return OBP_REPROCESS;
}

static OutputByteParserResult
parseOutputByte_DIGIT (unsigned char byte) {
  if (iswdigit(byte)) {
    outputParserNumber *= 10;
    outputParserNumber += byte - '0';
    return OBP_CONTINUE;
  }

  addOutputParserNumber(outputParserNumber);
  outputParserNumber = 0;
  if (byte == ';') return OBP_CONTINUE;

  outputParserState = OPS_ACTION;
  return OBP_REPROCESS;
}

static OutputByteParserResult
performBracketAction_h (unsigned char byte) {
  if (outputParserNumberCount == 1) {
    switch (outputParserNumberArray[0]) {
      case 4:
        logOutputAction("smir", "insert on");
        insertMode = 1;
        return OBP_DONE;

      case 34:
        logOutputAction("cnorm", "cursor normal visibility");
        ptySetCursorVisibility(1);
        return OBP_DONE;
    }
  }

  return OBP_UNEXPECTED;
}

static OutputByteParserResult
performBracketAction_l (unsigned char byte) {
  if (outputParserNumberCount == 1) {
    switch (outputParserNumberArray[0]) {
      case 4:
        logOutputAction("rmir", "insert off");
        insertMode = 0;
        return OBP_DONE;

      case 34:
        logOutputAction("cvvis", "cursor very visile");
        ptySetCursorVisibility(2);
        return OBP_DONE;
    }
  }

  return OBP_UNEXPECTED;
}

static OutputByteParserResult
performBracketAction_m (unsigned char byte) {
  if (outputParserNumberCount == 0) addOutputParserNumber(0);

  for (unsigned int index=0; index<outputParserNumberCount; index+=1) {
    unsigned int number = outputParserNumberArray[index];
    unsigned int count = outputParserNumberCount - index;

    switch (number / 10) {
      {
        int bright;

        case 3:
          bright = 0;
          goto DO_FOREGROUND;

        case 4:
          bright = 0;
          goto DO_BACKGROUND;

        case 9:
          bright = 1;
          goto DO_FOREGROUND;

        case 10:
          bright = 1;
          goto DO_BACKGROUND;

        const char *name;
        const char *description;
        void (*setColor) (int color);
        int color;

      DO_FOREGROUND:
        name = "setaf";
        description = "foreground color";
        setColor = ptySetForegroundColor;
        goto DO_COLOR;

      DO_BACKGROUND:
        name = "setab";
        description = "background color";
        setColor = ptySetBackgroundColor;
        goto DO_COLOR;

      DO_COLOR:
        color = number % 10;

        if (color == 8) {
          if (!bright) {
            if (count >= 2) {
              switch (outputParserNumberArray[++index]) {
                case 2: {
                  if (count >= 5) {
                    unsigned int red = outputParserNumberArray[++index];
                    unsigned int green = outputParserNumberArray[++index];
                    unsigned int blue = outputParserNumberArray[++index];

                    color = rgbToVga(red, green, blue, 0);
                    goto SET_COLOR;
                  }

                  break;
                }

                case 5: {
                  if (count >= 3) {
                    color = outputParserNumberArray[++index];
                    if (color <= 0XF) goto SET_COLOR;

                    /* Map the rest of the 256-color palette (16-255) to the
                     * nearest VGA color rather than rejecting it, which would
                     * abort the whole SGR run and drop trailing attributes. */
                    if (color <= 0XFF) {
                      color = rgbColorToVga(ansiToRgb(color), 0);
                      goto SET_COLOR;
                    }
                  }

                  break;
                }
              }
            }
          }

          return OBP_UNEXPECTED;
        }

        if (color == 9) {
          color = -1;
        } else if (bright) {
          color |= 0X8;
        }

      SET_COLOR:
        logOutputAction(name, description);
        setColor(color);
        continue;
      }
    }

    switch (number) {
      case 0:
        logOutputAction("sgr0", "all attributes off");
        ptySetAttributes(0);
        continue;

      case 1:
        logOutputAction("bold", "bold on");
        ptyAddAttributes(A_BOLD);
        continue;

      case 2:
        logOutputAction("dim", "dim on");
        ptyAddAttributes(A_DIM);
        continue;

      case 3:
        logOutputAction("sitm", "italic on");
        ptyAddAttributes(A_ITALIC);
        continue;

      case 4:
        logOutputAction("smul", "underline on");
        ptyAddAttributes(A_UNDERLINE);
        continue;

      case 5:
        logOutputAction("smbl", "slow blink on");
        ptyAddAttributes(A_BLINK);
        continue;

      case 6:
        logOutputAction("smfbl", "fast blink on");
        ptyAddAttributes(A_BLINK);
        continue;

      case 7:
        logOutputAction("rev", "reverse video on");
        ptyAddAttributes(A_REVERSE);
        continue;

      case 8:
        logOutputAction("smxon", "conceal on");
        continue;

      case 9:
        logOutputAction("smstrike", "strikethrough on");
        continue;

      case 22:
        logOutputAction("normal", "bold/dim off");
        ptyRemoveAttributes(A_BOLD | A_DIM);
        continue;

      case 23:
        logOutputAction("ritm", "italic off");
        ptyRemoveAttributes(A_ITALIC);
        continue;

      case 24:
        logOutputAction("rmul", "underline off");
        ptyRemoveAttributes(A_UNDERLINE);
        continue;

      case 25:
        logOutputAction("rmbl", "slow/fast blink off");
        ptyRemoveAttributes(A_BLINK);
        continue;

      case 27:
        logOutputAction("unrev", "reverse video off");
        ptyRemoveAttributes(A_REVERSE);
        continue;

      case 28:
        logOutputAction("rmxon", "conceal off");
        continue;

      case 29:
        logOutputAction("rmstrike", "strikethrough off");
        continue;
    }

    return OBP_UNEXPECTED;
  }

  return OBP_DONE;
}

static OutputByteParserResult
performBracketAction (unsigned char byte) {
  switch (byte) {
    case 'A':
      logOutputAction("cuu", "cursor up");
      ptyMoveCursorUp(getOutputActionCount());
      return OBP_DONE;

    case 'B':
      logOutputAction("cud", "cursor down");
      ptyMoveCursorDown(getOutputActionCount());
      return OBP_DONE;

    case 'C':
      logOutputAction("cuf", "cursor right");
      ptyMoveCursorRight(getOutputActionCount());
      return OBP_DONE;

    case 'D':
      logOutputAction("cub", "cursor left");
      ptyMoveCursorLeft(getOutputActionCount());
      return OBP_DONE;

    case 'G': {
      if (outputParserNumberCount != 1) return OBP_UNEXPECTED   ;
      unsigned int *column = &outputParserNumberArray[0];
      if (!(*column)--) return OBP_UNEXPECTED;

      logOutputAction("hpa", "set cursor column");
      ptySetCursorColumn(*column);
      return OBP_DONE;
    }

    {
      const char *action;
      int allowZero;

    case 'f':
      action = "hvp";
      allowZero = 1;
      goto MOVE_CURSOR;

    case 'H':
      action = "cup";
      allowZero = 0;
      goto MOVE_CURSOR;

    MOVE_CURSOR:
      if (outputParserNumberCount == 0) {
        addOutputParserNumber(1);
        addOutputParserNumber(1);
      } else if (outputParserNumberCount != 2) {
        return OBP_UNEXPECTED;
      }

      unsigned int *row = &outputParserNumberArray[0];
      unsigned int *column = &outputParserNumberArray[1];

      if (allowZero) {
        if (*row) *row -= 1;
        if (*column) *column -= 1;
      } else {
        if (!(*row)--) return OBP_UNEXPECTED;
        if (!(*column)--) return OBP_UNEXPECTED;
      }

      logOutputAction(action, "set cursor position");
      ptySetCursorPosition(*row, *column);
      return OBP_DONE;
    }

    case 'J': {
      if (outputParserNumberCount == 0) addOutputParserNumber(0);
      if (outputParserNumberCount != 1) return OBP_UNEXPECTED;

      switch (outputParserNumberArray[0]) {
        case 0:
          logOutputAction("ed", "clear to end of display");
          ptyClearToEndOfDisplay();
          return OBP_DONE;

        case 1:
          logOutputAction("ed1", "clear to beginning of display");
          ptyClearToBeginningOfDisplay();
          return OBP_DONE;

        case 3: /* clear scrollback - we keep none, so clear the screen */
        case 2:
          logOutputAction("ed2", "clear display");
          ptyClearDisplay();
          return OBP_DONE;
      }

      break;
    }

    case 'K': {
      if (outputParserNumberCount == 0) addOutputParserNumber(0);
      if (outputParserNumberCount != 1) return OBP_UNEXPECTED;

      switch (outputParserNumberArray[0]) {
        case 0:
          logOutputAction("el", "clear to end of line");
          ptyClearToEndOfLine();
          return OBP_DONE;

        case 1:
          logOutputAction("el1", "clear to beginning of line");
          ptyClearToBeginningOfLine();
          return OBP_DONE;

        case 2:
          logOutputAction("el2", "clear line");
          ptyClearLine();
          return OBP_DONE;
      }

      break;
    }

    case 'X':
      logOutputAction("ech", "erase characters");
      ptyEraseCharacters(getOutputActionCount());
      return OBP_DONE;

    case 'b':
      logOutputAction("rep", "repeat last character");
      for (unsigned int count=getOutputActionCount(); count>0; count-=1) {
        addTextCharacter(lastTextCharacter);
      }
      return OBP_DONE;

    case 'c': {
      /* Device attributes: reply so a probing program doesn't stall. */
      unsigned int parameter = outputParserNumberCount? outputParserNumberArray[0]: 0;
      if (parameter != 0) return OBP_UNEXPECTED;

      if (outputParserGreaterThan) {
        logOutputAction("da2", "secondary device attributes");
        sendDeviceResponse("\x1B[>0;0;0c");
      } else {
        logOutputAction("da", "primary device attributes");
        sendDeviceResponse("\x1B[?1;2c");
      }

      return OBP_DONE;
    }

    case 'n': {
      /* Device status report. */
      if (outputParserNumberCount != 1) return OBP_UNEXPECTED;

      switch (outputParserNumberArray[0]) {
        case 5: /* "are you ok?" -> "I am ok" */
          logOutputAction("dsr", "device status report");
          sendDeviceResponse("\x1B[0n");
          return OBP_DONE;

        case 6: { /* report cursor position (CPR) */
          unsigned int row, column;
          ptyGetCursorPosition(&row, &column);

          char response[0X20];
          STR_BEGIN(response, sizeof(response));
          STR_PRINTF("\x1B[%u;%uR", (row + 1), (column + 1));
          STR_END;

          logOutputAction("cpr", "cursor position report");
          sendDeviceResponse(response);
          return OBP_DONE;
        }
      }

      break;
    }

    case 'L':
      logOutputAction("il", "insert lines");
      ptyInsertLines(getOutputActionCount());
      return OBP_DONE;

    case 'M':
      logOutputAction("dl", "delete lines");
      ptyDeleteLines(getOutputActionCount());
      return OBP_DONE;

    case 'P':
      logOutputAction("dch", "delete characters");
      ptyDeleteCharacters(getOutputActionCount());
      return OBP_DONE;

    case 'S':
      logOutputAction("indn", "scroll forward");
      ptyScrollUp(getOutputActionCount());
      return OBP_DONE;

    case 'T':
      logOutputAction("rin", "scroll backward");
      ptyScrollDown(getOutputActionCount());
      return OBP_DONE;

    case 'Z':
      logOutputAction("cbt", "tab backward");
      ptyTabBackward();
      return OBP_DONE;

    case 'd': {
      if (outputParserNumberCount != 1) return OBP_UNEXPECTED   ;
      unsigned int *row = &outputParserNumberArray[0];
      if (!(*row)--) return OBP_UNEXPECTED;

      logOutputAction("vpa", "set cursor row");
      ptySetCursorRow(*row);
      return OBP_DONE;
    }

    case 'h':
      return performBracketAction_h(byte);

    case 'l':
      return performBracketAction_l(byte);

    case 'm':
      /* "CSI > ... m" is XTMODKEYS (modifyOtherKeys) keyboard-protocol
       * negotiation, not SGR. Swallow it rather than mistaking the parameters
       * for graphic renditions (e.g. turning on underline/dim). */
      if (outputParserGreaterThan) return OBP_DONE;
      return performBracketAction_m(byte);

    case 'u':
      /* "CSI ... u" is the kitty keyboard-protocol negotiation (push/pop/set
       * the keyboard mode). We don't drive the host into those modes, so just
       * consume it - it must never reach the screen as text. */
      return OBP_DONE;

    case 'r': {
      if (outputParserNumberCount != 2) return OBP_UNEXPECTED;

      unsigned int *top = &outputParserNumberArray[0];
      unsigned int *bottom = &outputParserNumberArray[1];

      if (!(*top)--) return OBP_UNEXPECTED;
      if (!(*bottom)--) return OBP_UNEXPECTED;

      logOutputAction("csr", "set scroll region");
      ptySetScrollRegion(*top, *bottom);
      return OBP_DONE;
    }

    case '@':
      logOutputAction("ic", "insert characters");
      ptyInsertCharacters(getOutputActionCount());
      return OBP_DONE;
  }

  return OBP_UNEXPECTED;
}

static OutputByteParserResult
performQuestionMarkAction_h (unsigned char byte) {
  if (outputParserNumberCount == 1) {
    switch (outputParserNumberArray[0]) {
      case 1:
        logOutputAction("DECCKM", "application cursor keys on");
        cursorKeyMode = 1;
        return OBP_DONE;

      case 25:
        logOutputAction("cnorm", "cursor normal visibility");
        ptySetCursorVisibility(1);
        return OBP_DONE;

      case 1049:
        logOutputAction("smcup", "absolute cursor addressing on");
        absoluteCursorAddressingMode = 1;
        /* Entering the alternate screen presents a cleared buffer (this is
         * what xterm's ?1049 does). We have only one buffer, so emulate it by
         * saving the cursor and clearing the screen; otherwise a full-screen
         * program (e.g. Claude Code) draws on top of the previous contents. */
        ptySaveCursorPosition();
        ptySetCursorPosition(0, 0);
        ptyClearToEndOfDisplay();
        return OBP_DONE;

      case 2004:
        logOutputAction("smbp", "bracketed paste on");
        bracketedPasteMode = 1;
        return OBP_DONE;
    }
  }

  return OBP_UNEXPECTED;
}

static OutputByteParserResult
performQuestionMarkAction_l (unsigned char byte) {
  if (outputParserNumberCount == 1) {
    switch (outputParserNumberArray[0]) {
      case 1:
        logOutputAction("DECCKM", "application cursor keys off");
        cursorKeyMode = 0;
        return OBP_DONE;

      case 25:
        logOutputAction("civis", "cursor invisible");
        ptySetCursorVisibility(0);
        return OBP_DONE;

      case 1049:
        logOutputAction("rmcup", "absolute cursor addressing off");
        absoluteCursorAddressingMode = 0;
        /* Leaving the alternate screen would normally restore the primary
         * screen. We can't restore it (single buffer), but we must not leave
         * the full-screen program's last frame behind as stale text; clear it
         * and put the cursor back where it was before smcup. */
        ptySetCursorPosition(0, 0);
        ptyClearToEndOfDisplay();
        ptyRestoreCursorPosition();
        return OBP_DONE;

      case 2004:
        logOutputAction("rmbp", "bracketed paste off");
        bracketedPasteMode = 0;
        return OBP_DONE;
    }
  }

  return OBP_UNEXPECTED;
}

static OutputByteParserResult
performQuestionMarkAction (unsigned char byte) {
  switch (byte) {
    case 'h':
      return performQuestionMarkAction_h(byte);

    case 'l':
      return performQuestionMarkAction_l(byte);
  }

  return OBP_UNEXPECTED;
}

static OutputByteParserResult
parseOutputByte_ACTION (unsigned char byte) {
  if ((byte >= 0X20) && (byte <= 0X2F)) {
    /* CSI intermediate byte (e.g. the space in "\e[5 q" to set the
     * cursor shape). Consume it and wait for the final byte so the
     * whole sequence is swallowed as a unit rather than leaking.
     */
    return OBP_CONTINUE;
  }

  if (outputParserQuestionMark) {
    return performQuestionMarkAction(byte);
  } else {
    return performBracketAction(byte);
  }
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
  OPS(STRING),
  OPS(STRING_ESCAPE),
  OPS(DISCARD),
  OPS(TEXT),
};
#undef OPS

static int
parseOutputByte (unsigned char byte) {
  if (outputParserState == OPS_BASIC) {
    outputByteCount = 0;
  }

  if (outputByteCount < ARRAY_COUNT(outputByteBuffer)) {
    outputByteBuffer[outputByteCount++] = byte;
  }

  while (1) {
    OutputByteParserResult result = outputParserStateTable[outputParserState].parseOutputByte(byte);

    switch (result) {
      case OBP_REPROCESS:
        continue;

      case OBP_UNEXPECTED:
        logUnexpectedSequence();
        /* fall through */

      case OBP_DONE:
        outputParserState = OPS_BASIC;
        return 1;

      case OBP_CONTINUE:
        return 0;
    }
  }
}

int
ptyProcessTerminalOutput (const unsigned char *bytes, size_t count) {
  int wantRefresh = 0;

  const unsigned char *byte = bytes;
  const unsigned char *end = byte + count;

  while (byte < end) {
    wantRefresh = parseOutputByte(*byte++);
  }

  if (wantRefresh) {
    ptyRefreshScreen();
  }

  return 1;
}

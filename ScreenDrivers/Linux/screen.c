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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/tty.h>
#include <linux/vt.h>
#include <linux/kd.h>

#include "misc.h"
#include "system.h"
#include "sys_linux.h"
#include "charset.h"
#include "brldefs.h"

typedef enum {
  PARM_CHARSET,
  PARM_HFB,
  PARM_DEBUGSFM,
} ScreenParameters;
#define SCRPARMS "charset", "hfb", "debugsfm"

#include "scr_driver.h"
#include "screen.h"

static const char *problemText;
static unsigned int debugScreenFontMap = 0;

#define UNICODE_ROW_DIRECT 0XF000

typedef enum {
  CONV_OK,
  CONV_ILLEGAL,
  CONV_SHORT,
  CONV_OVERFLOW,
  CONV_ERROR
} CharacterConversionResult;

#if defined(HAVE_ICONV_H)
#include <iconv.h>

typedef struct {
  iconv_t iconvHandle;
} CharsetConverter;

#define ICONV_NULL ((iconv_t)-1)
#define CHARSET_CONVERTER_INITIALIZER {.iconvHandle = ICONV_NULL}

static int
allocateCharsetConverter (CharsetConverter *converter, const char *sourceCharset, const char *targetCharset) {
  if (converter->iconvHandle == ICONV_NULL) {
    if ((converter->iconvHandle = iconv_open(targetCharset, sourceCharset)) == ICONV_NULL) {
      LogError("iconv_open");
      return 0;
    }
  }
  return 1;
}

static void
deallocateCharsetConverter (CharsetConverter *converter) {
  if (converter->iconvHandle != ICONV_NULL) {
    iconv_close(converter->iconvHandle);
    converter->iconvHandle = ICONV_NULL;
  }
}

static CharacterConversionResult
convertCharacters (
  CharsetConverter *converter,
  const char **inputAddress, size_t *inputLength,
  char **outputAddress, size_t *outputLength
) {
  ssize_t result = iconv(converter->iconvHandle, (char **)inputAddress, inputLength, outputAddress, outputLength);
  if (result != -1) return CONV_OK;
  if (errno == EILSEQ) return CONV_ILLEGAL;
  if (errno == EINVAL) return CONV_SHORT;
  if (errno == E2BIG) return CONV_OVERFLOW;
  LogError("iconv");
  return CONV_ERROR;
}

typedef struct {
  char *name;
  unsigned isMultiByte:1;
  CharsetConverter charsetToWchar;
  CharsetConverter wcharToCharset;
} CharsetEntry;

static CharsetEntry *charsetEntries = NULL;
static unsigned int charsetCount = 0;
static unsigned int charsetIndex = 0;

static inline CharsetEntry *
getCharsetEntry (void) {
  return &charsetEntries[charsetIndex];
}

static void
deallocateCharsetEntries (void) {
  if (charsetEntries) {
    while (charsetCount) {
      CharsetEntry *charset = &charsetEntries[--charsetCount];
      free(charset->name);
      deallocateCharsetConverter(&charset->charsetToWchar);
      deallocateCharsetConverter(&charset->wcharToCharset);
    }

    free(charsetEntries);
    charsetEntries = NULL;
  }
}

static int
allocateCharsetEntries (const char *names) {
  int ok = 0;
  int count;
  char **namesArray = splitString(names, '+', &count);

  if (namesArray) {
    CharsetEntry *entries = calloc(count, sizeof(*entries));

    if (entries) {
      charsetEntries = entries;
      charsetCount = 0;
      charsetIndex = 0;
      ok = 1;

      while (charsetCount < count) {
        CharsetEntry *charset = &charsetEntries[charsetCount];

        if (!(charset->name = strdup(namesArray[charsetCount]))) {
          ok = 0;
          deallocateCharsetEntries();
          break;
        }

        charset->isMultiByte = 0;

        {
          static const CharsetConverter nullCharsetConverter = CHARSET_CONVERTER_INITIALIZER;
          charset->charsetToWchar = nullCharsetConverter;
          charset->wcharToCharset = nullCharsetConverter;
        }

        charsetCount += 1;
      }
    }

    deallocateStrings(namesArray);
  }

  return ok;
}

static CharacterConversionResult
convertCharsToWchar (const char *chars, size_t length, wchar_t *character, size_t *size) {
  unsigned int count = charsetCount;

  while (count--) {
    CharsetEntry *charset = getCharsetEntry();
    CharsetConverter *converter = &charset->charsetToWchar;
    CharacterConversionResult result = CONV_ERROR;

    if (allocateCharsetConverter(converter, charset->name, getWcharCharset())) {
      const char *inptr = chars;
      size_t inlen = length;
      char *outptr = (char *)character;
      size_t outlen = sizeof(*character);

      if ((result = convertCharacters(converter, &inptr, &inlen, &outptr, &outlen)) == CONV_OK)
        if (size)
          *size = inptr - chars;
    }

    if (result == CONV_SHORT) charset->isMultiByte = 1;
    if (result != CONV_ILLEGAL) return result;
    if (++charsetIndex == charsetCount) charsetIndex = 0;
  }

  return CONV_ILLEGAL;
}

static CharacterConversionResult
convertWcharToChars (wchar_t character, char *chars, size_t length, size_t *size) {
  CharsetEntry *charset = getCharsetEntry();
  CharsetConverter *converter = &charset->wcharToCharset;
  CharacterConversionResult result = CONV_ERROR;

  if (allocateCharsetConverter(converter, getWcharCharset(), charset->name)) {
    const char *inptr = (char *)&character;
    size_t inlen = sizeof(character);
    char *outptr = chars;
    size_t outlen = length;

    if ((result = convertCharacters(converter, &inptr, &inlen, &outptr, &outlen)) == CONV_OK) {
      size_t count = outptr - chars;
      if (size) *size = count;
      if (count > 1) charset->isMultiByte = 1;
    } else if ((result == CONV_OVERFLOW) && length) {
      charset->isMultiByte = 1;
    }
  }

  return result;
}

static wint_t
convertCharacter (const wchar_t *character) {
  static unsigned char spaces = 0;
  static unsigned char length = 0;
  static char buffer[MB_LEN_MAX];
  const wchar_t cellMask = 0XFF;

  if (!character) {
    length = 0;
    if (!spaces) return WEOF;
    spaces -= 1;
    return WC_C(' ');
  }

  if ((*character & ~cellMask) != UNICODE_ROW_DIRECT) {
    length = 0;
    return *character;
  }

  if (length < sizeof(buffer)) {
    buffer[length++] = *character & cellMask;

    while (1) {
      wchar_t wc;
      CharacterConversionResult result = convertCharsToWchar(buffer, length, &wc, NULL);

      if (result == CONV_OK) {
        length = 0;
        return wc;
      }

      if (result == CONV_SHORT) break;
      if (result != CONV_ILLEGAL) break;

      if (!--length) break;
      memcpy(buffer, buffer+1, length);
    }
  }

  spaces += 1;
  return WEOF;
}
#else /* charset conversion definitions */
static int
allocateCharsetEntries (const char *names) {
  return 1;
}

static CharacterConversionResult
convertWcharToChars (wchar_t character, char *chars, size_t length, size_t *size) {
  if (!length) return CONV_OVERFLOW;

  {
    int c = convertWcharToChar(character);
    chars[0] = c;
    *size = 1;
    return CONV_OK;
  }
}

static wint_t
convertCharacter (const wchar_t *character) {
  if (!character) return WEOF;
  return character[0];
}
#endif /* charset conversion definitions */

static int
setDeviceName (const char **name, const char *const *names, const char *description, int mode) {
  return (*name = resolveDeviceName(names, description, mode)) != NULL;
}

static char *
vtName (const char *name, unsigned char vt) {
  if (vt) {
    size_t length = strlen(name);
    char buffer[length+4];
    if (name[length-1] == '0') length -= 1;
    strncpy(buffer, name, length);
    sprintf(buffer+length,  "%u", vt);
    return strdupWrapper(buffer);
  }
  return strdupWrapper(name);
}

static const char *consoleName = NULL;
static int consoleDescriptor;

static int
setConsoleName (void) {
  static const char *const names[] = {"tty0", "vc/0", NULL};
  return setDeviceName(&consoleName, names, "console", R_OK|W_OK);
}

static void
closeConsole (void) {
  if (consoleDescriptor != -1) {
    if (close(consoleDescriptor) == -1) {
      LogError("console close");
    }
    LogPrint(LOG_DEBUG, "console closed: fd=%d", consoleDescriptor);
    consoleDescriptor = -1;
  }
}

static int
openConsole (unsigned char vt) {
  int opened = 0;
  char *name = vtName(consoleName, vt);
  if (name) {
    int console = openCharacterDevice(name, O_RDWR|O_NOCTTY, 4, vt);
    if (console != -1) {
      LogPrint(LOG_DEBUG, "console opened: %s: fd=%d", name, console);
      closeConsole();
      consoleDescriptor = console;
      opened = 1;
    }
    free(name);
  }
  return opened;
}

static const char *screenName = NULL;
static int screenDescriptor;
static unsigned char virtualTerminal;

static int
setScreenName (void) {
  static const char *const names[] = {"vcsa", "vcsa0", "vcc/a", NULL};
  return setDeviceName(&screenName, names, "screen", R_OK|W_OK);
}

static void
closeScreen (void) {
  if (screenDescriptor != -1) {
    if (close(screenDescriptor) == -1) {
      LogError("screen close");
    }
    LogPrint(LOG_DEBUG, "screen closed: fd=%d", screenDescriptor);
    screenDescriptor = -1;
  }
}

static int
openScreen (unsigned char vt) {
  int opened = 0;
  char *name = vtName(screenName, vt);
  if (name) {
    int screen = openCharacterDevice(name, O_RDWR, 7, 0X80|vt);
    if (screen != -1) {
      LogPrint(LOG_DEBUG, "screen opened: %s: fd=%d", name, screen);
      if (openConsole(vt)) {
        closeScreen();
        screenDescriptor = screen;
        virtualTerminal = vt;
        opened = 1;
      } else {
        close(screen);
        LogPrint(LOG_DEBUG, "screen closed: fd=%d", screen);
      }
    }
    free(name);
  }
  return opened;
}

static int
rebindConsole (void) {
  return virtualTerminal? 1: openConsole(0);
}

static int
controlConsole (int operation, void *argument) {
  int result = ioctl(consoleDescriptor, operation, argument);
  if (result == -1)
    if (errno == EIO)
      if (openConsole(virtualTerminal))
        result = ioctl(consoleDescriptor, operation, argument);
  return result;
}

static struct unipair *screenFontMapTable = NULL;
static unsigned short screenFontMapSize = 0;
static unsigned short screenFontMapCount;
static int
setScreenFontMap (int force) {
  struct unimapdesc sfm;
  unsigned short size = force? 0: screenFontMapCount;
  if (!size) size = 0X100;
  while (1) {
    sfm.entry_ct = size;
    if (!(sfm.entries = malloc(sfm.entry_ct * sizeof(*sfm.entries)))) {
      LogError("screen font map allocation");
      return 0;
    }
    if (controlConsole(GIO_UNIMAP, &sfm) != -1) break;
    free(sfm.entries);
    if (errno != ENOMEM) {
      LogError("ioctl GIO_UNIMAP");
      return 0;
    }
    if (!(size <<= 1)) {
      LogPrint(LOG_ERR, "screen font map too big.");
      return 0;
    }
  }
  if (!force) {
    if (sfm.entry_ct == screenFontMapCount) {
      if (memcmp(sfm.entries, screenFontMapTable, sfm.entry_ct*sizeof(sfm.entries[0])) == 0) {
        if (size == screenFontMapSize) {
          free(sfm.entries);
        } else {
          free(screenFontMapTable);
          screenFontMapTable = sfm.entries;
          screenFontMapSize = size;
        }
        return 0;
      }
    }
    free(screenFontMapTable);
  }
  screenFontMapTable = sfm.entries;
  screenFontMapCount = sfm.entry_ct;
  screenFontMapSize = size;
  LogPrint(LOG_INFO, "Screen Font Map Size: %d", screenFontMapCount);
  if (debugScreenFontMap) {
    int i;
    for (i=0; i<screenFontMapCount; ++i) {
      const struct unipair *map = &screenFontMapTable[i];
      LogPrint(LOG_DEBUG, "sfm[%03u]: unum=%4.4X fpos=%4.4X",
               i, map->unicode, map->fontpos);
    }
  }
  return 1;
}

static int vgaCharacterCount;
static int vgaLargeTable;

static int
setVgaCharacterCount (int force) {
  int oldCount = vgaCharacterCount;

  {
    struct console_font_op cfo;

    memset(&cfo, 0, sizeof(cfo));
    cfo.op = KD_FONT_OP_GET;
    cfo.height = 32;
    cfo.width = 16;

    if (controlConsole(KDFONTOP, &cfo) != -1) {
      vgaCharacterCount = cfo.charcount;
    } else {
      vgaCharacterCount = 0;

      if (errno != EINVAL) {
        LogPrint(LOG_WARNING, "ioctl KDFONTOP[GET]: %s", strerror(errno));
      }
    }
  }

  if (!vgaCharacterCount) {
    int index;
    for (index=0; index<screenFontMapCount; ++index) {
      const struct unipair *map = &screenFontMapTable[index];
      if (vgaCharacterCount <= map->fontpos) vgaCharacterCount = map->fontpos + 1;
    }
  }

  vgaCharacterCount = ((vgaCharacterCount - 1) | 0XFF) + 1;
  vgaLargeTable = vgaCharacterCount > 0X100;

  if (!force)
    if (vgaCharacterCount == oldCount)
      return 0;

  LogPrint(LOG_INFO, "VGA Character Count: %d(%s)",
           vgaCharacterCount,
           vgaLargeTable? "large": "small");

  return 1;
}

static unsigned short highFontBit;
static unsigned short fontAttributesMask;
static unsigned short unshiftedAttributesMask;
static unsigned short shiftedAttributesMask;

static void
setAttributesMasks (unsigned short bit) {
  fontAttributesMask = bit;
  unshiftedAttributesMask = (((bit & 0XF000) - 0X1000) & 0XF000) |
                            (((bit & 0X0F00) - 0X0100) & 0X0F00);
  shiftedAttributesMask = ((~((bit & 0XF000) - 0X1000) << 1) & 0XE000) |
                          ((~((bit & 0X0F00) - 0X0100) << 1) & 0X0E00);
  LogPrint(LOG_DEBUG, "attributes masks: font=%04X unshifted=%04X shifted=%04X",
           fontAttributesMask, unshiftedAttributesMask, shiftedAttributesMask);
}

#ifndef VT_GETHIFONTMASK
#define VT_GETHIFONTMASK 0X560D
#endif /* VT_GETHIFONTMASK */

static int
determineAttributesMasks (void) {
  if (!vgaLargeTable) {
    setAttributesMasks(0);
  } else if (highFontBit) {
    setAttributesMasks(highFontBit);
  } else {
    {
      unsigned short mask;
      if (controlConsole(VT_GETHIFONTMASK, &mask) == -1) {
        if (errno != EINVAL) LogError("ioctl[VT_GETHIFONTMASK]");
      } else if (mask & 0XFF) {
        LogPrint(LOG_ERR, "high font mask has bit set in low-order byte: %04X", mask);
      } else {
        setAttributesMasks(mask);
        return 1;
      }
    }

    if (lseek(screenDescriptor, 0, SEEK_SET) != -1) {
      unsigned char attributes[4];

      if (read(screenDescriptor, attributes, sizeof(attributes)) != -1) {
        const size_t count = attributes[0] * attributes[1];
        unsigned short buffer[count];

        if (read(screenDescriptor, buffer, sizeof(buffer)) != -1) {
          int counts[0X10];
          int index;

          memset(counts, 0, sizeof(counts));
          for (index=0; index<count; index+=1) ++counts[(buffer[index] & 0X0F00) >> 8];

          setAttributesMasks((counts[0XE] > counts[0X7])? 0X0100: 0X0800);
          return 1;
        } else {
          LogError("read");
        }
      } else {
        LogError("read");
      }
    } else {
      LogError("lseek");
    }
  }

  return 0;
}

static int
processParameters_LinuxScreen (char **parameters) {
  {
    const char *names = parameters[PARM_CHARSET];
    if (!names || !*names) names = getLocaleCharset();
    if (!allocateCharsetEntries(names)) return 0;
  }

  if (!validateYesNo(&debugScreenFontMap, parameters[PARM_DEBUGSFM]))
    LogPrint(LOG_WARNING, "%s: %s", "invalid screen font map debug setting", parameters[PARM_DEBUGSFM]);

  highFontBit = 0;
  if (parameters[PARM_HFB] && *parameters[PARM_HFB]) {
    int bit = 0;
    static const int minimum = 0;
    static const int maximum = 7;
    static const char *choices[] = {"auto", "vga", "fb", NULL};
    unsigned int choice;
    if (validateInteger(&bit, parameters[PARM_HFB], &minimum, &maximum)) {
      highFontBit = 1 << (bit + 8);
    } else if (!validateChoice(&choice, parameters[PARM_HFB], choices)) {
      LogPrint(LOG_WARNING, "%s: %s", "invalid high font bit", parameters[PARM_HFB]);
    } else if (choice) {
      static const unsigned short bits[] = {0X0800, 0X0100};
      highFontBit = bits[choice-1];
    }
  }

  return 1;
}

static wchar_t translationTable[0X200];
static int
setTranslationTable (int force) {
  int sfmChanged = setScreenFontMap(force);
  int vccChanged = (sfmChanged || force)? setVgaCharacterCount(force): 0;

  if (vccChanged || force) determineAttributesMasks();

  if (sfmChanged || vccChanged) {
    unsigned int count = ARRAY_COUNT(translationTable);

    {
      int i;
      for (i=0; i<count; ++i) {
        translationTable[i] = UNICODE_ROW_DIRECT | i;
      }
    }

    {
      int screenFontMapIndex = screenFontMapCount;
      while (screenFontMapIndex > 0) {
        const struct unipair *sfm = &screenFontMapTable[--screenFontMapIndex];
        if (sfm->fontpos < count) {
          translationTable[sfm->fontpos] = sfm->unicode;
        }
      }
    }

    return 1;
  }

  return 0;
}

#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>

typedef unsigned char At2KeyTable[0X100];

static const At2KeyTable at2KeysOriginal = {
  [0X76] = KEY_ESC,
  [0X05] = KEY_F1,
  [0X06] = KEY_F2,
  [0X04] = KEY_F3,
  [0X0C] = KEY_F4,
  [0X03] = KEY_F5,
  [0X0B] = KEY_F6,
  [0X83] = KEY_F7,
  [0X0A] = KEY_F8,
  [0X01] = KEY_F9,
  [0X09] = KEY_F10,
  [0X78] = KEY_F11,
  [0X07] = KEY_F12,
  [0X7E] = KEY_SCROLLLOCK,

  [0X0E] = KEY_GRAVE,
  [0X16] = KEY_1,
  [0X1E] = KEY_2,
  [0X26] = KEY_3,
  [0X25] = KEY_4,
  [0X2E] = KEY_5,
  [0X36] = KEY_6,
  [0X3D] = KEY_7,
  [0X3E] = KEY_8,
  [0X46] = KEY_9,
  [0X45] = KEY_0,
  [0X4E] = KEY_MINUS,
  [0X55] = KEY_EQUAL,
  [0X66] = KEY_BACKSPACE,

  [0X0D] = KEY_TAB,
  [0X15] = KEY_Q,
  [0X1D] = KEY_W,
  [0X24] = KEY_E,
  [0X2D] = KEY_R,
  [0X2C] = KEY_T,
  [0X35] = KEY_Y,
  [0X3C] = KEY_U,
  [0X43] = KEY_I,
  [0X44] = KEY_O,
  [0X4D] = KEY_P,
  [0X54] = KEY_LEFTBRACE,
  [0X5B] = KEY_RIGHTBRACE,
  [0X5D] = KEY_BACKSLASH,

  [0X58] = KEY_CAPSLOCK,
  [0X1C] = KEY_A,
  [0X1B] = KEY_S,
  [0X23] = KEY_D,
  [0X2B] = KEY_F,
  [0X34] = KEY_G,
  [0X33] = KEY_H,
  [0X3B] = KEY_J,
  [0X42] = KEY_K,
  [0X4B] = KEY_L,
  [0X4C] = KEY_SEMICOLON,
  [0X52] = KEY_APOSTROPHE,
  [0X5A] = KEY_ENTER,

  [0X12] = KEY_LEFTSHIFT,
  [0X61] = KEY_102ND,
  [0X1A] = KEY_Z,
  [0X22] = KEY_X,
  [0X21] = KEY_C,
  [0X2A] = KEY_V,
  [0X32] = KEY_B,
  [0X31] = KEY_N,
  [0X3A] = KEY_M,
  [0X41] = KEY_COMMA,
  [0X49] = KEY_DOT,
  [0X4A] = KEY_SLASH,
  [0X59] = KEY_RIGHTSHIFT,

  [0X14] = KEY_LEFTCTRL,
  [0X11] = KEY_LEFTALT,
  [0X29] = KEY_SPACE,

  [0X77] = KEY_NUMLOCK,
  [0X7C] = KEY_KPASTERISK,
  [0X7B] = KEY_KPMINUS,
  [0X79] = KEY_KPPLUS,
  [0X71] = KEY_KPDOT,
  [0X70] = KEY_KP0,
  [0X69] = KEY_KP1,
  [0X72] = KEY_KP2,
  [0X7A] = KEY_KP3,
  [0X6B] = KEY_KP4,
  [0X73] = KEY_KP5,
  [0X74] = KEY_KP6,
  [0X6C] = KEY_KP7,
  [0X75] = KEY_KP8,
  [0X7D] = KEY_KP9
};

static const At2KeyTable at2KeysE0 = {
  [0X1F] = KEY_LEFTMETA,
  [0X11] = KEY_RIGHTALT,
  [0X27] = KEY_RIGHTMETA,
  [0X2F] = KEY_COMPOSE,
  [0X14] = KEY_RIGHTCTRL,
  [0X70] = KEY_INSERT,
  [0X71] = KEY_DELETE,
  [0X6C] = KEY_HOME,
  [0X69] = KEY_END,
  [0X7D] = KEY_PAGEUP,
  [0X7A] = KEY_PAGEDOWN,
  [0X75] = KEY_UP,
  [0X6B] = KEY_LEFT,
  [0X72] = KEY_DOWN,
  [0X74] = KEY_RIGHT,
  [0X4A] = KEY_KPSLASH,
  [0X5A] = KEY_KPENTER
};

static const At2KeyTable at2KeysE1 = {
};

static const unsigned char *at2Keys;
static int at2Pressed;
#endif /* HAVE_LINUX_INPUT_H */

static int currentConsoleNumber;
static int
construct_LinuxScreen (void) {
  if (setScreenName()) {
    screenDescriptor = -1;

    if (setConsoleName()) {
      consoleDescriptor = -1;

      if (openScreen(currentConsoleNumber=0)) {
        if (setTranslationTable(1)) {
          return 1;
        }
      }
    }
  }
  return 0;
}

static void
destruct_LinuxScreen (void) {
  closeConsole();
  consoleName = NULL;

  closeScreen();
  screenName = NULL;

  if (screenFontMapTable) {
    free(screenFontMapTable);
    screenFontMapTable = NULL;
  }
  screenFontMapSize = 0;
  screenFontMapCount = 0;
}

static int
userVirtualTerminal_LinuxScreen (int number) {
  return MAX_NR_CONSOLES + 1 + number;
}

static int
readScreenDevice (off_t offset, void *buffer, size_t size) {
  if (lseek(screenDescriptor, offset, SEEK_SET) == -1) {
    LogError("screen seek");
  } else {
    int count = read(screenDescriptor, buffer, size);
    if (count == size) return 1;

    if (count == -1) {
      LogError("screen read");
    } else {
      LogPrint(LOG_ERR, "truncated screen data: expected %d bytes, read %d",
               size, count);
    }
  }
  return 0;
}

static int
readScreenDimensions (short *columns, short *rows) {
  typedef struct {
    unsigned char rows;
    unsigned char columns;
  } ScreenDimensions;

  ScreenDimensions dimensions;
  if (!readScreenDevice(0, &dimensions, sizeof(dimensions))) return 0;

  *rows = dimensions.rows;
  *columns = dimensions.columns;
  return 1;
}

static int
readScreenRow (int row, size_t size, ScreenCharacter *characters, int *offsets) {
  unsigned short line[size];
  size_t length = sizeof(line);
  int column = 0;

  if (readScreenDevice((4 + (row * length)), line, length)) {
    const unsigned short *source = line;
    const unsigned short *end = source + size;
    ScreenCharacter *character = characters;

    while (source != end) {
      unsigned short position = *source & 0XFF;
      wint_t wc;

      if (*source & fontAttributesMask) position |= 0X100;
      if ((wc = convertCharacter(&translationTable[position])) != WEOF) {
        if (character) {
          character->text = wc;
          character->attributes = ((*source & unshiftedAttributesMask) |
                                   ((*source & shiftedAttributesMask) >> 1)) >> 8;
          character += 1;
        }

        if (offsets) offsets[column++] = source - line;
      }

      source += 1;
    }

    {
      wint_t wc;
      while ((wc = convertCharacter(NULL)) != WEOF) {
        if (character) {
          character->text = wc;
          character->attributes = 0X07;
          character += 1;
        }

        if (offsets) offsets[column++] = size - 1;
      }
    }

    return 1;
  }

  return 0;
}

static int
readCursorCoordinates (short *column, short *row, short columns) {
  typedef struct {
    unsigned char column;
    unsigned char row;
  } ScreenCoordinates;

  ScreenCoordinates coordinates;

  if (readScreenDevice(2, &coordinates, sizeof(coordinates))) {
#if defined(HAVE_ICONV_H)
    const CharsetEntry *charset = getCharsetEntry();
#endif /* HAVE_ICONV_H */

    *row = coordinates.row;

#if defined(HAVE_ICONV_H)
    if (!charset->isMultiByte) {
      *column = coordinates.column;
      return 1;
    }
#endif /* HAVE_ICONV_H */

    {
      int offsets[columns];

      if (readScreenRow(coordinates.row, columns, NULL, offsets)) {
        int first = 0;
        int last = columns - 1;

        while (first <= last) {
          int current = (first + last) / 2;

          if (offsets[current] < coordinates.column) {
            first = current + 1;
          } else {
            last = current - 1;
          }
        }

        if (first == columns) first -= 1;
        *column = first;
        return 1;
      }
    }
  }

  return 0;
}

static int
getScreenDescription (ScreenDescription *description) {
  if (!problemText) {
    if (readScreenDimensions(&description->cols, &description->rows)) {
      if (readCursorCoordinates(&description->posx, &description->posy, description->cols)) {
        return 1;
      }
    }

    problemText = "screen header read error";
  }

  description->rows = 1;
  description->cols = strlen(problemText);
  description->posx = 0;
  description->posy = 0;
  return 0;
}

static int
getConsoleDescription (ScreenDescription *description) {
  if (virtualTerminal) {
    description->number = virtualTerminal;
  } else {
    struct vt_stat state;
    if (controlConsole(VT_GETSTATE, &state) == -1) {
      LogError("ioctl VT_GETSTATE");
      description->number = 0;
      problemText = "can't get virtual terminal number";
      return 0;
    }
    description->number = state.v_active;

    if (currentConsoleNumber && (description->number != currentConsoleNumber) && !rebindConsole()) {
      problemText = "can't access console";
      return 0;
    }
  }

  if (currentConsoleNumber != description->number) {
    currentConsoleNumber = description->number;
    setTranslationTable(1);
  }

  {
    int mode;
    if (controlConsole(KDGETMODE, &mode) == -1) {
      LogError("ioctl KDGETMODE");
    } else if (mode == KD_TEXT) {
      problemText = NULL;
      return 1;
    }
    problemText = "screen not in text mode";
    return 0;
  }
}

static void
describe_LinuxScreen (ScreenDescription *description) {
  getConsoleDescription(description);
  getScreenDescription(description);
  description->unreadable = problemText;

  /* Periodically recalculate font mapping. I don't know any way to be
   * notified when it changes, and the recalculation is not too
   * long/difficult.
   */
  {
    static int timer = 0;
    if (++timer > 100) {
      setTranslationTable(0);
      timer = 0;
    }
  }
}

static int
readCharacters_LinuxScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  short columns;
  short rows;

  if (readScreenDimensions(&columns, &rows)) {
    if (validateScreenBox(box, columns, rows)) {
      if (problemText) {
        setScreenMessage(box, buffer, problemText);
        return 1;
      }

      {
        int row;

        for (row=0; row<box->height; ++row) {
          ScreenCharacter characters[columns];
          if (!readScreenRow(box->top+row, columns, characters, NULL)) return 0;

          memcpy(buffer, &characters[box->left],
                 box->width * sizeof(characters[0]));
          buffer += box->width;
        }

        return 1;
      }
    }
  }

  return 0;
}

static int
getCapsLockState (void) {
  char leds;
  if (controlConsole(KDGETLED, &leds) != -1)
    if (leds & LED_CAP)
      return 1;
  return 0;
}

static int
insertUinput (ScreenKey key) {
#ifdef HAVE_LINUX_INPUT_H
  int code;

  switch (key & SCR_KEY_CHAR_MASK) {
    default:                    code = KEY_RESERVED;   break;
    case SCR_KEY_ESCAPE:        code = KEY_ESC;        break;
    case '1':                   code = KEY_1;          break;
    case '2':                   code = KEY_2;          break;
    case '3':                   code = KEY_3;          break;
    case '4':                   code = KEY_4;          break;
    case '5':                   code = KEY_5;          break;
    case '6':                   code = KEY_6;          break;
    case '7':                   code = KEY_7;          break;
    case '8':                   code = KEY_8;          break;
    case '9':                   code = KEY_9;          break;
    case '0':                   code = KEY_0;          break;
    case '-':                   code = KEY_MINUS;      break;
    case '=':                   code = KEY_EQUAL;      break;
    case SCR_KEY_BACKSPACE:     code = KEY_BACKSPACE;  break;
    case SCR_KEY_TAB:           code = KEY_TAB;        break;
    case 'q':                   code = KEY_Q;          break;
    case 'w':                   code = KEY_W;          break;
    case 'e':                   code = KEY_E;          break;
    case 'r':                   code = KEY_R;          break;
    case 't':                   code = KEY_T;          break;
    case 'y':                   code = KEY_Y;          break;
    case 'u':                   code = KEY_U;          break;
    case 'i':                   code = KEY_I;          break;
    case 'o':                   code = KEY_O;          break;
    case 'p':                   code = KEY_P;          break;
    case '[':                   code = KEY_LEFTBRACE;  break;
    case ']':                   code = KEY_RIGHTBRACE; break;
    case SCR_KEY_ENTER:         code = KEY_ENTER;      break;
                                    /* KEY_LEFTCTRL */
    case 'a':                   code = KEY_A;          break;
    case 's':                   code = KEY_S;          break;
    case 'd':                   code = KEY_D;          break;
    case 'f':                   code = KEY_F;          break;
    case 'g':                   code = KEY_G;          break;
    case 'h':                   code = KEY_H;          break;
    case 'j':                   code = KEY_J;          break;
    case 'k':                   code = KEY_K;          break;
    case 'l':                   code = KEY_L;          break;
    case ';':                   code = KEY_SEMICOLON;  break;
    case '\'':                  code = KEY_APOSTROPHE; break;
    case '`':                   code = KEY_GRAVE;      break;
                                    /* KEY_LEFTSHIFT */
    case '\\':                  code = KEY_BACKSLASH;  break;
    case 'z':                   code = KEY_Z;          break;
    case 'x':                   code = KEY_X;          break;
    case 'c':                   code = KEY_C;          break;
    case 'v':                   code = KEY_V;          break;
    case 'b':                   code = KEY_B;          break;
    case 'n':                   code = KEY_N;          break;
    case 'm':                   code = KEY_M;          break;
    case ',':                   code = KEY_COMMA;      break;
    case '.':                   code = KEY_DOT;        break;
    case '/':                   code = KEY_SLASH;      break;
                                    /* KEY_RIGHTSHIFT */
                                    /* KEY_KPASTERISK */
                                    /* KEY_LEFTALT */
    case ' ':                   code = KEY_SPACE;      break;
                                    /* KEY_CAPSLOCK */
    case SCR_KEY_FUNCTION +  0: code = KEY_F1;         break;
    case SCR_KEY_FUNCTION +  1: code = KEY_F2;         break;
    case SCR_KEY_FUNCTION +  2: code = KEY_F3;         break;
    case SCR_KEY_FUNCTION +  3: code = KEY_F4;         break;
    case SCR_KEY_FUNCTION +  4: code = KEY_F5;         break;
    case SCR_KEY_FUNCTION +  5: code = KEY_F6;         break;
    case SCR_KEY_FUNCTION +  6: code = KEY_F7;         break;
    case SCR_KEY_FUNCTION +  7: code = KEY_F8;         break;
    case SCR_KEY_FUNCTION +  8: code = KEY_F9;         break;
    case SCR_KEY_FUNCTION +  9: code = KEY_F10;        break;

    case SCR_KEY_FUNCTION + 10: code = KEY_F11;        break;
    case SCR_KEY_FUNCTION + 11: code = KEY_F12;        break;

    case SCR_KEY_HOME:          code = KEY_HOME;       break;
    case SCR_KEY_CURSOR_UP:     code = KEY_UP;         break;
    case SCR_KEY_PAGE_UP:       code = KEY_PAGEUP;     break;
    case SCR_KEY_CURSOR_LEFT:   code = KEY_LEFT;       break;
    case SCR_KEY_CURSOR_RIGHT:  code = KEY_RIGHT;      break;
    case SCR_KEY_END:           code = KEY_END;        break;
    case SCR_KEY_CURSOR_DOWN:   code = KEY_DOWN;       break;
    case SCR_KEY_PAGE_DOWN:     code = KEY_PAGEDOWN;   break;
    case SCR_KEY_INSERT:        code = KEY_INSERT;     break;
    case SCR_KEY_DELETE:        code = KEY_DELETE;     break;

    case SCR_KEY_FUNCTION + 12: code = KEY_F13;        break;
    case SCR_KEY_FUNCTION + 13: code = KEY_F14;        break;
    case SCR_KEY_FUNCTION + 14: code = KEY_F15;        break;
    case SCR_KEY_FUNCTION + 15: code = KEY_F16;        break;
    case SCR_KEY_FUNCTION + 16: code = KEY_F17;        break;
    case SCR_KEY_FUNCTION + 17: code = KEY_F18;        break;
    case SCR_KEY_FUNCTION + 18: code = KEY_F19;        break;
    case SCR_KEY_FUNCTION + 19: code = KEY_F20;        break;
    case SCR_KEY_FUNCTION + 20: code = KEY_F21;        break;
    case SCR_KEY_FUNCTION + 21: code = KEY_F22;        break;
    case SCR_KEY_FUNCTION + 22: code = KEY_F23;        break;
    case SCR_KEY_FUNCTION + 23: code = KEY_F24;        break;
  }

  if (code != KEY_RESERVED) {
#define KEY_EVENT(k,p) { if (!writeKeyEvent((k), (p))) return 0; }
    int modCaps = (key & SCR_KEY_UPPER) && !getCapsLockState();
    int modShift = !!(key & SCR_KEY_SHIFT);
    int modControl = !!(key & SCR_KEY_CONTROL);
    int modAltLeft = !!(key & SCR_KEY_ALT_LEFT);
    int modAltRight = !!(key & SCR_KEY_ALT_RIGHT);

    if (modCaps) {
      KEY_EVENT(KEY_CAPSLOCK, 1);
      KEY_EVENT(KEY_CAPSLOCK, 0);
    }
    if (modShift) KEY_EVENT(KEY_LEFTSHIFT, 1);
    if (modControl) KEY_EVENT(KEY_LEFTCTRL, 1);
    if (modAltLeft) KEY_EVENT(KEY_LEFTALT, 1);
    if (modAltRight) KEY_EVENT(KEY_RIGHTALT, 1);

    KEY_EVENT(code, 1);
    KEY_EVENT(code, 0);

    if (modAltRight) KEY_EVENT(KEY_RIGHTALT, 0);
    if (modAltLeft) KEY_EVENT(KEY_LEFTALT, 0);
    if (modControl) KEY_EVENT(KEY_LEFTCTRL, 0);
    if (modShift) KEY_EVENT(KEY_LEFTSHIFT, 0);
    if (modCaps) {
      KEY_EVENT(KEY_CAPSLOCK, 1);
      KEY_EVENT(KEY_CAPSLOCK, 0);
    }
#undef KEY_EVENT

    return 1;
  }
#endif /* HAVE_LINUX_INPUT_H */

  return 0;
}

static int
insertByte (char byte) {
  if (controlConsole(TIOCSTI, &byte) != -1) return 1;
  LogError("ioctl TIOCSTI");
  return 0;
}

static int
insertBytes (const char *byte, size_t count) {
  while (count) {
    if (!insertByte(*byte++)) return 0;
    count -= 1;
  }
  return 1;
}

static int
insertXlate (wchar_t character) {
  char bytes[MB_LEN_MAX];
  size_t count;
  CharacterConversionResult result = convertWcharToChars(character, bytes, sizeof(bytes), &count);

  if (result != CONV_OK) {
    uint32_t value = character;
    LogPrint(LOG_WARNING, "character 0X%02" PRIX32 " not insertable in xlate mode." , value);
    return 0;
  }

  return insertBytes(bytes, count);
}

static int
insertUnicode (wchar_t character) {
  {
    Utf8Buffer utf8;
    size_t utfs = convertWcharToUtf8(character, utf8);
    if (utfs) return insertBytes(utf8, utfs);
  }

  {
    uint32_t value = character;
    LogPrint(LOG_WARNING, "character 0X%02" PRIX32 " not insertable in unicode mode." , value);
  }

  return 0;
}

static const unsigned char emul0XtScanCodeToLinuxKeyCode[0X80] = {
  [0X1C] = 0X60, /* KEY_KPENTER */
  [0X1D] = 0X61, /* KEY_RIGHTCTRL */
  [0X35] = 0X62, /* KEY_KPSLASH */
  [0X37] = 0X63, /* KEY_SYSRQ */
  [0X38] = 0X64, /* KEY_RIGHTALT */
  [0X47] = 0X66, /* KEY_HOME */
  [0X48] = 0X67, /* KEY_CURSOR_UP */
  [0X49] = 0X68, /* KEY_PAGE_UP */
  [0X4B] = 0X69, /* KEY_CURSOR_LEFT */
  [0X4D] = 0X6A, /* KEY_CURSOR_RIGHT */
  [0X4F] = 0X6B, /* KEY_END */
  [0X50] = 0X6C, /* KEY_CURSOR_DOWN */
  [0X51] = 0X6D, /* KEY_PAGE_DOWN */
  [0X52] = 0X6E, /* KEY_INSERT */
  [0X53] = 0X6F, /* KEY_DELETE */
  [0X5B] = 0X7D, /* KEY_LEFTMETA */
  [0X5C] = 0X7E, /* KEY_RIGHTMETA */
  [0X5D] = 0X7F, /* KEY_COMPOSE */
};

static const unsigned int emul1XtScanCodeToLinuxKeyCode[0X80] = {
  [0X01] = 0X1D0, /* KEY_FN */
  [0X1D] = 0X77,  /* KEY_PAUSE */
};

static int
insertCode (ScreenKey key, int raw) {
  unsigned char prefix = 0X00;
  unsigned char code;

  setKeyModifiers(&key, SCR_KEY_SHIFT | SCR_KEY_CONTROL);

  switch (key & SCR_KEY_CHAR_MASK) {
    case SCR_KEY_ESCAPE:        code = 0X01; break;
    case SCR_KEY_FUNCTION +  0: code = 0X3B; break;
    case SCR_KEY_FUNCTION +  1: code = 0X3C; break;
    case SCR_KEY_FUNCTION +  2: code = 0X3D; break;
    case SCR_KEY_FUNCTION +  3: code = 0X3E; break;
    case SCR_KEY_FUNCTION +  4: code = 0X3F; break;
    case SCR_KEY_FUNCTION +  5: code = 0X40; break;
    case SCR_KEY_FUNCTION +  6: code = 0X41; break;
    case SCR_KEY_FUNCTION +  7: code = 0X42; break;
    case SCR_KEY_FUNCTION +  8: code = 0X43; break;
    case SCR_KEY_FUNCTION +  9: code = 0X44; break;
    case SCR_KEY_FUNCTION + 10: code = 0X57; break;
    case SCR_KEY_FUNCTION + 11: code = 0X58; break;
    case '`':                   code = 0X29; break;
    case '1':                   code = 0X02; break;
    case '2':                   code = 0X03; break;
    case '3':                   code = 0X04; break;
    case '4':                   code = 0X05; break;
    case '5':                   code = 0X06; break;
    case '6':                   code = 0X07; break;
    case '7':                   code = 0X08; break;
    case '8':                   code = 0X09; break;
    case '9':                   code = 0X0A; break;
    case '0':                   code = 0X0B; break;
    case '-':                   code = 0X0C; break;
    case '=':                   code = 0X0D; break;
    case SCR_KEY_BACKSPACE:     code = 0X0E; break;
    case SCR_KEY_TAB:           code = 0X0F; break;
    case 'q':                   code = 0X10; break;
    case 'w':                   code = 0X11; break;
    case 'e':                   code = 0X12; break;
    case 'r':                   code = 0X13; break;
    case 't':                   code = 0X14; break;
    case 'y':                   code = 0X15; break;
    case 'u':                   code = 0X16; break;
    case 'i':                   code = 0X17; break;
    case 'o':                   code = 0X18; break;
    case 'p':                   code = 0X19; break;
    case '[':                   code = 0X1A; break;
    case ']':                   code = 0X1B; break;
    case '\\':                  code = 0X2B; break;
    case 'a':                   code = 0X1E; break;
    case 's':                   code = 0X1F; break;
    case 'd':                   code = 0X20; break;
    case 'f':                   code = 0X21; break;
    case 'g':                   code = 0X22; break;
    case 'h':                   code = 0X23; break;
    case 'j':                   code = 0X24; break;
    case 'k':                   code = 0X25; break;
    case 'l':                   code = 0X26; break;
    case ';':                   code = 0X27; break;
    case '\'':                  code = 0X28; break;
    case SCR_KEY_ENTER:         code = 0X1C; break;
    case 'z':                   code = 0X2C; break;
    case 'x':                   code = 0X2D; break;
    case 'c':                   code = 0X2E; break;
    case 'v':                   code = 0X2F; break;
    case 'b':                   code = 0X30; break;
    case 'n':                   code = 0X31; break;
    case 'm':                   code = 0X32; break;
    case ',':                   code = 0X33; break;
    case '.':                   code = 0X34; break;
    case '/':                   code = 0X35; break;
    case ' ':                   code = 0X39; break;
    default:
      switch (key & SCR_KEY_CHAR_MASK) {
        case SCR_KEY_INSERT:       code = 0X52; break;
        case SCR_KEY_HOME:         code = 0X47; break;
        case SCR_KEY_PAGE_UP:      code = 0X49; break;
        case SCR_KEY_DELETE:       code = 0X53; break;
        case SCR_KEY_END:          code = 0X4F; break;
        case SCR_KEY_PAGE_DOWN:    code = 0X51; break;
        case SCR_KEY_CURSOR_UP:    code = 0X48; break;
        case SCR_KEY_CURSOR_LEFT:  code = 0X4B; break;
        case SCR_KEY_CURSOR_DOWN:  code = 0X50; break;
        case SCR_KEY_CURSOR_RIGHT: code = 0X4D; break;
        default:
          if (insertUinput(key)) return 1;
          LogPrint(LOG_WARNING, "key %04X not suported in raw keycode mode.", key);
          return 0;
      }

      if (raw) {
        prefix = 0XE0;
      } else if (!(code = emul0XtScanCodeToLinuxKeyCode[code])) {
        LogPrint(LOG_WARNING, "key %04X not suported in medium raw keycode mode.", key);
        return 0;
      }
      break;
  }

  {
    int modCaps = (key & SCR_KEY_UPPER) && !getCapsLockState();
    int modShift = !!(key & SCR_KEY_SHIFT);
    int modControl = !!(key & SCR_KEY_CONTROL);
    int modAltLeft = !!(key & SCR_KEY_ALT_LEFT);
    int modAltRight = !!(key & SCR_KEY_ALT_RIGHT);

    const char codeCapsLock = 0X3A;
    const char codeShift = 0X2A;
    const char codeControl = 0X1D;
    const char codeAlt = 0X38;
    const char codeEmul0 = 0XE0;
    const char bitRelease = 0X80;

    char codes[18];
    unsigned int count = 0;

    if (modCaps) {
      codes[count++] = codeCapsLock;
      codes[count++] = codeCapsLock | bitRelease;
    }
    if (modShift) codes[count++] = codeShift;
    if (modControl) codes[count++] = codeControl;
    if (modAltLeft) codes[count++] = codeAlt;
    if (modAltRight) {
      if (raw) {
        codes[count++] = codeEmul0;
        codes[count++] = codeAlt;
      } else {
        codes[count++] = emul0XtScanCodeToLinuxKeyCode[codeAlt & 0XFF];
      }
    }
    if (prefix) codes[count++] = prefix;
    codes[count++] = code;

    if (prefix) codes[count++] = prefix;
    codes[count++] = code | bitRelease;
    if (modAltRight) {
      if (raw) {
        codes[count++] = codeEmul0;
        codes[count++] = codeAlt | bitRelease;;
      } else {
        codes[count++] = emul0XtScanCodeToLinuxKeyCode[codeAlt & 0XFF] | bitRelease;;
      }
    }
    if (modAltLeft) codes[count++] = codeAlt | bitRelease;
    if (modControl) codes[count++] = codeControl | bitRelease;
    if (modShift) codes[count++] = codeShift | bitRelease;
    if (modCaps) {
      codes[count++] = codeCapsLock;
      codes[count++] = codeCapsLock | bitRelease;
    }

    return insertBytes(codes, count);
  }
}

static int
insertTranslated (ScreenKey key, int (*insertCharacter)(wchar_t character)) {
  wchar_t buffer[2];
  wchar_t *sequence;
  wchar_t *end;

  setKeyModifiers(&key, 0);

  if (isSpecialKey(key)) {
    switch (key) {
      case SCR_KEY_ENTER:
        sequence = WS_C("\r");
        break;
      case SCR_KEY_TAB:
        sequence = WS_C("\t");
        break;
      case SCR_KEY_BACKSPACE:
        sequence = WS_C("\x7f");
        break;
      case SCR_KEY_ESCAPE:
        sequence = WS_C("\x1b");
        break;
      case SCR_KEY_CURSOR_LEFT:
        sequence = WS_C("\x1b[D");
        break;
      case SCR_KEY_CURSOR_RIGHT:
        sequence = WS_C("\x1b[C");
        break;
      case SCR_KEY_CURSOR_UP:
        sequence = WS_C("\x1b[A");
        break;
      case SCR_KEY_CURSOR_DOWN:
        sequence = WS_C("\x1b[B");
        break;
      case SCR_KEY_PAGE_UP:
        sequence = WS_C("\x1b[5~");
        break;
      case SCR_KEY_PAGE_DOWN:
        sequence = WS_C("\x1b[6~");
        break;
      case SCR_KEY_HOME:
        sequence = WS_C("\x1b[1~");
        break;
      case SCR_KEY_END:
        sequence = WS_C("\x1b[4~");
        break;
      case SCR_KEY_INSERT:
        sequence = WS_C("\x1b[2~");
        break;
      case SCR_KEY_DELETE:
        sequence = WS_C("\x1b[3~");
        break;
      case SCR_KEY_FUNCTION + 0:
        sequence = WS_C("\x1b[[A");
        break;
      case SCR_KEY_FUNCTION + 1:
        sequence = WS_C("\x1b[[B");
        break;
      case SCR_KEY_FUNCTION + 2:
        sequence = WS_C("\x1b[[C");
        break;
      case SCR_KEY_FUNCTION + 3:
        sequence = WS_C("\x1b[[D");
        break;
      case SCR_KEY_FUNCTION + 4:
        sequence = WS_C("\x1b[[E");
        break;
      case SCR_KEY_FUNCTION + 5:
        sequence = WS_C("\x1b[17~");
        break;
      case SCR_KEY_FUNCTION + 6:
        sequence = WS_C("\x1b[18~");
        break;
      case SCR_KEY_FUNCTION + 7:
        sequence = WS_C("\x1b[19~");
        break;
      case SCR_KEY_FUNCTION + 8:
        sequence = WS_C("\x1b[20~");
        break;
      case SCR_KEY_FUNCTION + 9:
        sequence = WS_C("\x1b[21~");
        break;
      case SCR_KEY_FUNCTION + 10:
        sequence = WS_C("\x1b[23~");
        break;
      case SCR_KEY_FUNCTION + 11:
        sequence = WS_C("\x1b[24~");
        break;
      case SCR_KEY_FUNCTION + 12:
        sequence = WS_C("\x1b[25~");
        break;
      case SCR_KEY_FUNCTION + 13:
        sequence = WS_C("\x1b[26~");
        break;
      case SCR_KEY_FUNCTION + 14:
        sequence = WS_C("\x1b[28~");
        break;
      case SCR_KEY_FUNCTION + 15:
        sequence = WS_C("\x1b[29~");
        break;
      case SCR_KEY_FUNCTION + 16:
        sequence = WS_C("\x1b[31~");
        break;
      case SCR_KEY_FUNCTION + 17:
        sequence = WS_C("\x1b[32~");
        break;
      case SCR_KEY_FUNCTION + 18:
        sequence = WS_C("\x1b[33~");
        break;
      case SCR_KEY_FUNCTION + 19:
        sequence = WS_C("\x1b[34~");
        break;
      default:
	if (insertUinput(key)) return 1;
        LogPrint(LOG_WARNING, "key %04X not supported in xlate mode.", key);
        return 0;
    }
    end = sequence + wcslen(sequence);
  } else {
    sequence = end = buffer + ARRAY_COUNT(buffer);
    *--sequence = key & SCR_KEY_CHAR_MASK;

    if (key & SCR_KEY_ALT_LEFT) {
      int meta;
      if (controlConsole(KDGKBMETA, &meta) == -1) return 0;

      switch (meta) {
        case K_ESCPREFIX:
          *--sequence = 0X1B;
          break;

        case K_METABIT:
          if (*sequence < 0X80) {
            *sequence |= 0X80;
            break;
          }

        default:
          LogPrint(LOG_WARNING, "unsupported keyboard meta mode: %d", meta);
          return 0;
      }
    }
  }

  while (sequence != end)
    if (!insertCharacter(*sequence++))
      return 0;
  return 1;
}

static int
insertKey_LinuxScreen (ScreenKey key) {
  int ok = 0;
  LogPrint(LOG_DEBUG, "insert key: %4.4X", key);
  if (rebindConsole()) {
    int mode;
    if (controlConsole(KDGKBMODE, &mode) != -1) {
      switch (mode) {
        case K_RAW:
          if (insertCode(key, 1)) ok = 1;
          break;
        case K_MEDIUMRAW:
          if (insertCode(key, 0)) ok = 1;
          break;
        case K_XLATE:
          if (insertTranslated(key, insertXlate)) ok = 1;
          break;
        case K_UNICODE:
          if (insertTranslated(key, insertUnicode)) ok = 1;
          break;
        default:
          LogPrint(LOG_WARNING, "unsupported keyboard mode: %d", mode);
          break;
      }
    } else {
      LogError("ioctl KDGKBMODE");
    }
  }
  return ok;
}

typedef struct {
  char subcode;
  short xs;
  short ys;
  short xe;
  short ye;
  short mode;
} PACKED CharacterSelectionArguments;

static int
selectCharacters (CharacterSelectionArguments *arguments) {
  if (controlConsole(TIOCLINUX, arguments) != -1) return 1;
  if (errno != EINVAL) LogError("ioctl[TIOCLINUX]");
  return 0;
}

static int
highlightRegion_LinuxScreen (int left, int right, int top, int bottom) {
  CharacterSelectionArguments arguments = {
    .subcode = 2,
    .xs = left + 1,
    .ys = top + 1,
    .xe = right + 1,
    .ye = bottom + 1,
    .mode = 0
  };
  return selectCharacters(&arguments);
}

static int
unhighlightRegion_LinuxScreen (void) {
  CharacterSelectionArguments arguments = {
    .subcode = 2,
    .xs = 0,
    .ys = 0,
    .xe = 0,
    .ye = 0,
    .mode = 4
  };
  return selectCharacters(&arguments);
}

static int
validateVt (int vt) {
  if ((vt >= 1) && (vt <= 0X3F)) return 1;
  LogPrint(LOG_DEBUG, "virtual terminal out of range: %d", vt);
  return 0;
}

static int
selectVirtualTerminal_LinuxScreen (int vt) {
  if (vt == virtualTerminal) return 1;
  if (vt && !validateVt(vt)) return 0;
  return openScreen(vt);
}

static int
switchVirtualTerminal_LinuxScreen (int vt) {
  if (validateVt(vt)) {
    if (selectVirtualTerminal_LinuxScreen(0)) {
      if (ioctl(consoleDescriptor, VT_ACTIVATE, vt) != -1) {
        LogPrint(LOG_DEBUG, "switched to virtual tertminal %d.", vt);
        return 1;
      } else {
        LogError("ioctl VT_ACTIVATE");
      }
    }
  }
  return 0;
}

static int
currentVirtualTerminal_LinuxScreen (void) {
  ScreenDescription description;
  getConsoleDescription(&description);
  return description.number;
}

static int
executeCommand_LinuxScreen (int command) {
  int blk = command & BRL_MSK_BLK;
  int arg UNUSED = command & BRL_MSK_ARG;
  int cmd = blk | arg;

  switch (cmd) {
    case BRL_CMD_RESTARTBRL:
#ifdef HAVE_LINUX_INPUT_H
      releaseAllKeys();
#endif /* HAVE_LINUX_INPUT_H */
      break;

    default:
#ifdef HAVE_LINUX_INPUT_H
      switch (blk) {
        case BRL_BLK_PASSXT:
	  {
            int press = !(arg & 0X80);
            arg &= 0X7F;

            if (command & BRL_FLG_KBD_EMUL0) {
              unsigned char code = emul0XtScanCodeToLinuxKeyCode[arg];
              if (!code) {
		LogPrint(LOG_WARNING, "Xt emul0 scancode not supported: %02X", arg);
                return 0;
              }
              arg = code;
	    } else if (command & BRL_FLG_KBD_EMUL1) {
              unsigned int code = emul1XtScanCodeToLinuxKeyCode[arg];
              if (!code) {
		LogPrint(LOG_WARNING, "Xt emul1 scancode not supported: %02X", arg);
                return 0;
              }
              arg = code;
	    }

            return writeKeyEvent(arg, press);
	  }

	case BRL_BLK_PASSAT:
          {
            int handled = 0;

            if (command & BRL_FLG_KBD_RELEASE) {
              at2Pressed = 0;
            } else if (arg == 0XF0) {
              at2Pressed = 0;
              handled = 1;
            }

            if (command & BRL_FLG_KBD_EMUL0) {
              at2Keys = at2KeysE0;
            } else if (arg == 0XE0) {
              at2Keys = at2KeysE0;
              handled = 1;
	    } else if (command & BRL_FLG_KBD_EMUL1) {
              at2Keys = at2KeysE1;
            } else if (arg == 0XE1) {
              at2Keys = at2KeysE1;
              handled = 1;
            }

            if (handled) return 1;
          }

          {
            unsigned char key = at2Keys[arg];
            int pressed = at2Pressed;

            at2Keys = at2KeysOriginal;
            at2Pressed = 1;

            if (key) return writeKeyEvent(key, pressed);
          }
          break;
      }
#endif /* HAVE_LINUX_INPUT_H */
      break;
  }

  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.describe = describe_LinuxScreen;
  main->base.readCharacters = readCharacters_LinuxScreen;
  main->base.insertKey = insertKey_LinuxScreen;
  main->base.highlightRegion = highlightRegion_LinuxScreen;
  main->base.unhighlightRegion = unhighlightRegion_LinuxScreen;
  main->base.selectVirtualTerminal = selectVirtualTerminal_LinuxScreen;
  main->base.switchVirtualTerminal = switchVirtualTerminal_LinuxScreen;
  main->base.currentVirtualTerminal = currentVirtualTerminal_LinuxScreen;
  main->base.executeCommand = executeCommand_LinuxScreen;
  main->processParameters = processParameters_LinuxScreen;
  main->construct = construct_LinuxScreen;
  main->destruct = destruct_LinuxScreen;
  main->userVirtualTerminal = userVirtualTerminal_LinuxScreen;

#ifdef HAVE_LINUX_INPUT_H
  at2Keys = at2KeysOriginal;
  at2Pressed = 1;
#endif /* HAVE_LINUX_INPUT_H */
}

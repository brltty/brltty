/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/tty.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/tiocl.h>

#include "log.h"
#include "report.h"
#include "async_io.h"
#include "device.h"
#include "io_misc.h"
#include "parse.h"
#include "brl_cmds.h"
#include "kbd_keycodes.h"
#include "ascii.h"
#include "charset.h"
#include "system_linux.h"

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
      logSystemError("iconv_open");
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
  logSystemError("iconv");
  return CONV_ERROR;
}
#else /* charset conversion definitions */
typedef struct {
  char aStructNeedsAtLeastOneField;
} CharsetConverter;

#define CHARSET_CONVERTER_INITIALIZER {0}

static int
allocateCharsetConverter (CharsetConverter *converter, const char *sourceCharset, const char *targetCharset) {
  return 1;
}

static void
deallocateCharsetConverter (CharsetConverter *converter) {
}

static CharacterConversionResult
convertCharacters (
  CharsetConverter *converter,
  const char **inputAddress, size_t *inputLength,
  char **outputAddress, size_t *outputLength
) {
  *(*outputAddress)++ = *(*inputAddress)++;
  *inputLength -= 1;
  *outputLength -= 1;
  return CONV_OK;
}
#endif /* charset conversion definitions */

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
          logMallocError();
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
      memmove(buffer, buffer+1, length);
    }
  }

  spaces += 1;
  return WEOF;
}

static int
setDeviceName (const char **name, const char *const *names, const char *description) {
  return (*name = resolveDeviceName(names, description)) != NULL;
}

static char *
vtName (const char *name, unsigned char vt) {
  char *string;

  if (vt) {
    size_t length = strlen(name);
    char buffer[length+4];

    if (name[length-1] == '0') length -= 1;
    strncpy(buffer, name, length);
    sprintf(buffer+length,  "%u", vt);
    string = strdup(buffer);
  } else {
    string = strdup(name);
  }

  if (!string) logMallocError();
  return string;
}

static const char *consoleName = NULL;
static int consoleDescriptor;

static int
setConsoleName (void) {
  static const char *const names[] = {"tty0", "vc/0", NULL};
  return setDeviceName(&consoleName, names, "console");
}

static void
closeConsole (void) {
  if (consoleDescriptor != -1) {
    if (close(consoleDescriptor) == -1) {
      logSystemError("console close");
    }

    logMessage(LOG_DEBUG, "console closed: fd=%d", consoleDescriptor);
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
      logMessage(LOG_DEBUG, "console opened: %s: fd=%d", name, console);
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

static int isMonitorable;
static AsyncHandle screenMonitor;

static int screenUpdated;
static unsigned char *cacheBuffer;
static size_t cacheSize;

static int currentConsoleNumber;
static int inTextMode;

typedef struct {
  unsigned char rows;
  unsigned char columns;
} ScreenSize;

typedef struct {
  unsigned char column;
  unsigned char row;
} ScreenLocation;

typedef struct {
  ScreenSize size;
  ScreenLocation location;
} ScreenHeader;

#ifdef HAVE_SYS_POLL_H
#include <poll.h>

static int
canMonitorScreen (void) {
  struct pollfd pollDescriptor = {
    .fd = screenDescriptor,
    .events = POLLPRI
  };

  return poll(&pollDescriptor, 1, 0) == 1;
}

#else /* can poll */
static int
canMonitorScreen (void) {
  return 0;
}
#endif /* can poll */

static size_t
readScreenDevice (off_t offset, void *buffer, size_t size) {
  const ssize_t count = pread(screenDescriptor, buffer, size, offset);

  if (count == -1) {
    logSystemError("screen read");
  } else {
    return count;
  }

  return 0;
}

ASYNC_MONITOR_CALLBACK(lxScreenUpdated) {
  asyncDiscardHandle(screenMonitor);
  screenMonitor = NULL;

  screenUpdated = 1;
  mainScreenUpdated();

  return 0;
}

static int
setScreenName (void) {
  static const char *const names[] = {"vcsa", "vcsa0", "vcc/a", NULL};
  return setDeviceName(&screenName, names, "screen");
}

static void
closeScreen (void) {
  if (screenMonitor) {
    asyncCancelRequest(screenMonitor);
    screenMonitor = NULL;
  }

  if (screenDescriptor != -1) {
    if (close(screenDescriptor) == -1) {
      logSystemError("screen close");
    }

    logMessage(LOG_DEBUG, "screen closed: fd=%d", screenDescriptor);
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
      logMessage(LOG_DEBUG, "screen opened: %s: fd=%d", name, screen);

      if (openConsole(vt)) {
        closeScreen();
        screenDescriptor = screen;
        virtualTerminal = vt;
        opened = 1;

        isMonitorable = canMonitorScreen();
        logMessage(LOG_DEBUG, "screen is monitorable: %s",
                   (isMonitorable? "yes": "no"));

        screenMonitor = NULL;
        screenUpdated = 1;
      } else {
        close(screen);
        logMessage(LOG_DEBUG, "screen closed: fd=%d", screen);
      }
    }

    free(name);
  }

  return opened;
}

static size_t
readScreenCache (off_t offset, void *buffer, size_t size) {
  if (offset <= cacheSize) {
    size_t left = cacheSize - offset;

    if (size > left) size = left;
    memcpy(buffer, &cacheBuffer[offset], size);
    return size;
  } else {
    logMessage(LOG_ERR, "invalid screen cache offset: %u", (unsigned int)offset);
  }

  return 0;
}

static int
readScreenData (off_t offset, void *buffer, size_t size) {
  size_t count = (cacheBuffer? readScreenCache: readScreenDevice)(offset, buffer, size);
  if (count == size) return 1;

  logMessage(LOG_ERR,
             "truncated screen data: expected %zu bytes but read %zu",
             size, count);
  return 0;
}

static int
readScreenHeader (ScreenHeader *header) {
  return readScreenData(0, header, sizeof(*header));
}

static int
readScreenSize (ScreenSize *size) {
  return readScreenData(0, size, sizeof(*size));
}

static int
readScreenContent (off_t offset, uint16_t *buffer, size_t count) {
  count *= sizeof(*buffer);
  offset *= sizeof(*buffer);
  offset += sizeof(ScreenHeader);
  return readScreenData(offset, buffer, count);
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
      logMallocError();
      return 0;
    }

    if (controlConsole(GIO_UNIMAP, &sfm) != -1) break;
    free(sfm.entries);

    if (errno != ENOMEM) {
      logSystemError("ioctl[GIO_UNIMAP]");
      return 0;
    }

    if (!(size <<= 1)) {
      logMessage(LOG_ERR, "screen font map too big");
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
  }

  if (screenFontMapTable) free(screenFontMapTable);
  screenFontMapTable = sfm.entries;
  screenFontMapCount = sfm.entry_ct;
  screenFontMapSize = size;
  logMessage(LOG_INFO, "Screen Font Map Size: %d", screenFontMapCount);

  if (debugScreenFontMap) {
    unsigned int i;

    for (i=0; i<screenFontMapCount; ++i) {
      const struct unipair *map = &screenFontMapTable[i];

      logMessage(LOG_DEBUG, "sfm[%03u]: unum=%4.4X fpos=%4.4X",
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
    cfo.height = UINT_MAX;
    cfo.width = UINT_MAX;

    if (controlConsole(KDFONTOP, &cfo) != -1) {
      vgaCharacterCount = cfo.charcount;
    } else {
      vgaCharacterCount = 0;

      if (errno != EINVAL) {
        logMessage(LOG_WARNING, "ioctl[KDFONTOP[GET]]: %s", strerror(errno));
      }
    }
  }

  if (!vgaCharacterCount) {
    unsigned int index;

    for (index=0; index<screenFontMapCount; ++index) {
      const struct unipair *map = &screenFontMapTable[index];

      if (vgaCharacterCount <= map->fontpos) vgaCharacterCount = map->fontpos + 1;
    }
  }

  vgaCharacterCount = ((vgaCharacterCount - 1) | 0XFF) + 1;
  vgaLargeTable = vgaCharacterCount > 0X100;

  if (!force) {
    if (vgaCharacterCount == oldCount) {
      return 0;
    }
  }

  logMessage(LOG_INFO, "VGA Character Count: %d(%s)",
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
  logMessage(LOG_DEBUG, "attributes masks: font=%04X unshifted=%04X shifted=%04X",
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
        if (errno != EINVAL) logSystemError("ioctl[VT_GETHIFONTMASK]");
      } else if (mask & 0XFF) {
        logMessage(LOG_ERR, "high font mask has bit set in low-order byte: %04X", mask);
      } else {
        setAttributesMasks(mask);
        return 1;
      }
    }

    {
      ScreenSize size;

      if (readScreenSize(&size)) {
        const size_t count = size.columns * size.rows;
        unsigned short buffer[count];

        if (readScreenContent(0, buffer, ARRAY_COUNT(buffer))) {
          int counts[0X10];
          unsigned int index;

          memset(counts, 0, sizeof(counts));

          for (index=0; index<count; index+=1) ++counts[(buffer[index] & 0X0F00) >> 8];

          setAttributesMasks((counts[0XE] > counts[0X7])? 0X0100: 0X0800);
          return 1;
        }
      }
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

  if (!validateYesNo(&debugScreenFontMap, parameters[PARM_DEBUGSFM])) {
    logMessage(LOG_WARNING, "%s: %s", "invalid screen font map debug setting", parameters[PARM_DEBUGSFM]);
  }

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
      logMessage(LOG_WARNING, "%s: %s", "invalid high font bit", parameters[PARM_HFB]);
    } else if (choice) {
      static const unsigned short bits[] = {0X0800, 0X0100};
      highFontBit = bits[choice-1];
    }
  }

  return 1;
}

static void
releaseParameters_LinuxScreen (void) {
  deallocateCharsetEntries();
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
      unsigned int i;

      for (i=0; i<count; ++i) {
        translationTable[i] = UNICODE_ROW_DIRECT | i;
      }
    }

    {
      unsigned int screenFontMapIndex = screenFontMapCount;

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

static const LinuxKeyCode *xtKeys;
static const LinuxKeyCode *atKeys;
static int atKeyPressed;
static int ps2KeyPressed;
#endif /* HAVE_LINUX_INPUT_H */

static UinputObject *uinputKeyboard = NULL;
static ReportListenerInstance *brailleOfflineListener;

static void
closeKeyboard (void) {
  if (uinputKeyboard) {
    destroyUinputObject(uinputKeyboard);
    uinputKeyboard = NULL;
  }
}

static int
openKeyboard (void) {
  if (!uinputKeyboard) {
    if (!(uinputKeyboard = newUinputKeyboard("Linux Screen Driver Keyboard"))) {
      return 0;
    }

    atexit(closeKeyboard);
  }

  return 1;
}

static void
resetKeyboard (void) {
  if (uinputKeyboard) {
    releasePressedKeys(uinputKeyboard);
  }
}

REPORT_LISTENER(lxBrailleOfflineListener) {
  resetKeyboard();
}

static int
construct_LinuxScreen (void) {
  screenUpdated = 0;
  cacheBuffer = NULL;
  cacheSize = 0;

  currentConsoleNumber = 0;
  inTextMode = 1;

  brailleOfflineListener = NULL;

#ifdef HAVE_LINUX_INPUT_H
  xtKeys = linuxKeyMap_xt00;
  atKeys = linuxKeyMap_at00;
  atKeyPressed = 1;
  ps2KeyPressed = 1;
#endif /* HAVE_LINUX_INPUT_H */

  if (setScreenName()) {
    screenDescriptor = -1;

    if (setConsoleName()) {
      consoleDescriptor = -1;

      if (openScreen(0)) {
        if (setTranslationTable(1)) {
          openKeyboard();
          brailleOfflineListener = registerReportListener(REPORT_BRAILLE_OFFLINE, lxBrailleOfflineListener, NULL);
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

  if (cacheBuffer) {
    free(cacheBuffer);
    cacheBuffer = NULL;
  }
  cacheSize = 0;

  if (brailleOfflineListener) {
    unregisterReportListener(brailleOfflineListener);
    brailleOfflineListener = NULL;
  }
}

static int
userVirtualTerminal_LinuxScreen (int number) {
  return MAX_NR_CONSOLES + 1 + number;
}

static int
readScreenRow (int row, size_t size, ScreenCharacter *characters, int *offsets) {
  uint16_t line[size];
  int column = 0;

  if (readScreenContent((row * size), line, size)) {
    const uint16_t *source = line;
    const uint16_t *end = source + size;
    ScreenCharacter *character = characters;

    while (source != end) {
      uint16_t position = *source & 0XFF;
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
poll_LinuxScreen (void) {
  int poll = !isMonitorable? 1:
             screenMonitor? 0:
             !asyncMonitorFileAlert(&screenMonitor, screenDescriptor,
                                    lxScreenUpdated, NULL);

  if (poll) screenUpdated = 1;
  return poll;
}

static int
getConsoleNumber (void) {
  int number;

  if (virtualTerminal) {
    number = virtualTerminal;
  } else {
    {
      struct vt_stat state;

      if (controlConsole(VT_GETSTATE, &state) == -1) {
        logSystemError("ioctl[VT_GETSTATE]");
        problemText = "can't get console number";
        return 0;
      }

      number = state.v_active;
    }

    if (number != currentConsoleNumber) {
      if (currentConsoleNumber && !rebindConsole()) {
        problemText = "can't access console";
        return 0;
      }

      setTranslationTable(1);
    }
  }

  return number;
}

static int
testTextMode (void) {
  int mode;

  if (controlConsole(KDGETMODE, &mode) == -1) {
    logSystemError("ioctl[KDGETMODE]");
  } else if (mode == KD_TEXT) {
    return 1;
  }

  problemText = gettext("screen not in text mode");
  return 0;
}

static size_t
toScreenCacheSize (const ScreenSize *screenSize) {
  return (screenSize->columns * screenSize->rows * 2) + sizeof(ScreenHeader);
}

static int
refreshCacheBuffer (void) {
  if (!cacheBuffer) {
    ScreenHeader header;

    {
      size_t size = sizeof(header);
      size_t count = readScreenDevice(0, &header, size);

      if (!count) return 0;

      if (count < size) {
        logBytes(LOG_ERR, "truncated screen header", &header, count);
        return 0;
      }
    }

    {
      size_t size = toScreenCacheSize(&header.size);
      unsigned char *buffer = malloc(size);

      if (!buffer) {
        logMallocError();
        return 0;
      }

      cacheBuffer = buffer;
      cacheSize = size;
    }
  }

  while (1) {
    size_t count = readScreenDevice(0, cacheBuffer, cacheSize);

    if (!count) return 0;

    if (count < sizeof(ScreenHeader)) {
      logBytes(LOG_ERR, "truncated screen header", cacheBuffer, count);
      return 0;
    }

    {
      ScreenHeader *header = (void *)cacheBuffer;
      size_t size = toScreenCacheSize(&header->size);

      if (count >= size) return 1;

      {
        unsigned char *buffer = realloc(cacheBuffer, size);

        if (!buffer) {
          logMallocError();
          return 0;
        }

        cacheBuffer = buffer;
        cacheSize = size;
      }
    }
  }
}

static int
refresh_LinuxScreen (void) {
  if (screenUpdated) {
    while (1) {
      problemText = NULL;

      if (!refreshCacheBuffer()) {
        problemText = "cannot read screen content";
        return 0;
      }

      inTextMode = testTextMode();

      {
        int consoleNumber = getConsoleNumber();

        if (consoleNumber == currentConsoleNumber) break;
        logMessage(LOG_DEBUG,
                   "console number changed: %u -> %u",
                   currentConsoleNumber, consoleNumber);

        currentConsoleNumber = consoleNumber;
      }
    }

    screenUpdated = 0;
  }

  return 1;
}

static void
adjustCursorColumn (short *column, short row, short columns) {
  const CharsetEntry *charset = getCharsetEntry();

  if (charset->isMultiByte) {
    int offsets[columns];

    if (readScreenRow(row, columns, NULL, offsets)) {
      int first = 0;
      int last = columns - 1;

      while (first <= last) {
        int current = (first + last) / 2;

        if (offsets[current] < *column) {
          first = current + 1;
        } else {
          last = current - 1;
        }
      }

      if (first == columns) first -= 1;
      *column = first;
    }
  }
}

static int
getScreenDescription (ScreenDescription *description) {
  ScreenHeader header;

  if (readScreenHeader(&header)) {
    description->cols = header.size.columns;
    description->rows = header.size.rows;

    description->posx = header.location.column;
    description->posy = header.location.row;

    adjustCursorColumn(&description->posx, description->posy, description->cols);
    return 1;
  }

  problemText = "screen header read error";
  return 0;
}

static void
describe_LinuxScreen (ScreenDescription *description) {
  if (!cacheBuffer) {
    problemText = NULL;
    inTextMode = testTextMode();
    currentConsoleNumber = getConsoleNumber();
  }

  if ((description->number = currentConsoleNumber)) {
    if (inTextMode) {
      if (getScreenDescription(description)) {
      }
    }
  }

  if ((description->unreadable = problemText)) {
    description->cols = strlen(problemText);
    description->rows = 1;

    description->posx = 0;
    description->posy = 0;
  }
}

static int
readCharacters_LinuxScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  ScreenSize size;

  if (readScreenSize(&size)) {
    if (validateScreenBox(box, size.columns, size.rows)) {
      if (problemText) {
        setScreenMessage(box, buffer, problemText);
        return 1;
      }

      {
        int row;

        for (row=0; row<box->height; ++row) {
          ScreenCharacter characters[size.columns];
          if (!readScreenRow(box->top+row, size.columns, characters, NULL)) return 0;

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

static inline int
hasModUpper (ScreenKey key) {
  return (key & SCR_KEY_UPPER) && !getCapsLockState();
}

static inline int
hasModShift (ScreenKey key) {
  return !!(key & SCR_KEY_SHIFT);
}

static inline int
hasModControl (ScreenKey key) {
  return !!(key & SCR_KEY_CONTROL);
}

static inline int
hasModAltLeft (ScreenKey key) {
  return !!(key & SCR_KEY_ALT_LEFT);
}

static inline int
hasModAltRight (ScreenKey key) {
  return !!(key & SCR_KEY_ALT_RIGHT);
}

static int
injectKeyEvent (int key, int press) {
  if (!openKeyboard()) return 0;
  return writeKeyEvent(uinputKeyboard, key, press);
}

static int
insertUinput (
  LinuxKeyCode code,
  int modUpper, int modShift, int modControl,
  int modAltLeft, int modAltRight
) {
#ifdef HAVE_LINUX_INPUT_H
  if (code) {
#define KEY_EVENT(KEY, PRESS) { if (!injectKeyEvent((KEY), (PRESS))) return 0; }
    if (modUpper) {
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

    if (modUpper) {
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
  logSystemError("ioctl[TIOCSTI]");
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

    logMessage(LOG_WARNING, "character not supported in xlate mode: 0X%02"PRIX32, value);
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

    logMessage(LOG_WARNING, "character not supported in unicode keyboard mode: 0X%02"PRIX32, value);
  }

  return 0;
}

static int
insertCode (ScreenKey key, int raw) {
  const LinuxKeyCode *map;
  unsigned char code;
  unsigned char escape;

  setScreenKeyModifiers(&key, SCR_KEY_SHIFT | SCR_KEY_CONTROL);

#define KEY_TO_XT(KEY, ESCAPE, CODE) \
  case (KEY): \
  map = linuxKeyMap_xt ## ESCAPE; \
  code = XT_KEY_ ## ESCAPE ## _ ## CODE; \
  escape = XT_MOD_ ## ESCAPE; \
  break;

  switch (key & SCR_KEY_CHAR_MASK) {
    KEY_TO_XT(SCR_KEY_ESCAPE, 00, Escape)
    KEY_TO_XT(SCR_KEY_F1, 00, F1)
    KEY_TO_XT(SCR_KEY_F2, 00, F2)
    KEY_TO_XT(SCR_KEY_F3, 00, F3)
    KEY_TO_XT(SCR_KEY_F4, 00, F4)
    KEY_TO_XT(SCR_KEY_F5, 00, F5)
    KEY_TO_XT(SCR_KEY_F6, 00, F6)
    KEY_TO_XT(SCR_KEY_F7, 00, F7)
    KEY_TO_XT(SCR_KEY_F8, 00, F8)
    KEY_TO_XT(SCR_KEY_F9, 00, F9)
    KEY_TO_XT(SCR_KEY_F10, 00, F10)
    KEY_TO_XT(SCR_KEY_F11, 00, F11)
    KEY_TO_XT(SCR_KEY_F12, 00, F12)

    KEY_TO_XT(SCR_KEY_F13, 00, F13)
    KEY_TO_XT(SCR_KEY_F14, 00, F14)
    KEY_TO_XT(SCR_KEY_F15, 00, F15)
    KEY_TO_XT(SCR_KEY_F16, 00, F16)
    KEY_TO_XT(SCR_KEY_F17, 00, F17)
    KEY_TO_XT(SCR_KEY_F18, 00, F18)
    KEY_TO_XT(SCR_KEY_F19, 00, F19)
    KEY_TO_XT(SCR_KEY_F20, 00, F20)
    KEY_TO_XT(SCR_KEY_F21, 00, F21)
    KEY_TO_XT(SCR_KEY_F22, 00, F22)
    KEY_TO_XT(SCR_KEY_F23, 00, F23)
    KEY_TO_XT(SCR_KEY_F24, 00, F24)

    KEY_TO_XT('`', 00, Grave)
    KEY_TO_XT('1', 00, 1)
    KEY_TO_XT('2', 00, 2)
    KEY_TO_XT('3', 00, 3)
    KEY_TO_XT('4', 00, 4)
    KEY_TO_XT('5', 00, 5)
    KEY_TO_XT('6', 00, 6)
    KEY_TO_XT('7', 00, 7)
    KEY_TO_XT('8', 00, 8)
    KEY_TO_XT('9', 00, 9)
    KEY_TO_XT('0', 00, 0)
    KEY_TO_XT('-', 00, Minus)
    KEY_TO_XT('=', 00, Equal)
    KEY_TO_XT(SCR_KEY_BACKSPACE, 00, Backspace)

    KEY_TO_XT(SCR_KEY_TAB, 00, Tab)
    KEY_TO_XT('q', 00, Q)
    KEY_TO_XT('w', 00, W)
    KEY_TO_XT('e', 00, E)
    KEY_TO_XT('r', 00, R)
    KEY_TO_XT('t', 00, T)
    KEY_TO_XT('y', 00, Y)
    KEY_TO_XT('u', 00, U)
    KEY_TO_XT('i', 00, I)
    KEY_TO_XT('o', 00, O)
    KEY_TO_XT('p', 00, P)
    KEY_TO_XT('[', 00, LeftBracket)
    KEY_TO_XT(']', 00, RightBracket)
    KEY_TO_XT('\\', 00, Backslash)

    KEY_TO_XT('a', 00, A)
    KEY_TO_XT('s', 00, S)
    KEY_TO_XT('d', 00, D)
    KEY_TO_XT('f', 00, F)
    KEY_TO_XT('g', 00, G)
    KEY_TO_XT('h', 00, H)
    KEY_TO_XT('j', 00, J)
    KEY_TO_XT('k', 00, K)
    KEY_TO_XT('l', 00, L)
    KEY_TO_XT(';', 00, Semicolon)
    KEY_TO_XT('\'', 00, Apostrophe)
    KEY_TO_XT(SCR_KEY_ENTER, 00, Enter)

    KEY_TO_XT('z', 00, Z)
    KEY_TO_XT('x', 00, X)
    KEY_TO_XT('c', 00, C)
    KEY_TO_XT('v', 00, V)
    KEY_TO_XT('b', 00, B)
    KEY_TO_XT('n', 00, N)
    KEY_TO_XT('m', 00, M)
    KEY_TO_XT(',', 00, Comma)
    KEY_TO_XT('.', 00, Period)
    KEY_TO_XT('/', 00, Slash)

    KEY_TO_XT(' ', 00, Space)

    KEY_TO_XT(SCR_KEY_INSERT, E0, Insert)
    KEY_TO_XT(SCR_KEY_DELETE, E0, Delete)
    KEY_TO_XT(SCR_KEY_HOME, E0, Home)
    KEY_TO_XT(SCR_KEY_END, E0, End)
    KEY_TO_XT(SCR_KEY_PAGE_UP, E0, PageUp)
    KEY_TO_XT(SCR_KEY_PAGE_DOWN, E0, PageDown)

    KEY_TO_XT(SCR_KEY_CURSOR_UP, E0, ArrowUp)
    KEY_TO_XT(SCR_KEY_CURSOR_LEFT, E0, ArrowLeft)
    KEY_TO_XT(SCR_KEY_CURSOR_DOWN, E0, ArrowDown)
    KEY_TO_XT(SCR_KEY_CURSOR_RIGHT, E0, ArrowRight)

    default:
      logMessage(LOG_WARNING, "key not supported in raw keyboard mode: %04X", key);
      return 0;
  }
#undef KEY_TO_XT

  {
    const int modUpper = hasModUpper(key);
    const int modShift = hasModShift(key);
    const int modControl = hasModControl(key);
    const int modAltLeft = hasModAltLeft(key);
    const int modAltRight = hasModAltRight(key);

    if (raw) {
      char codes[18];
      unsigned int count = 0;

      if (modUpper) {
        codes[count++] = XT_KEY_00_CapsLock;
        codes[count++] = XT_KEY_00_CapsLock | XT_BIT_RELEASE;
      }

      if (modShift) codes[count++] = XT_KEY_00_LeftShift;
      if (modControl) codes[count++] = XT_KEY_00_LeftControl;
      if (modAltLeft) codes[count++] = XT_KEY_00_LeftAlt;

      if (modAltRight) {
        codes[count++] = XT_MOD_E0;
        codes[count++] = XT_KEY_E0_RightAlt;
      }

      if (escape) codes[count++] = escape;
      codes[count++] = code;

      if (escape) codes[count++] = escape;
      codes[count++] = code | XT_BIT_RELEASE;

      if (modAltRight) {
        codes[count++] = XT_MOD_E0;
        codes[count++] = XT_KEY_E0_RightAlt | XT_BIT_RELEASE;
      }

      if (modAltLeft) codes[count++] = XT_KEY_00_LeftAlt | XT_BIT_RELEASE;
      if (modControl) codes[count++] = XT_KEY_00_LeftControl | XT_BIT_RELEASE;
      if (modShift) codes[count++] = XT_KEY_00_LeftShift | XT_BIT_RELEASE;

      if (modUpper) {
        codes[count++] = XT_KEY_00_CapsLock;
        codes[count++] = XT_KEY_00_CapsLock | XT_BIT_RELEASE;
      }

      return insertBytes(codes, count);
    } else {
      LinuxKeyCode mapped = map[code];

      if (!mapped) {
        logMessage(LOG_WARNING, "key not supported in medium raw keyboard mode: %04X", key);
        return 0;
      }

      return insertUinput(mapped, modUpper, modShift, modControl, modAltLeft, modAltRight);
    }
  }
}

static int
insertTranslated (ScreenKey key, int (*insertCharacter)(wchar_t character)) {
  wchar_t buffer[2];
  const wchar_t *sequence;
  const wchar_t *end;

  setScreenKeyModifiers(&key, 0);

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

      case SCR_KEY_F1:
        sequence = WS_C("\x1b[[A");
        break;

      case SCR_KEY_F2:
        sequence = WS_C("\x1b[[B");
        break;

      case SCR_KEY_F3:
        sequence = WS_C("\x1b[[C");
        break;

      case SCR_KEY_F4:
        sequence = WS_C("\x1b[[D");
        break;

      case SCR_KEY_F5:
        sequence = WS_C("\x1b[[E");
        break;

      case SCR_KEY_F6:
        sequence = WS_C("\x1b[17~");
        break;

      case SCR_KEY_F7:
        sequence = WS_C("\x1b[18~");
        break;

      case SCR_KEY_F8:
        sequence = WS_C("\x1b[19~");
        break;

      case SCR_KEY_F9:
        sequence = WS_C("\x1b[20~");
        break;

      case SCR_KEY_F10:
        sequence = WS_C("\x1b[21~");
        break;

      case SCR_KEY_F11:
        sequence = WS_C("\x1b[23~");
        break;

      case SCR_KEY_F12:
        sequence = WS_C("\x1b[24~");
        break;

      case SCR_KEY_F13:
        sequence = WS_C("\x1b[25~");
        break;

      case SCR_KEY_F14:
        sequence = WS_C("\x1b[26~");
        break;

      case SCR_KEY_F15:
        sequence = WS_C("\x1b[28~");
        break;

      case SCR_KEY_F16:
        sequence = WS_C("\x1b[29~");
        break;

      case SCR_KEY_F17:
        sequence = WS_C("\x1b[31~");
        break;

      case SCR_KEY_F18:
        sequence = WS_C("\x1b[32~");
        break;

      case SCR_KEY_F19:
        sequence = WS_C("\x1b[33~");
        break;

      case SCR_KEY_F20:
        sequence = WS_C("\x1b[34~");
        break;

      default:
        if (insertCode(key, 0)) return 1;
        logMessage(LOG_WARNING, "key not supported in xlate keyboard mode: %04X", key);
        return 0;
    }

    end = sequence + wcslen(sequence);
  } else {
    wchar_t *character = buffer + ARRAY_COUNT(buffer);

    end = character;
    *--character = key & SCR_KEY_CHAR_MASK;

    if (hasModAltLeft(key)) {
      int meta;

      if (controlConsole(KDGKBMETA, &meta) == -1) return 0;

      switch (meta) {
        case K_ESCPREFIX:
          *--character = ESC;
          break;

        case K_METABIT:
          if (*character < 0X80) {
            *character |= 0X80;
            break;
          }

        default:
          logMessage(LOG_WARNING, "unsupported keyboard meta mode: %d", meta);
          return 0;
      }
    }

    sequence = character;
  }

  while (sequence != end) {
    if (!insertCharacter(*sequence)) return 0;
    sequence += 1;
  }

  return 1;
}

static int
insertKey_LinuxScreen (ScreenKey key) {
  int ok = 0;

  logMessage(LOG_DEBUG, "insert key: %4.4X", key);

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

#ifdef K_OFF
        case K_OFF:
          ok = 1;
          break;
#endif /* K_OFF */

        default:
          logMessage(LOG_WARNING, "unsupported keyboard mode: %d", mode);
          break;
      }
    } else {
      logSystemError("ioctl[KDGKBMODE]");
    }
  }

  return ok;
}

typedef struct {
  char subcode;
  struct tiocl_selection selection;
} PACKED CharacterSelectionArgument;

static int
selectCharacters (CharacterSelectionArgument *argument) {
  if (controlConsole(TIOCLINUX, argument) != -1) return 1;
  if (errno != EINVAL) logSystemError("ioctl[TIOCLINUX]");
  return 0;
}

static int
highlightRegion_LinuxScreen (int left, int right, int top, int bottom) {
  CharacterSelectionArgument argument = {
    .subcode = TIOCL_SETSEL,

    .selection = {
      .xs = left + 1,
      .ys = top + 1,
      .xe = right + 1,
      .ye = bottom + 1,
      .sel_mode = TIOCL_SELCHAR
    }
  };

  return selectCharacters(&argument);
}

static int
unhighlightRegion_LinuxScreen (void) {
  CharacterSelectionArgument argument = {
    .subcode = TIOCL_SETSEL,

    .selection = {
      .xs = 0,
      .ys = 0,
      .xe = 0,
      .ye = 0,
      .sel_mode = TIOCL_SELCLEAR
    }
  };

  return selectCharacters(&argument);
}

static int
validateVt (int vt) {
  if ((vt >= 1) && (vt <= 0X3F)) return 1;
  logMessage(LOG_DEBUG, "virtual terminal out of range: %d", vt);
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
        logMessage(LOG_DEBUG, "switched to virtual tertminal %d.", vt);
        return 1;
      } else {
        logSystemError("ioctl[VT_ACTIVATE]");
      }
    }
  }
  return 0;
}

static int
currentVirtualTerminal_LinuxScreen (void) {
  return currentConsoleNumber;
}

static int
handleCommand_LinuxScreen (int command) {
  int blk = command & BRL_MSK_BLK;
  int arg UNUSED = command & BRL_MSK_ARG;
  int cmd = blk | arg;

  switch (cmd) {
    default:
#ifdef HAVE_LINUX_INPUT_H
      switch (blk) {
        case BRL_CMD_BLK(PASSXT):
          {
            int handled = 0;

            if (command & BRL_FLG_KBD_EMUL0) {
              xtKeys = linuxKeyMap_xtE0;
            } else if (arg == XT_MOD_E0) {
              xtKeys = linuxKeyMap_xtE0;
              handled = 1;
	    } else if (command & BRL_FLG_KBD_EMUL1) {
              xtKeys = linuxKeyMap_xtE1;
            } else if (arg == XT_MOD_E1) {
              xtKeys = linuxKeyMap_xtE1;
              handled = 1;
            }

            if (handled) return 1;
          }

	  {
            LinuxKeyCode key = xtKeys[arg & ~XT_BIT_RELEASE];
            int press = !(arg & XT_BIT_RELEASE);

            xtKeys = linuxKeyMap_xt00;

            if (key) return injectKeyEvent(key, press);
	  }
          break;

	case BRL_CMD_BLK(PASSAT):
          {
            int handled = 0;

            if (command & BRL_FLG_KBD_RELEASE) {
              atKeyPressed = 0;
            } else if (arg == AT_MOD_RELEASE) {
              atKeyPressed = 0;
              handled = 1;
            }

            if (command & BRL_FLG_KBD_EMUL0) {
              atKeys = linuxKeyMap_atE0;
            } else if (arg == AT_MOD_E0) {
              atKeys = linuxKeyMap_atE0;
              handled = 1;
	    } else if (command & BRL_FLG_KBD_EMUL1) {
              atKeys = linuxKeyMap_atE1;
            } else if (arg == AT_MOD_E1) {
              atKeys = linuxKeyMap_atE1;
              handled = 1;
            }

            if (handled) return 1;
          }

          {
            LinuxKeyCode key = atKeys[arg];
            int press = atKeyPressed;

            atKeys = linuxKeyMap_at00;
            atKeyPressed = 1;

            if (key) return injectKeyEvent(key, press);
          }
          break;

	case BRL_CMD_BLK(PASSPS2):
          {
            int handled = 0;

            if (command & BRL_FLG_KBD_RELEASE) {
              ps2KeyPressed = 0;
            } else if (arg == PS2_MOD_RELEASE) {
              ps2KeyPressed = 0;
              handled = 1;
            }

            if (handled) return 1;
          }

          {
            LinuxKeyCode key = linuxKeyMap_ps2[arg];
            int press = ps2KeyPressed;

            ps2KeyPressed = 1;

            if (key) return injectKeyEvent(key, press);
          }
          break;

        default:
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

  main->base.poll = poll_LinuxScreen;
  main->base.refresh = refresh_LinuxScreen;
  main->base.describe = describe_LinuxScreen;
  main->base.readCharacters = readCharacters_LinuxScreen;
  main->base.insertKey = insertKey_LinuxScreen;
  main->base.highlightRegion = highlightRegion_LinuxScreen;
  main->base.unhighlightRegion = unhighlightRegion_LinuxScreen;
  main->base.selectVirtualTerminal = selectVirtualTerminal_LinuxScreen;
  main->base.switchVirtualTerminal = switchVirtualTerminal_LinuxScreen;
  main->base.currentVirtualTerminal = currentVirtualTerminal_LinuxScreen;
  main->base.handleCommand = handleCommand_LinuxScreen;

  main->processParameters = processParameters_LinuxScreen;
  main->releaseParameters = releaseParameters_LinuxScreen;
  main->construct = construct_LinuxScreen;
  main->destruct = destruct_LinuxScreen;
  main->userVirtualTerminal = userVirtualTerminal_LinuxScreen;
}

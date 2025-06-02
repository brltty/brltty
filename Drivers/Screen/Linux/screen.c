/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <linux/tty.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/tiocl.h>
#include <linux/major.h>

#ifndef VT_GETHIFONTMASK
#define VT_GETHIFONTMASK 0X560D
#endif /* VT_GETHIFONTMASK */

#ifndef VT_GETCONSIZECSRPOS
#define VT_GETCONSIZECSRPOS    _IOR('V', 0x10, struct vt_consizecsrpos)

struct vt_consizecsrpos {
  __u16 con_rows;
  __u16 con_cols;
  __u16 csr_row;
  __u16 csr_col;
};
#endif /* VT_GETCONSIZECSRPOS */

#ifndef TIOCL_GETFGCONSOLE
#define TIOCL_GETFGCONSOLE 16
#endif /* TIOCL_GETFGCONSOLE */

#ifndef TIOCL_GETBRACKETEDPASTE
#define TIOCL_GETBRACKETEDPASTE 18
#endif /* TIOCL_GETBRACKETEDPASTE */

#include "log.h"
#include "report.h"
#include "message.h"
#include "async_handle.h"
#include "async_io.h"
#include "device.h"
#include "io_misc.h"
#include "timing.h"
#include "parse.h"
#include "brl_cmds.h"
#include "kbd_keycodes.h"
#include "ascii.h"
#include "unicode.h"
#include "charset.h"
#include "scr_gpm.h"
#include "system_linux.h"

typedef enum {
  PARM_CHARSET,
  PARM_FALLBACK_TEXT,
  PARM_HIGH_FONT_BIT,
  PARM_LARGE_SCREEN_BUG,
  PARM_LOG_SCREEN_FONT_MAP,
  PARM_RPI_SPACES_BUG,
  PARM_UNICODE,
  PARM_VIRTUAL_TERMINAL_NUMBER,
  PARM_WIDECHAR_PADDING,
} ScreenParameters;
#define SCRPARMS "charset", "fallbacktext", "hfb", "largescreenbug", "logsfm", "rpispacesbug", "unicode", "vt", "widecharpadding"

#include "scr_driver.h"
#include "screen.h"

static void
logKernelLimitation (unsigned char *alreadyLogged, int major, int minor, const char *message) {
  if (alreadyLogged) {
    if (*alreadyLogged) return;
    *alreadyLogged = 1;
  }

  logMessage(LOG_WARNING,
    "pre-%d.%d kernel limitation: %s",
    major, minor, message
  );
}

static const char *problemText;
static const char *fallbackText;

static unsigned int largeScreenBug;
static unsigned int logScreenFontMap;
static unsigned int rpiSpacesBug;
static unsigned int unicodeEnabled;
static int selectedVirtualTerminal;
static unsigned int widecharPadding;

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

static void
logUnexpectedSize (size_t expected, size_t actual, const char *type) {
  logMessage(LOG_WARNING,
    "%s %s read: expected %"PRIsize " bytes but read %"PRIsize,
    (actual < expected)? "short": "long",
    type, expected, actual
  );
}

static void
logTruncatedData (size_t expected, size_t actual, const char *type) {
  logMessage(LOG_ERR,
    "truncated %s data: expected %"PRIsize " bytes but only read %"PRIsize,
    type, expected, actual
  );
}

static int
extendCacheBuffer (void **buffer, size_t *size, size_t newSize, const char *type) {
  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
    "extending %s buffer: %"PRIsize " -> %"PRIsize,
    type, *size, newSize
  );

  void *newBuffer = malloc(newSize);

  if (!newBuffer) {
    logMallocError();
    return 0;
  }

  if (*buffer) free(*buffer);
  *buffer = newBuffer;
  *size = newSize;

  return 1;
}

static size_t
readCache (
  off_t offset, void *buffer, size_t size,
  const unsigned char *cache, size_t used, const char *type
) {
  if (offset > used) {
    logMessage(LOG_ERR,
      "invalid %s cache offset: %"PRIsize " > %"PRIsize,
      type, offset, used
    );
  } else {
    size_t left = used - offset;
    if (size > left) size = left;

    memcpy(buffer, &cache[offset], size);
    return size;
  }

  return 0;
}

static int
setDeviceName (const char **name, const char *const *names, int strict, const char *type) {
  return (*name = resolveDeviceName(names, strict, type)) != NULL;
}

static char *
vtName (const char *name, int vt) {
  char *string;

  if (vt) {
    int length = strlen(name);
    if (name[length-1] == '0') length -= 1;

    char buffer[length+4];
    snprintf(buffer, sizeof(buffer), "%.*s%d", length, name, vt);

    string = strdup(buffer);
  } else {
    string = strdup(name);
  }

  if (!string) logMallocError();
  return string;
}

static const char *consoleName = NULL;

static int
setConsoleName (void) {
  static const char *const names[] = {"tty0", "vc/0", NULL};
  return setDeviceName(&consoleName, names, 0, "console");
}

static void
closeConsole (int *fd, const char *type) {
  if (*fd != -1) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
      "closing %s console: fd=%d",
      type, *fd
    );

    if (close(*fd) == -1) logSystemError("close[console]");
    *fd = -1;
  }
}

static int
openConsole (int *fd, int vt, const char *type) {
  int opened = 0;
  char *name = vtName(consoleName, vt);

  if (name) {
    int console = openCharacterDevice(name, O_WRONLY|O_NOCTTY, TTY_MAJOR, vt);

    if (console != -1) {
      logMessage(LOG_CATEGORY(SCREEN_DRIVER),
        "%s console opened: %s: fd=%d",
        type, name, console
      );

      closeConsole(fd, type);
      *fd = console;
      opened = 1;
    }

    free(name);
  }

  return opened;
}

static int
controlConsole (int *fd, int vt, const char *type, int operation, void *argument) {
  int result = ioctl(*fd, operation, argument);

  if (result == -1) {
    if (errno == EIO) {
      logMessage(LOG_ERR,
                 "console control error %d: fd=%d vt=%d op=0X%04X: %s",
                 errno, *fd, vt, operation, strerror(errno));

      if (openConsole(fd, vt, type)) {
        result = ioctl(*fd, operation, argument);
      }
    }
  }

  return result;
}

static const char mainConsoleType[] = "main";
static int mainConsoleDescriptor;

static const int NO_CONSOLE = 0;
static const int MAIN_CONSOLE = 0;

static void
closeMainConsole (void) {
  closeConsole(&mainConsoleDescriptor, mainConsoleType);
}

static int
openMainConsole (void) {
  return openConsole(&mainConsoleDescriptor, MAIN_CONSOLE, mainConsoleType);
}

static int
controlMainConsole (int operation, void *argument) {
  return controlConsole(&mainConsoleDescriptor, MAIN_CONSOLE, mainConsoleType, operation, argument);
}

static int
getConsoleState (struct vt_stat *state) {
  if (controlMainConsole(VT_GETSTATE, state) != -1) return 1;
  logSystemError("ioctl[VT_GETSTATE]");
  problemText = gettext("can't get console state");
  return 0;
}

static int
getForegroundConsoleNumber (int *vt) {
  {
    unsigned char subcode = TIOCL_GETFGCONSOLE;
    int result = controlMainConsole(TIOCLINUX, &subcode);

    if (result != -1) {
      *vt = result + 1;
      return 1;
    } else {
      logSystemError("ioctl[TIOCLINUX(TIOCL_GETFGCONSOLE)]");
    }
  }

  {
    struct vt_stat state;

    if (getConsoleState(&state)) {
      *vt = state.v_active;
      return 1;
    }
  }

  problemText = gettext("can't get foreground console number");
  return 0;
}

static const char currentConsoleType[] = "current";
static int currentConsoleDescriptor;

static inline int
isCurrentConsoleOpen (void) {
  return currentConsoleDescriptor != -1;
}

static int
controlCurrentConsole (int operation, void *argument) {
  if (isCurrentConsoleOpen()) {
    return controlConsole(&currentConsoleDescriptor, selectedVirtualTerminal, currentConsoleType, operation, argument);
  }

  switch (operation) {
    case GIO_UNIMAP: {
      struct unimapdesc *sfm = argument;
      memset(sfm, 0, sizeof(*sfm));
      sfm->entries = NULL;
      sfm->entry_ct = 0;
      return 0;
    }

    case KDFONTOP: {
      struct console_font_op *cfo = argument;

      if (cfo->op == KD_FONT_OP_GET) {
        cfo->charcount = 0;
        cfo->width = 8;
        cfo->height = 16;
        return 0;
      }

      break;
    }

    case VT_GETHIFONTMASK: {
      unsigned short *mask = argument;
      *mask = 0;
      return 0;
    }

    case KDGETMODE: {
      int *mode = argument;
      *mode = KD_TEXT;
      return 0;
    }

    default:
      break;
  }

  logMessage(LOG_WARNING, "current console not open");
  errno = EAGAIN;
  return -1;
}

static void
closeCurrentConsole (void) {
  closeConsole(&currentConsoleDescriptor, currentConsoleType);
}

static int
openCurrentConsole (int *vt) {
  if (openConsole(&currentConsoleDescriptor, selectedVirtualTerminal, currentConsoleType)) {
    if (selectedVirtualTerminal) return 1;
    unsigned int device;

    if (controlCurrentConsole(TIOCGDEV, &device) != -1) {
      {
        const unsigned int expected = TTY_MAJOR;
        unsigned int actual = major(device);

        if (actual != expected) {
          logMessage(LOG_WARNING,
            "unexpected foreground tty device type (expected %u, got %u)",
            expected, actual
          );

          return 0;
        }
      }

      unsigned int ttyNumber = minor(device);
      const unsigned int ttyNumberMask = 0X3F;

      if ((ttyNumber & ttyNumberMask) != ttyNumber) {
        logMessage(LOG_WARNING,
          "unexpected foreground tty device group (expected %d, got %u)",
          *vt, ttyNumber
        );

        return 0;
      }

      if (ttyNumber != *vt) {
        logMessage(LOG_WARNING,
          "unexpected foreground tty number (expecting %d, got %u) - assuming VT switch",
          *vt, ttyNumber
        );

        *vt = ttyNumber;
      }

      return 1;
    } else {
      logSystemError("ioctl[TIOCGDEV]");
    }
  }

  return 0;
}

static const char unicodeDeviceType[] = "unicode";
static const char *unicodeDeviceName = NULL;
static int unicodeDescriptor;

static int
setUnicodeDeviceName (void) {
  static const char *const names[] = {"vcsu", "vcsu0", NULL};
  return setDeviceName(&unicodeDeviceName, names, 1, unicodeDeviceType);
}

static void
closeUnicodeDevice (int *fd) {
  if (*fd != -1) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER), "closing unicode: fd=%d", *fd);
    if (close(*fd) == -1) logSystemError("close[unicode]");
    *fd = -1;
  }
}

static int
openUnicodeDevice (int *fd, int vt) {
  if (!unicodeDeviceName) return 0;
  if (*fd != -1) return 1;

  int opened = 0;
  char *name = vtName(unicodeDeviceName, vt);

  if (name) {
    int unicode = openCharacterDevice(name, O_RDWR, VCS_MAJOR, 0X40|vt);

    if (unicode != -1) {
      logMessage(LOG_CATEGORY(SCREEN_DRIVER),
                 "unicode device opened: %s: fd=%d", name, unicode);

      *fd = unicode;
      opened = 1;
    } else {
      logMessage(LOG_ERR, "unicode device open error: %s: %s", name, strerror(errno));
      unicodeDeviceName = NULL;
    }

    free(name);
  }

  return opened;
}

static void
closeCurrentUnicodeDevice (void) {
  closeUnicodeDevice(&unicodeDescriptor);
}

static int
openCurrentUnicodeDevice (void) {
  if (!unicodeEnabled) return 0;
  return openUnicodeDevice(&unicodeDescriptor, selectedVirtualTerminal);
}

static size_t
readUnicodeDevice (off_t offset, void *buffer, size_t size) {
  if (openCurrentUnicodeDevice()) {
    const ssize_t count = pread(unicodeDescriptor, buffer, size, offset);

    if (count != -1) {
      if (rpiSpacesBug) {
        uint32_t *character = buffer;
        const uint32_t *end = character + (count / sizeof(*character));

        while (character < end) {
          if (*character == 0X20202020) {
            static unsigned char bugLogged = 0;

            if (!bugLogged) {
              logMessage(LOG_WARNING, "Linux screen driver: RPI spaces bug detected");
              bugLogged = 1;
            }

            *character = ' ';
          }

          character += 1;
        }
      }

      return count;
    }

    if (errno != ENODATA) logSystemError("unicode device read");
  }

  return 0;
}

static void *unicodeCacheBuffer;
static size_t unicodeCacheSize;
static size_t unicodeCacheUsed;

static size_t
readUnicodeCache (off_t offset, void *buffer, size_t size) {
  return readCache(offset, buffer, size, unicodeCacheBuffer, unicodeCacheUsed, unicodeDeviceType);
}

static int
readUnicodeData (off_t offset, void *buffer, size_t size) {
  size_t count = (unicodeCacheUsed? readUnicodeCache: readUnicodeDevice)(offset, buffer, size);
  if (count == size) return 1;

  logTruncatedData(size, count, unicodeDeviceType);
  return 0;
}

static int
readUnicodeContent (off_t offset, uint32_t *buffer, size_t count) {
  count *= sizeof(*buffer);
  offset *= sizeof(*buffer);
  return readUnicodeData(offset, buffer, count);
}

static int
refreshUnicodeCache (unsigned int characters) {
  const size_t expectedSize = characters * sizeof(uint32_t);

  if (expectedSize > unicodeCacheSize) {
    if (!extendCacheBuffer(&unicodeCacheBuffer, &unicodeCacheSize, expectedSize, unicodeDeviceType)) {
      return 0;
    }
  }

  unicodeCacheUsed = readUnicodeDevice(0, unicodeCacheBuffer, unicodeCacheSize);
  if (unicodeCacheUsed == expectedSize) return 1;

  logUnexpectedSize(expectedSize, unicodeCacheUsed, unicodeDeviceType);
  return 0;
}

static const char screenDeviceType[] = "screen";
static const char *screenName = NULL;
static int screenDescriptor;

static int isMonitorable;
static THREAD_LOCAL AsyncHandle screenMonitor = NULL;

static int screenUpdated;

static int currentConsoleNumber;
static int inTextMode;
static TimePeriod mappingRecalculationTimer;

typedef struct {
  unsigned short int columns;
  unsigned short int rows;
} ScreenSize;

typedef struct {
  unsigned short int column;
  unsigned short int row;
} ScreenLocation;

typedef struct {
  ScreenSize size;
  ScreenLocation cursor;
  unsigned char hideCursor:1;
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

static int
setScreenName (void) {
  static const char *const names[] = {"vcsa", "vcsa0", "vcc/a", NULL};
  return setDeviceName(&screenName, names, 0, screenDeviceType);
}

static int
openScreenDevice (int *fd, int vt) {
  int opened = 0;
  char *name = vtName(screenName, vt);

  if (name) {
    int screen = openCharacterDevice(name, O_RDWR, VCS_MAJOR, 0X80|vt);

    if (screen != -1) {
      logMessage(LOG_CATEGORY(SCREEN_DRIVER),
                 "screen opened: %s: fd=%d", name, screen);

      *fd = screen;
      opened = 1;
    } else {
      logMessage(LOG_ERR, "screen open error: %s: %s", name, strerror(errno));
    }

    free(name);
  }

  return opened;
}

static void
closeCurrentScreen (void) {
  if (screenMonitor) {
    asyncCancelRequest(screenMonitor);
    screenMonitor = NULL;
  }

  if (screenDescriptor != -1) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "closing screen: fd=%d", screenDescriptor);

    if (close(screenDescriptor) == -1) logSystemError("close[screen]");
    screenDescriptor = -1;
  }
}

static int
setCurrentScreen (int vt) {
  int screen;
  if (!openScreenDevice(&screen, vt)) return 0;

  closeCurrentConsole();
  closeCurrentUnicodeDevice();
  closeCurrentScreen();

  screenDescriptor = screen;
  selectedVirtualTerminal = vt;

  isMonitorable = canMonitorScreen();
  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
             "screen is monitorable: %s",
             (isMonitorable? "yes": "no"));

  screenMonitor = NULL;
  screenUpdated = 1;
  return 1;
}

static size_t
vcsaReadDevice (off_t offset, void *buffer, size_t size) {
  const ssize_t count = pread(screenDescriptor, buffer, size, offset);
  if (count != -1) return count;

  logSystemError("screen read");
  return 0;
}

static int
vcsaHasClamping (void) {
  int major, minor;
  if (!getKernelRelease(&major, &minor, NULL)) return 0;

  if (major < 5) return 0;
  if (major > 5) return 1;
  return minor >= 1;
}

typedef struct {
  struct {
    uint8_t rows;
    uint8_t columns;
  } size;

  struct {
    uint8_t column;
    uint8_t row;
  } cursor;
} VcsaHeader;

static int
vcsaReadHeader (ScreenHeader *header) {
  VcsaHeader vcsa;

  {
    size_t vcsaSize = sizeof(vcsa);
    const size_t count = vcsaReadDevice(0, &vcsa, vcsaSize);
    if (!count) return 0;

    if (count < vcsaSize) {
      logBytes(LOG_ERR,
        "truncated vcsa header: %"PRIsize " < %"PRIsize,
        &vcsa, count, count, vcsaSize
      );

      return 0;
    }
  }

  header->size.columns = vcsa.size.columns;
  header->size.rows = vcsa.size.rows;
  header->cursor.column = vcsa.cursor.column;
  header->cursor.row = vcsa.cursor.row;

  if (isCurrentConsoleOpen()) {
    if ((header->size.columns == UINT8_MAX) || (header->size.rows == UINT8_MAX) || largeScreenBug) {
      struct winsize winSize;

      if (controlCurrentConsole(TIOCGWINSZ, &winSize) != -1) {
        header->size.columns = winSize.ws_col;
        header->size.rows = winSize.ws_row;
      } else {
        logSystemError("ioctl[TIOCGWINSZ]");
      }
    }
  }

  if (largeScreenBug) {
    if (header->cursor.column >= header->size.columns) {
      header->cursor.column = header->size.columns - 1;
    }

    if (header->cursor.row >= header->size.rows) {
      header->cursor.row = header->size.rows - 1;
    }
  } else if ((header->cursor.column == UINT8_MAX) || (header->cursor.row == UINT8_MAX)) {
    header->hideCursor = 1;

    {
      static unsigned char alreadyLogged = 0;

      logKernelLimitation(
        &alreadyLogged, 6, 16,
        "screen cursor not rendered when beyond column and/or row 255"
      );
    }
  }

  return 1;
}

static size_t
readScreenDevice (off_t offset, void *buffer, size_t size) {
  size_t result = 0;
  const size_t headerSize = sizeof(ScreenHeader);

  if (offset < headerSize) {
    static unsigned char useGetConSizeCsrPos = 1;
    unsigned char useVcsa = 1;

    ScreenHeader header;
    memset(&header, 0, sizeof(header));

    if (useGetConSizeCsrPos && isCurrentConsoleOpen()) {
      struct vt_consizecsrpos info;

      if (controlCurrentConsole(VT_GETCONSIZECSRPOS, &info) != -1) {
        useVcsa = 0;
        header.size.columns = info.con_cols;
        header.size.rows = info.con_rows;
        header.cursor.column = info.csr_col;
        header.cursor.row = info.csr_row;
      } else {
        if (errno == ENOTTY) useGetConSizeCsrPos = 0;
        logSystemError("ioctl[VT_GETCONSIZECSRPOS]");
      }
    }

    if (useVcsa) {
      if (!vcsaReadHeader(&header)) {
        goto done;
      }
    }

    {
      const void *from = &header + offset;
      size_t count = headerSize - offset;
      if (size < count) count = size;

      buffer = mempcpy(buffer, from, count);
      result += count;

      if (!(size -= count)) goto done;
      offset = headerSize;
    }
  }

  offset -= headerSize;
  offset += sizeof(VcsaHeader);

  size_t count = vcsaReadDevice(offset, buffer, size);
  if (!count) goto done;
  result += count;

done:
  return result;
}

static void *screenCacheBuffer;
static size_t screenCacheSize;
static size_t screenCacheUsed;

static size_t
readScreenCache (off_t offset, void *buffer, size_t size) {
  return readCache(offset, buffer, size, screenCacheBuffer, screenCacheUsed, screenDeviceType);
}

static int
readScreenData (off_t offset, void *buffer, size_t size) {
  size_t count = (screenCacheUsed? readScreenCache: readScreenDevice)(offset, buffer, size);
  if (count == size) return 1;

  logTruncatedData(size, count, screenDeviceType);
  return 0;
}

static int
readScreenHeader (ScreenHeader *header) {
  return readScreenData(0, header, sizeof(*header));
}

static int
readScreenContent (off_t offset, uint16_t *buffer, size_t count) {
  count *= sizeof(*buffer);
  offset *= sizeof(*buffer);
  offset += sizeof(ScreenHeader);
  return readScreenData(offset, buffer, count);
}

static inline size_t
toScreenBufferSize (const ScreenSize *screenSize) {
  return (screenSize->columns * screenSize->rows * 2) + sizeof(ScreenHeader);
}

static void
logTruncatedScreenHeader (const ScreenHeader *header, size_t count) {
  logBytes(LOG_ERR,
    "truncated screen header: %"PRIsize " < %"PRIsize,
    header, count, count, sizeof(*header)
  );
}

static int
refreshScreenCache (unsigned int *characters) {
  if (!screenCacheBuffer) {
    ScreenHeader header;
    const size_t headerSize = sizeof(header);

    {
      size_t bytesRead = readScreenDevice(0, &header, headerSize);
      if (!bytesRead) return 0;

      if (bytesRead < headerSize) {
        logTruncatedScreenHeader(&header, bytesRead);
        return 0;
      }
    }

    {
      const size_t size = toScreenBufferSize(&header.size);

      logMessage(LOG_CATEGORY(SCREEN_DRIVER),
        "allocating screen buffer: %"PRIsize,
        size
      );

      void *buffer = malloc(size);

      if (!buffer) {
        logMallocError();
        return 0;
      }

      screenCacheBuffer = buffer;
      screenCacheSize = size;
    }
  }

  unsigned int unexpectedSizeCounter = 10;
  while (1) {
    screenCacheUsed = readScreenDevice(0, screenCacheBuffer, screenCacheSize);
    if (!screenCacheUsed) return 0;

    ScreenHeader *header = screenCacheBuffer;
    const size_t headerSize = sizeof(*header);

    if (screenCacheUsed < headerSize) {
      logTruncatedScreenHeader(header, screenCacheUsed);
      return 0;
    }

    const size_t expectedSize = toScreenBufferSize(&header->size);

    if (screenCacheUsed == expectedSize) {
      *characters = header->size.columns * header->size.rows;
      return 1;
    }

    if (expectedSize > screenCacheSize) {
      if (!extendCacheBuffer(&screenCacheBuffer, &screenCacheSize, expectedSize, screenDeviceType)) {
        return 0;
      }
    } else {
      logUnexpectedSize(expectedSize, screenCacheUsed, screenDeviceType);

      if (!--unexpectedSizeCounter) {
        logMessage(LOG_WARNING, "too many unexpected size screen reads");
        return 0;
      }
    }
  }
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

    if (controlCurrentConsole(GIO_UNIMAP, &sfm) != -1) break;
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
        free(sfm.entries);
        return 0;
      }
    }
  }

  if (screenFontMapTable) free(screenFontMapTable);
  screenFontMapTable = sfm.entries;
  screenFontMapCount = sfm.entry_ct;
  screenFontMapSize = size;
  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
             "Font Map Size: %d", screenFontMapCount);

  if (logScreenFontMap) {
    for (unsigned int i=0; i<screenFontMapCount; i+=1) {
      const struct unipair *map = &screenFontMapTable[i];

      logMessage(LOG_CATEGORY(SCREEN_DRIVER),
                 "SFM[%03u]: U+%04X@%04X",
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
    struct console_font_op cfo = {
      .width = UINT_MAX,
      .height = UINT_MAX,
      .op = KD_FONT_OP_GET
    };

    vgaCharacterCount = 0;
    {
      static unsigned char isNotImplemented = 0;

      if (!isNotImplemented) {
        if (controlCurrentConsole(KDFONTOP, &cfo) != -1) {
          logMessage(LOG_CATEGORY(SCREEN_DRIVER),
                     "Font Properties: %ux%u*%u",
                     cfo.width, cfo.height, cfo.charcount);
          vgaCharacterCount = cfo.charcount;
        } else {
          int logLevel = LOG_WARNING;

          if (errno == ENOSYS) {
            isNotImplemented = 1;
            logLevel = LOG_CATEGORY(SCREEN_DRIVER);
          }

          if (errno != EINVAL) {
            logMessage(logLevel, "ioctl[KDFONTOP(GET)]: %s", strerror(errno));
          }
        }
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

  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
             "VGA Character Count: %d(%s)",
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
  unshiftedAttributesMask = bit - 1;
  shiftedAttributesMask = ~unshiftedAttributesMask & ~bit;
  unshiftedAttributesMask &= 0XFF00;
  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
             "Attributes Masks: Font:%04X Unshifted:%04X Shifted:%04X",
             fontAttributesMask, unshiftedAttributesMask, shiftedAttributesMask);
}

static int
determineAttributesMasks (void) {
  if (!vgaLargeTable) {
    setAttributesMasks(0);
  } else if (highFontBit) {
    setAttributesMasks(highFontBit);
  } else {
    {
      unsigned short mask;

      if (controlCurrentConsole(VT_GETHIFONTMASK, &mask) == -1) {
        if (errno != EINVAL) logSystemError("ioctl[VT_GETHIFONTMASK]");
      } else if (mask & 0XFF) {
        logMessage(LOG_ERR, "high font mask has bit set in low-order byte: %04X", mask);
      } else {
        setAttributesMasks(mask);
        return 1;
      }
    }

    {
      ScreenHeader header;

      if (readScreenHeader(&header)) {
        const ScreenSize *size = &header.size;

        const size_t count = size->columns * size->rows;
        unsigned short buffer[count];

        if (readScreenContent(0, buffer, ARRAY_COUNT(buffer))) {
          unsigned int counts[0X10];
          memset(counts, 0, sizeof(counts));

          for (unsigned int index=0; index<count; index+=1) {
            counts[(buffer[index] & 0X0F00) >> 8] += 1;
          }

          setAttributesMasks((counts[0XE] > counts[0X7])? 0X0100: 0X0800);
          return 1;
        }
      }
    }
  }

  return 0;
}

static wchar_t translationTable[0X200];

static int
setTranslationTable (int force) {
  int mappingChanged = 0;
  int sfmChanged = setScreenFontMap(force);
  int vccChanged = (sfmChanged || force)? setVgaCharacterCount(force): 0;

  if (vccChanged || force) determineAttributesMasks();

  if (sfmChanged || vccChanged) {
    unsigned int count = ARRAY_COUNT(translationTable);

    for (unsigned int i=0; i<count; i+=1) {
      translationTable[i] = UNICODE_ROW_DIRECT | i;
    }

    {
      unsigned int screenFontMapIndex = screenFontMapCount;

      while (screenFontMapIndex > 0) {
        const struct unipair *sfm = &screenFontMapTable[--screenFontMapIndex];

        if (sfm->fontpos < count) {
          wchar_t *character = &translationTable[sfm->fontpos];
          if (*character == 0X20) continue;
          *character = sfm->unicode;
        }
      }
    }

    mappingChanged = 1;
  }

  if (mappingChanged) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER), "character mapping changed");
  }

  restartTimePeriod(&mappingRecalculationTimer);
  return mappingChanged;
}

static int
readScreenRow (int row, size_t size, ScreenCharacter *characters, int *offsets) {
  off_t offset = row * size;

  uint16_t vgaBuffer[size];
  if (!readScreenContent(offset, vgaBuffer, size)) return 0;

  uint32_t unicodeBuffer[size];
  const uint32_t *unicode = NULL;

  if (unicodeEnabled) {
    if (readUnicodeContent(offset, unicodeBuffer, size)) {
      unicode = unicodeBuffer;
    }
  }

  ScreenCharacter *character = characters;
  int column = 0;

  {
    const uint16_t *vga = vgaBuffer;
    const uint16_t *end = vga + size;
    int blanks = 0;

    while (vga < end) {
      wint_t wc;

      if (unicode) {
        wc = *unicode++;

        if ((blanks > 0) && iswspace(wc)) {
          blanks -= 1;
          wc = WEOF;
        } else if (widecharPadding) {
          blanks = 0;
        } else {
          blanks = getCharacterWidth(wc) - 1;
        }
      } else {
        uint16_t position = *vga & 0XFF;
        if (*vga & fontAttributesMask) position |= 0X100;
        wc = convertCharacter(&translationTable[position]);
      }

      if (wc != WEOF) {
        if (character) {
          character->attributes = ((*vga & unshiftedAttributesMask) |
                                   ((*vga & shiftedAttributesMask) >> 1)) >> 8;

          character->text = wc;
          character += 1;
        }

        if (offsets) offsets[column] = vga - vgaBuffer;
        column += 1;
      }

      vga += 1;
    }
  }

  if (!unicode) {
    wint_t wc;

    while ((wc = convertCharacter(NULL)) != WEOF) {
      if (column < size) {
        if (character) {
          character->text = wc;
          character->attributes = SCR_COLOUR_DEFAULT;
          character += 1;
        }

        if (offsets) offsets[column] = size - 1;
        column += 1;
      }
    }
  }

  while (column < size) {
    if (character) {
      static const ScreenCharacter pad = {
        .text = WC_C(' '),
        .attributes = SCR_COLOUR_DEFAULT
      };

      *character++ = pad;
    }

    if (offsets) offsets[column] = size - 1;
    column += 1;
  }

  return 1;
}

static void
adjustCursorColumn (short *column, short row, short columns) {
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

#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>

static const LinuxKeyCode *xtKeys;
static const LinuxKeyCode *atKeys;
static int atKeyPressed;
static int ps2KeyPressed;
#endif /* HAVE_LINUX_INPUT_H */

static UinputObject *uinputKeyboard = NULL;
static ReportListenerInstance *brailleDeviceOfflineListener;

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

static int
processParameters_LinuxScreen (char **parameters) {
  fallbackText = parameters[PARM_FALLBACK_TEXT];

  {
    const char *names = parameters[PARM_CHARSET];

    if (!names || !*names) names = getLocaleCharset();
    if (!allocateCharsetEntries(names)) return 0;
  }

  highFontBit = 0;
  {
    const char *parameter = parameters[PARM_HIGH_FONT_BIT];

    if (parameter && *parameter) {
      int bit = 0;

      static const int minimum = 0;
      static const int maximum = 7;

      static const char *choices[] = {"auto", "vga", "fb", NULL};
      unsigned int choice;

      if (validateInteger(&bit, parameter, &minimum, &maximum)) {
        highFontBit = 1 << (bit + 8);
      } else if (!validateChoice(&choice, parameter, choices)) {
        logMessage(LOG_WARNING, "%s: %s", "invalid high font bit", parameter);
      } else if (choice) {
        static const unsigned short bits[] = {0X0800, 0X0100};
        highFontBit = bits[choice-1];
      }
    }
  }

  largeScreenBug = !vcsaHasClamping();
  {
    const char *parameter = parameters[PARM_LARGE_SCREEN_BUG];

    if (parameter && *parameter) {
      if (!validateYesNo(&largeScreenBug, parameter)) {
        logMessage(LOG_WARNING, "%s: %s", "invalid large screen bug setting", parameter);
      }
    }
  }

  logScreenFontMap = 0;
  {
    const char *parameter = parameters[PARM_LOG_SCREEN_FONT_MAP];

    if (parameter && *parameter) {
      if (!validateYesNo(&logScreenFontMap, parameter)) {
        logMessage(LOG_WARNING, "%s: %s", "invalid log screen font map setting", parameter);
      }
    }
  }

  rpiSpacesBug = 0;
  {
    const char *parameter = parameters[PARM_RPI_SPACES_BUG];

    if (parameter && *parameter) {
      if (!validateYesNo(&rpiSpacesBug, parameter)) {
        logMessage(LOG_WARNING, "%s: %s", "invalid RPI spaces bug setting", parameter);
      }
    }
  }

  unicodeEnabled = 1;
  {
    const char *parameter = parameters[PARM_UNICODE];

    if (parameter && *parameter) {
      if (!validateYesNo(&unicodeEnabled, parameter)) {
        logMessage(LOG_WARNING, "%s: %s", "invalid direct unicode setting", parameter);
      }
    }
  }

  selectedVirtualTerminal = 0;
  {
    const char *parameter = parameters[PARM_VIRTUAL_TERMINAL_NUMBER];

    if (parameter && *parameter) {
      static const int minimum = 0;
      static const int maximum = MAX_NR_CONSOLES;

      if (!validateInteger(&selectedVirtualTerminal, parameter, &minimum, &maximum)) {
        logMessage(LOG_WARNING, "%s: %s", "invalid virtual terminal number", parameter);
      }
    }
  }

  widecharPadding = 0;
  {
    const char *parameter = parameters[PARM_WIDECHAR_PADDING];

    if (parameter && *parameter) {
      if (!validateYesNo(&widecharPadding, parameter)) {
        logMessage(LOG_WARNING, "%s: %s", "invalid widechar padding setting", parameter);
      }
    }
  }

  return 1;
}

static void
releaseParameters_LinuxScreen (void) {
  deallocateCharsetEntries();
}

REPORT_LISTENER(lxBrailleDeviceOfflineListener) {
  resetKeyboard();
}

static int
construct_LinuxScreen (void) {
  mainConsoleDescriptor = -1;
  currentConsoleDescriptor = -1;
  screenDescriptor = -1;
  unicodeDescriptor = -1;

  screenUpdated = 0;
  screenCacheBuffer = NULL;
  screenCacheSize = 0;
  screenCacheUsed = 0;

  unicodeCacheBuffer = NULL;
  unicodeCacheSize = 0;
  unicodeCacheUsed = 0;

  currentConsoleNumber = 0;
  inTextMode = 1;
  startTimePeriod(&mappingRecalculationTimer, 4000);

  brailleDeviceOfflineListener = NULL;

#ifdef HAVE_LINUX_INPUT_H
  xtKeys = linuxKeyMap_xt00;
  atKeys = linuxKeyMap_at00;
  atKeyPressed = 1;
  ps2KeyPressed = 1;
#endif /* HAVE_LINUX_INPUT_H */

  if (setScreenName()) {
    if (setConsoleName()) {
      if (unicodeEnabled) {
        if (!setUnicodeDeviceName()) {
          unicodeEnabled = 0;
        }
      }

      if (openMainConsole()) {
        if (setCurrentScreen(selectedVirtualTerminal)) {
          openKeyboard();
          brailleDeviceOfflineListener = registerReportListener(REPORT_BRAILLE_DEVICE_OFFLINE, lxBrailleDeviceOfflineListener, NULL);
          return 1;
        }
      } else {
        logSystemError("main console open");
      }
    }
  }

  closeCurrentConsole();
  closeCurrentScreen();
  closeMainConsole();
  return 0;
}

static void
destruct_LinuxScreen (void) {
  if (brailleDeviceOfflineListener) {
    unregisterReportListener(brailleDeviceOfflineListener);
    brailleDeviceOfflineListener = NULL;
  }

  closeCurrentConsole();
  consoleName = NULL;

  closeCurrentScreen();
  screenName = NULL;

  if (screenFontMapTable) {
    free(screenFontMapTable);
    screenFontMapTable = NULL;
  }
  screenFontMapSize = 0;
  screenFontMapCount = 0;

  if (screenCacheBuffer) {
    free(screenCacheBuffer);
    screenCacheBuffer = NULL;
  }
  screenCacheSize = 0;
  screenCacheUsed = 0;

  if (unicodeCacheBuffer) {
    free(unicodeCacheBuffer);
    unicodeCacheBuffer = NULL;
  }
  unicodeCacheSize = 0;
  unicodeCacheUsed = 0;

  closeMainConsole();
}

ASYNC_MONITOR_CALLBACK(lxScreenUpdated) {
  asyncDiscardHandle(screenMonitor);
  screenMonitor = NULL;

  screenUpdated = 1;
  mainScreenUpdated();

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
isUnusedConsole (int vt) {
  int isUnused = 0;
  int fd;

  if (openScreenDevice(&fd, vt)) {
    const off_t start = sizeof(VcsaHeader);
    off_t offset = start;
    uint16_t firstCharacter;

    while (1) {
      uint16_t buffer[0X800];
      size_t bytesRead = pread(fd, buffer, sizeof(buffer), offset);

      if (!bytesRead) {
        isUnused = 1;
        goto done;
      }

      const uint16_t *from = buffer;
      const uint16_t *to = from + (bytesRead / sizeof(buffer[0]));
      if (offset == start) firstCharacter = *from++;

      while (from < to) {
        if (*from != firstCharacter) goto done;
        from += 1;
      }

      offset += bytesRead;
    }

  done:
    close(fd);
    fd = -1;
  } else {
    switch (errno) {
      default:
        logMessage(LOG_WARNING, "can't open screen %d: %s", vt, strerror(errno));
        break;

      case ENOENT: // device name not defined
      case EPERM: // can't define device within nodev directory
      case ENXIO: // device not defined within kernel
        isUnused = 1;
        break;
    }
  }

  return isUnused;
}

static int
canOpenConsole (int vt) {
  static uint64_t openableConsoles = 0;
  uint64_t bit = UINT64_C(1) << vt;

  if (bit && !(openableConsoles & bit)) {
    if (vt != MAIN_CONSOLE) {
      struct vt_stat state;
      if (!getConsoleState(&state)) return 0;

      if (!(state.v_state & bit)) {
        if (isUnusedConsole(vt)) {
          return 0;
        }
      }
    }

    openableConsoles |= bit;
  }

  return 1;
}

static int
getConsoleNumber (void) {
  int vt;

  if (selectedVirtualTerminal) {
    vt = selectedVirtualTerminal;
  } else if (!getForegroundConsoleNumber(&vt)) {
    vt = NO_CONSOLE;
  }

  if (vt != currentConsoleNumber) {
    closeCurrentConsole();
  }

  if (vt != NO_CONSOLE) {
    if (!isCurrentConsoleOpen()) {
      if (!canOpenConsole(vt)) {
        problemText = gettext("console not in use");
      } else if (!openCurrentConsole(&vt)) {
        logSystemError("current console open");
        problemText = gettext("can't open console");
      }

      setTranslationTable(1);
    }
  }

  return vt;
}

static int
testTextMode (void) {
  if (problemText) return 0;
  int mode;

  if (controlCurrentConsole(KDGETMODE, &mode) == -1) {
    logSystemError("ioctl[KDGETMODE]");
  } else if (mode == KD_TEXT) {
    if (afterTimePeriod(&mappingRecalculationTimer, NULL)) setTranslationTable(0);
    return 1;
  }

  problemText = gettext("screen not in text mode");
  return 0;
}

static int
refreshCache (void) {
  unsigned int unicodeRefreshCounter = 10;

  while (1) {
    unsigned int characters;

    if (refreshScreenCache(&characters)) {
      if (!unicodeEnabled) return 1;
      if (refreshUnicodeCache(characters)) return 1;

      if (--unicodeRefreshCounter) continue;
      logMessage(LOG_WARNING, "too many unicode cache refresh attempts");
    } else {
      screenCacheUsed = 0;
    }

    unicodeCacheUsed = 0;
    return 0;
  }
}

static int
refresh_LinuxScreen (void) {
  if (screenUpdated) {
    problemText = NULL;

    while (1) {
      {
        int newConsoleNumber = getConsoleNumber();

        if (newConsoleNumber != currentConsoleNumber) {
          if (!screenUpdated) {
            screenUpdated = 1;

            logMessage(LOG_WARNING,
              "foreground console changed during cache refresh - retrying"
            );
          }

          logMessage(LOG_CATEGORY(SCREEN_DRIVER),
            "foreground console number changed: %d -> %d",
            currentConsoleNumber, newConsoleNumber
          );

          currentConsoleNumber = newConsoleNumber;
        } else if (!screenUpdated) {
          break;
        }
      }

      if (currentConsoleNumber == NO_CONSOLE) {
        problemText = gettext("no foreground console");
      } else if (refreshCache()) {
        inTextMode = testTextMode();
        screenUpdated = 0;
        continue;
      } else {
        logMessage(LOG_ERR, "cache refresh failed");
        problemText = gettext("can't read screen content");
      }

      break;
    }

    if (problemText) {
      if (*fallbackText) {
        problemText = gettext(fallbackText);
      }
    }
  }

  return 1;
}

static int
getScreenDescription (ScreenDescription *description) {
  ScreenHeader header;

  if (readScreenHeader(&header)) {
    description->cols = header.size.columns;
    description->rows = header.size.rows;

    description->posx = header.cursor.column;
    description->posy = header.cursor.row;
    if (header.hideCursor) description->hasCursor = 0;

    adjustCursorColumn(&description->posx, description->posy, description->cols);
    return 1;
  }

  problemText = gettext("can't read screen header");
  return 0;
}

static void
describe_LinuxScreen (ScreenDescription *description) {
  if (!screenCacheBuffer) {
    problemText = NULL;
    currentConsoleNumber = getConsoleNumber();
    inTextMode = testTextMode();
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
  ScreenHeader header;

  if (readScreenHeader(&header)) {
    const ScreenSize *size = &header.size;

    if (validateScreenBox(box, size->columns, size->rows)) {
      if (problemText) {
        setScreenMessage(box, buffer, problemText);
        return 1;
      }

      for (unsigned int row=0; row<box->height; row+=1) {
        ScreenCharacter characters[size->columns];
        if (!readScreenRow(box->top+row, size->columns, characters, NULL)) return 0;

        memcpy(buffer, &characters[box->left],
               (box->width * sizeof(characters[0])));

        buffer += box->width;
      }

      return 1;
    }
  }

  return 0;
}

static int
getCapsLockState (void) {
  char leds;
  if (controlCurrentConsole(KDGETLED, &leds) != -1)
    if (leds & LED_CAP)
      return 1;
  return 0;
}

static int
injectKeyboardEvent (int key, int press) {
  logMessage(
    LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
    "injecting key %s: %02X",
    (press? "press": "release"), key
  );

  if (!openKeyboard()) return 0;
  return writeKeyEvent(uinputKeyboard, key, press);
}

static inline int
hasModUpper (ScreenKey modifiers) {
  return (modifiers & SCR_KEY_UPPER) && !getCapsLockState();
}

static inline int
hasModShift (ScreenKey modifiers) {
  return !!(modifiers & SCR_KEY_SHIFT);
}

static inline int
hasModControl (ScreenKey modifiers) {
  return !!(modifiers & SCR_KEY_CONTROL);
}

static inline int
hasModAltLeft (ScreenKey modifiers) {
  return !!(modifiers & SCR_KEY_ALT_LEFT);
}

static inline int
hasModAltRight (ScreenKey modifiers) {
  return !!(modifiers & SCR_KEY_ALT_RIGHT);
}

static inline int
hasModGui (ScreenKey modifiers) {
  return !!(modifiers & SCR_KEY_GUI);
}

static inline int
hasModCapslock (ScreenKey modifiers) {
  return !!(modifiers & SCR_KEY_CAPSLOCK);
}

static int
insertLinuxKey (LinuxKeyCode code, ScreenKey modifiers) {
#ifdef HAVE_LINUX_INPUT_H
  const int modUpper = hasModUpper(modifiers);
  const int modShift = hasModShift(modifiers);
  const int modControl = hasModControl(modifiers);
  const int modAltLeft = hasModAltLeft(modifiers);
  const int modAltRight = hasModAltRight(modifiers);
  const int modGui = hasModGui(modifiers);
  const int modCapslock = hasModCapslock(modifiers);

#define KEY_EVENT(KEY, PRESS) { if (!injectKeyboardEvent((KEY), (PRESS))) return 0; }
  if (modUpper) {
    KEY_EVENT(KEY_CAPSLOCK, 1);
    KEY_EVENT(KEY_CAPSLOCK, 0);
  }

  if (modCapslock) KEY_EVENT(KEY_CAPSLOCK, 1);
  if (modShift) KEY_EVENT(KEY_LEFTSHIFT, 1);
  if (modControl) KEY_EVENT(KEY_LEFTCTRL, 1);
  if (modAltLeft) KEY_EVENT(KEY_LEFTALT, 1);
  if (modAltRight) KEY_EVENT(KEY_RIGHTALT, 1);
  if (modGui) KEY_EVENT(KEY_LEFTMETA, 1);

  if (code) {
    KEY_EVENT(code, 1);
    KEY_EVENT(code, 0);
  }

  if (modGui) KEY_EVENT(KEY_LEFTMETA, 0);
  if (modAltRight) KEY_EVENT(KEY_RIGHTALT, 0);
  if (modAltLeft) KEY_EVENT(KEY_LEFTALT, 0);
  if (modControl) KEY_EVENT(KEY_LEFTCTRL, 0);
  if (modShift) KEY_EVENT(KEY_LEFTSHIFT, 0);
  if (modCapslock) KEY_EVENT(KEY_CAPSLOCK, 0);

  if (modUpper) {
    KEY_EVENT(KEY_CAPSLOCK, 1);
    KEY_EVENT(KEY_CAPSLOCK, 0);
  }
#undef KEY_EVENT

  return 1;
#else /* HAVE_LINUX_INPUT_H */
  return 0;
#endif /* HAVE_LINUX_INPUT_H */
}

static int
insertByte (char byte) {
  logMessage(
    LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
    "inserting byte: %02X", byte
  );

  if (controlCurrentConsole(TIOCSTI, &byte) != -1) return 1;
  logSystemError("ioctl[TIOCSTI]");
  logPossibleCause("BRLTTY is running without the CAP_SYS_ADMIN kernel capability (see man 7 capabilities)");
  logPossibleCause("the sysctl parameter dev.tty.legacy_tiocsti is off (see https://lore.kernel.org/linux-hardening/Y0m9l52AKmw6Yxi1@hostpad/)");

  message(
    NULL, "Linux character injection (TIOCSTI) is disabled on this system",
    (MSG_SILENT)
  );

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

typedef struct {
  const LinuxKeyCode *xtMap;
  unsigned char xtCode;
  unsigned char xtEscape;
} ScreenKeyEntry;

static const ScreenKeyEntry *
getScreenKeyEntry (ScreenKey key, unsigned char *shift) {
  *shift = 0;

#define SCREEN_KEY_ENTRY(KEY, ESCAPE, CODE) \
  case (KEY): { \
    static const ScreenKeyEntry screenKeyEntry = { \
      .xtMap = linuxKeyMap_xt ## ESCAPE, \
      .xtCode = XT_KEY_ ## ESCAPE ## _ ## CODE, \
      .xtEscape = XT_MOD_ ## ESCAPE, \
    }; \
    return &screenKeyEntry; \
  }

#define SCREEN_KEY_SYMBOL(UNSHIFTED, SHIFTED, ESCAPE, CODE) \
  case (SHIFTED): *shift = 1; \
  SCREEN_KEY_ENTRY(UNSHIFTED, ESCAPE, CODE)

  switch (key & SCR_KEY_CHAR_MASK) {
    SCREEN_KEY_ENTRY(SCR_KEY_ESCAPE, 00, Escape)

    SCREEN_KEY_ENTRY(SCR_KEY_F1, 00, F1)
    SCREEN_KEY_ENTRY(SCR_KEY_F2, 00, F2)
    SCREEN_KEY_ENTRY(SCR_KEY_F3, 00, F3)
    SCREEN_KEY_ENTRY(SCR_KEY_F4, 00, F4)
    SCREEN_KEY_ENTRY(SCR_KEY_F5, 00, F5)
    SCREEN_KEY_ENTRY(SCR_KEY_F6, 00, F6)
    SCREEN_KEY_ENTRY(SCR_KEY_F7, 00, F7)
    SCREEN_KEY_ENTRY(SCR_KEY_F8, 00, F8)
    SCREEN_KEY_ENTRY(SCR_KEY_F9, 00, F9)
    SCREEN_KEY_ENTRY(SCR_KEY_F10, 00, F10)
    SCREEN_KEY_ENTRY(SCR_KEY_F11, 00, F11)
    SCREEN_KEY_ENTRY(SCR_KEY_F12, 00, F12)

    SCREEN_KEY_ENTRY(SCR_KEY_F13, 00, F13)
    SCREEN_KEY_ENTRY(SCR_KEY_F14, 00, F14)
    SCREEN_KEY_ENTRY(SCR_KEY_F15, 00, F15)
    SCREEN_KEY_ENTRY(SCR_KEY_F16, 00, F16)
    SCREEN_KEY_ENTRY(SCR_KEY_F17, 00, F17)
    SCREEN_KEY_ENTRY(SCR_KEY_F18, 00, F18)
    SCREEN_KEY_ENTRY(SCR_KEY_F19, 00, F19)
    SCREEN_KEY_ENTRY(SCR_KEY_F20, 00, F20)
    SCREEN_KEY_ENTRY(SCR_KEY_F21, 00, F21)
    SCREEN_KEY_ENTRY(SCR_KEY_F22, 00, F22)
    SCREEN_KEY_ENTRY(SCR_KEY_F23, 00, F23)
    SCREEN_KEY_ENTRY(SCR_KEY_F24, 00, F24)

    SCREEN_KEY_SYMBOL('`', '~', 00, Grave)
    SCREEN_KEY_SYMBOL('1', '!', 00, 1)
    SCREEN_KEY_SYMBOL('2', '@', 00, 2)
    SCREEN_KEY_SYMBOL('3', '#', 00, 3)
    SCREEN_KEY_SYMBOL('4', '$', 00, 4)
    SCREEN_KEY_SYMBOL('5', '%', 00, 5)
    SCREEN_KEY_SYMBOL('6', '^', 00, 6)
    SCREEN_KEY_SYMBOL('7', '&', 00, 7)
    SCREEN_KEY_SYMBOL('8', '*', 00, 8)
    SCREEN_KEY_SYMBOL('9', '(', 00, 9)
    SCREEN_KEY_SYMBOL('0', ')', 00, 0)
    SCREEN_KEY_SYMBOL('-', '_', 00, Minus)
    SCREEN_KEY_SYMBOL('=', '+', 00, Equal)
    SCREEN_KEY_ENTRY(SCR_KEY_BACKSPACE, 00, Backspace)

    SCREEN_KEY_ENTRY(SCR_KEY_TAB, 00, Tab)
    SCREEN_KEY_SYMBOL('q', 'Q', 00, Q)
    SCREEN_KEY_SYMBOL('w', 'W', 00, W)
    SCREEN_KEY_SYMBOL('e', 'E', 00, E)
    SCREEN_KEY_SYMBOL('r', 'R', 00, R)
    SCREEN_KEY_SYMBOL('t', 'T', 00, T)
    SCREEN_KEY_SYMBOL('y', 'Y', 00, Y)
    SCREEN_KEY_SYMBOL('u', 'U', 00, U)
    SCREEN_KEY_SYMBOL('i', 'I', 00, I)
    SCREEN_KEY_SYMBOL('o', 'O', 00, O)
    SCREEN_KEY_SYMBOL('p', 'P', 00, P)
    SCREEN_KEY_SYMBOL('[', '{', 00, LeftBracket)
    SCREEN_KEY_SYMBOL(']', '}', 00, RightBracket)
    SCREEN_KEY_SYMBOL('\\', '|', 00, Backslash)

    SCREEN_KEY_SYMBOL('a', 'A', 00, A)
    SCREEN_KEY_SYMBOL('s', 'S', 00, S)
    SCREEN_KEY_SYMBOL('d', 'D', 00, D)
    SCREEN_KEY_SYMBOL('f', 'F', 00, F)
    SCREEN_KEY_SYMBOL('g', 'G', 00, G)
    SCREEN_KEY_SYMBOL('h', 'H', 00, H)
    SCREEN_KEY_SYMBOL('j', 'J', 00, J)
    SCREEN_KEY_SYMBOL('k', 'K', 00, K)
    SCREEN_KEY_SYMBOL('l', 'L', 00, L)
    SCREEN_KEY_SYMBOL(';', ':', 00, Semicolon)
    SCREEN_KEY_SYMBOL('\'', '"', 00, Apostrophe)
    SCREEN_KEY_ENTRY(SCR_KEY_ENTER, 00, Enter)

    SCREEN_KEY_SYMBOL('z', 'Z', 00, Z)
    SCREEN_KEY_SYMBOL('x', 'X', 00, X)
    SCREEN_KEY_SYMBOL('c', 'C', 00, C)
    SCREEN_KEY_SYMBOL('v', 'V', 00, V)
    SCREEN_KEY_SYMBOL('b', 'B', 00, B)
    SCREEN_KEY_SYMBOL('n', 'N', 00, N)
    SCREEN_KEY_SYMBOL('m', 'M', 00, M)
    SCREEN_KEY_SYMBOL(',', '<', 00, Comma)
    SCREEN_KEY_SYMBOL('.', '>', 00, Period)
    SCREEN_KEY_SYMBOL('/', '?', 00, Slash)

    SCREEN_KEY_ENTRY(' ', 00, Space)

    SCREEN_KEY_ENTRY(SCR_KEY_INSERT, E0, Insert)
    SCREEN_KEY_ENTRY(SCR_KEY_DELETE, E0, Delete)
    SCREEN_KEY_ENTRY(SCR_KEY_HOME, E0, Home)
    SCREEN_KEY_ENTRY(SCR_KEY_END, E0, End)
    SCREEN_KEY_ENTRY(SCR_KEY_PAGE_UP, E0, PageUp)
    SCREEN_KEY_ENTRY(SCR_KEY_PAGE_DOWN, E0, PageDown)

    SCREEN_KEY_ENTRY(SCR_KEY_CURSOR_UP, E0, ArrowUp)
    SCREEN_KEY_ENTRY(SCR_KEY_CURSOR_LEFT, E0, ArrowLeft)
    SCREEN_KEY_ENTRY(SCR_KEY_CURSOR_DOWN, E0, ArrowDown)
    SCREEN_KEY_ENTRY(SCR_KEY_CURSOR_RIGHT, E0, ArrowRight)
  }

#undef SCREEN_KEY_SYMBOL
#undef SCREEN_KEY_ENTRY

  return NULL;
}

static int
insertKeyCode (ScreenKey screenKey, int raw) {
  setScreenKeyModifiers(&screenKey, SCR_KEY_SHIFT | SCR_KEY_CONTROL);

  unsigned char shift = 0;
  const ScreenKeyEntry *ske = getScreenKeyEntry(screenKey, &shift);
  if (shift) screenKey |= SCR_KEY_SHIFT;

  if (!ske) {
    logMessage(LOG_WARNING, "key not supported in raw keyboard mode: %04X", screenKey);
    return 0;
  }

  if (raw) {
    const int modUpper = hasModUpper(screenKey);
    const int modShift = hasModShift(screenKey);
    const int modControl = hasModControl(screenKey);
    const int modAltLeft = hasModAltLeft(screenKey);
    const int modAltRight = hasModAltRight(screenKey);
    const int modGui = hasModGui(screenKey);
    const int modCapslock = hasModCapslock(screenKey);

    char codes[24];
    unsigned int count = 0;

    if (modUpper) {
      codes[count++] = XT_KEY_00_CapsLock;
      codes[count++] = XT_KEY_00_CapsLock | XT_BIT_RELEASE;
    }

    if (modCapslock) codes[count++] = XT_KEY_00_CapsLock;
    if (modShift) codes[count++] = XT_KEY_00_LeftShift;
    if (modControl) codes[count++] = XT_KEY_00_LeftControl;
    if (modAltLeft) codes[count++] = XT_KEY_00_LeftAlt;

    if (modAltRight) {
      codes[count++] = XT_MOD_E0;
      codes[count++] = XT_KEY_E0_RightAlt;
    }

    if (modGui) {
      codes[count++] = XT_MOD_E0;
      codes[count++] = XT_KEY_E0_LeftGUI;
    }

    if (ske->xtEscape) codes[count++] = ske->xtEscape;
    codes[count++] = ske->xtCode;

    if (ske->xtEscape) codes[count++] = ske->xtEscape;
    codes[count++] = ske->xtCode | XT_BIT_RELEASE;

    if (modGui) {
      codes[count++] = XT_MOD_E0;
      codes[count++] = XT_KEY_E0_LeftGUI | XT_BIT_RELEASE;
    }

    if (modAltRight) {
      codes[count++] = XT_MOD_E0;
      codes[count++] = XT_KEY_E0_RightAlt | XT_BIT_RELEASE;
    }

    if (modAltLeft) codes[count++] = XT_KEY_00_LeftAlt | XT_BIT_RELEASE;
    if (modControl) codes[count++] = XT_KEY_00_LeftControl | XT_BIT_RELEASE;
    if (modShift) codes[count++] = XT_KEY_00_LeftShift | XT_BIT_RELEASE;
    if (modCapslock) codes[count++] = XT_KEY_00_CapsLock | XT_BIT_RELEASE;

    if (modUpper) {
      codes[count++] = XT_KEY_00_CapsLock;
      codes[count++] = XT_KEY_00_CapsLock | XT_BIT_RELEASE;
    }

    return insertBytes(codes, count);
  } else {
    LinuxKeyCode linuxKey = ske->xtMap[ske->xtCode];

    if (!linuxKey) {
      logMessage(LOG_WARNING, "key not supported in medium raw keyboard mode: %04X", screenKey);
      return 0;
    }

    return insertLinuxKey(linuxKey, screenKey);
  }
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
        if (insertKeyCode(key, 0)) return 1;
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

      if (controlCurrentConsole(KDGKBMETA, &meta) == -1) return 0;

      switch (meta) {
        case K_ESCPREFIX:
          *--character = ASCII_ESC;
          break;

        case K_METABIT:
          if (*character >= 0X80) {
            logMessage(LOG_WARNING, "can't add meta bit to character: U+%04X", (uint32_t)*character);
            return 0;
          }

          *character |= 0X80;
          break;

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
logKeyboardMode (const char *mode) {
  return logMessage(
    LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
    "keyboard mode is %s", mode
  );
}

static int
insertKey_LinuxScreen (ScreenKey key) {
  int ok = 0;
  int mode;

  if (controlCurrentConsole(KDGKBMODE, &mode) != -1) {
    switch (mode) {
      case K_RAW:
        logKeyboardMode("raw");
        if (insertKeyCode(key, 1)) ok = 1;
        break;

      case K_MEDIUMRAW:
        logKeyboardMode("medium raw");
        if (insertKeyCode(key, 0)) ok = 1;
        break;

      case K_XLATE:
        logKeyboardMode("translated");
        if (insertTranslated(key, insertXlate)) ok = 1;
        break;

      case K_UNICODE:
        logKeyboardMode("unicode");
        if (insertTranslated(key, insertUnicode)) ok = 1;
        break;

#ifdef K_OFF
      case K_OFF:
        logKeyboardMode("off");
        if (insertKeyCode(key, 0)) ok = 1;
        break;
#endif /* K_OFF */

      default:
        logMessage(LOG_WARNING, "unsupported keyboard mode: %d", mode);
        break;
    }
  } else {
    logSystemError("ioctl[KDGKBMODE]");
  }

  return ok;
}

static int
canBracketPaste_LinuxScreen (void) {
  unsigned char subcode = TIOCL_GETBRACKETEDPASTE;
  int result = controlCurrentConsole(TIOCLINUX, &subcode);
  if (result > 0) return 1;

  if (result == -1) {
    if (errno == EINVAL) {
      static unsigned char alreadyLogged = 0;

      logKernelLimitation(
        &alreadyLogged, 6, 16,
        "cannot determine current console bracketed paste state"
      );
    } else {
      logSystemError("ioctl[TIOCLINUX(TIOCL_GETBRACKETEDPASTE)]");
    }
  }

  return 0;
}

typedef struct {
  char subcode;
  struct tiocl_selection selection;
} PACKED RegionSelectionArgument;

static int
selectRegion (RegionSelectionArgument *argument) {
  if (controlCurrentConsole(TIOCLINUX, argument) != -1) return 1;
  if (errno != EINVAL) logSystemError("ioctl[TIOCLINUX]");
  return 0;
}

static int
highlightRegion_LinuxScreen (int left, int right, int top, int bottom) {
  RegionSelectionArgument argument = {
    .subcode = TIOCL_SETSEL,

    .selection = {
      .xs = left + 1,
      .ys = top + 1,
      .xe = right + 1,
      .ye = bottom + 1,
      .sel_mode = TIOCL_SELCHAR
    }
  };

  return selectRegion(&argument);
}

static int
unhighlightRegion_LinuxScreen (void) {
  RegionSelectionArgument argument = {
    .subcode = TIOCL_SETSEL,

    .selection = {
      .xs = 0,
      .ys = 0,
      .xe = 0,
      .ye = 0,
      .sel_mode = TIOCL_SELCLEAR
    }
  };

  return selectRegion(&argument);
}

static int
validateVirtualTerminalNumber (int vt) {
  if ((vt >= 1) && (vt <= MAX_NR_CONSOLES)) return 1;
  logMessage(LOG_WARNING, "virtual terminal out of range: %d", vt);
  return 0;
}

static int
selectVirtualTerminal_LinuxScreen (int vt) {
  if (vt == selectedVirtualTerminal) return 1;
  if (vt && !validateVirtualTerminalNumber(vt)) return 0;
  return setCurrentScreen(vt);
}

static int
switchVirtualTerminal_LinuxScreen (int vt) {
  if (validateVirtualTerminalNumber(vt)) {
    if (selectVirtualTerminal_LinuxScreen(0)) {
      if (controlMainConsole(VT_ACTIVATE, (void *)(intptr_t)vt) != -1) {
        logMessage(LOG_CATEGORY(SCREEN_DRIVER),
                   "switched to virtual tertminal %d", vt);
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
userVirtualTerminal_LinuxScreen (int number) {
  return MAX_NR_CONSOLES + 1 + number;
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
          if (command & BRL_FLG_KBD_RELEASE) arg |= XT_BIT_RELEASE;

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

            if (key) return injectKeyboardEvent(key, press);
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

            if (key) return injectKeyboardEvent(key, press);
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

            if (key) return injectKeyboardEvent(key, press);
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
  gpmIncludeScreenHandlers(main);

  main->base.poll = poll_LinuxScreen;
  main->base.refresh = refresh_LinuxScreen;
  main->base.describe = describe_LinuxScreen;
  main->base.readCharacters = readCharacters_LinuxScreen;
  main->base.insertKey = insertKey_LinuxScreen;
  main->base.canBracketPaste = canBracketPaste_LinuxScreen;
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

/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <linux/kd.h>

#include "Programs/misc.h"
#include "Programs/brldefs.h"

#include "Programs/scr_driver.h"
#include "screen.h"

static unsigned int debugCharacterTranslationTable = 0;
static unsigned int debugApplicationCharacterMap = 0;
static unsigned int debugScreenFontMap = 0;
static unsigned int debugScreenTextTranslation = 0;

/* Copied from linux-2.2.17/drivers/char/consolemap.c: translations[0]
 * 8-bit Latin-1 mapped to Unicode -- trivial mapping
 */
static const ApplicationCharacterMap iso01Map = {
   0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
   0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
   0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
   0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
   0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
   0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
   0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
   0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
   0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
   0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
   0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
   0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
   0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
   0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
   0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
   0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
   0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
   0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
   0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
   0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
   0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
   0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
   0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
   0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
   0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
   0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
   0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
   0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
   0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
   0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
   0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
   0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff
};

/* Copied from linux-2.2.17/drivers/char/consolemap.c: translations[1]
 * VT100 graphics mapped to Unicode
 */
static const ApplicationCharacterMap vt100Map = {
   0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
   0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
   0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
   0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
   0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
   0x0028, 0x0029, 0x002a, 0x2192, 0x2190, 0x2191, 0x2193, 0x002f,
   0x2588, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
   0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
   0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
   0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
   0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
   0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x00a0,
   0x25c6, 0x2592, 0x2409, 0x240c, 0x240d, 0x240a, 0x00b0, 0x00b1,
   0x2591, 0x240b, 0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0xf800,
   0xf801, 0x2500, 0xf803, 0xf804, 0x251c, 0x2524, 0x2534, 0x252c,
   0x2502, 0x2264, 0x2265, 0x03c0, 0x2260, 0x00a3, 0x00b7, 0x007f,
   0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
   0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
   0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
   0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
   0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
   0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
   0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
   0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
   0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
   0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
   0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
   0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
   0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
   0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
   0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
   0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff
};

/* Copied from linux-2.2.17/drivers/char/consolemap.c: translations[2]
 * IBM Codepage 437 mapped to Unicode
 */
static const ApplicationCharacterMap cp437Map = {
   0x0000, 0x263a, 0x263b, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022, 
   0x25d8, 0x25cb, 0x25d9, 0x2642, 0x2640, 0x266a, 0x266b, 0x263c,
   0x25b6, 0x25c0, 0x2195, 0x203c, 0x00b6, 0x00a7, 0x25ac, 0x21a8,
   0x2191, 0x2193, 0x2192, 0x2190, 0x221f, 0x2194, 0x25b2, 0x25bc,
   0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
   0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
   0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
   0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
   0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
   0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
   0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
   0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
   0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
   0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
   0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
   0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x2302,
   0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
   0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
   0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
   0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,
   0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba,
   0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
   0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
   0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
   0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f,
   0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
   0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b,
   0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
   0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4,
   0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,
   0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248,
   0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0
};

static const char *
findDevicePath (const char *paths, const char *description, int mode) {
  char *devices = strdupWrapper(paths);
  char *tokens = devices;
  const char *delimiters = " ";
  char *path = NULL;
  int exists = 0;
  char *device;
  while ((device = strtok(tokens, delimiters))) {
    tokens = NULL;
    device = strdupWrapper(device);
    LogPrint(LOG_DEBUG, "Checking %s Device: %s",
             description, device);
    if (access(device, mode) == -1) {
      LogPrint(LOG_DEBUG, "%s Device access error: %s: %s",
               description, device, strerror(errno));
    } else {
      struct stat status;
      if (stat(device, &status) == -1) {
        LogPrint(LOG_ERR, "%s Device stat error: %s: %s",
                 description, device, strerror(errno));
      } else if (!S_ISCHR(status.st_mode)) {
        LogPrint(LOG_ERR, "%s Device not character special: %s",
                 description, device);
      } else {
        if (path) free(path);
        path = device;
        exists = 1;
        break;
      }
    }
    if (errno != ENOENT) {
      if (!exists) {
        exists = 1;
        if (path) {
          free(path);
          path = NULL;
        }
      }
    }
    if (path)
      free(device);
    else
      path = device;
  }
  free(devices);
  return path;
}

static int
setDevicePath (const char **path, const char *paths, const char *description, int mode) {
  LogPrint(LOG_DEBUG, "%s Device List: %s", description, paths);
  if ((*path = findDevicePath(paths, description, mode))) {
    LogPrint(LOG_INFO, "%s Device: %s", description, *path);
    return 1;
  } else {
    LogPrint(LOG_ERR, "%s Device not specified.", description);
  }
  return 0;
}

static char *
vtPath (const char *base, unsigned char vt) {
  if (vt) {
    size_t length = strlen(base);
    char buffer[length+4];
    if (base[length-1] == '0') --length;
    strncpy(buffer, base, length);
    sprintf(buffer+length,  "%u", vt);
    return strdup(buffer);
  }
  return strdup(base);
}

static int
openDevice (const char *path, const char *description, int flags, int major, int minor) {
  int file;
  LogPrint(LOG_DEBUG, "Opening %s device: %s", description, path);
  if ((file = open(path, flags)) == -1) {
    LogPrint(LOG_ERR, "Cannot open %s device: %s: %s",
             description, path, strerror(errno));
    if (errno == ENOENT) {
      mode_t mode = S_IFCHR | S_IRUSR | S_IWUSR;
      LogPrint(LOG_NOTICE, "Creating %s device: %s mode=%06o major=%d minor=%d",
               description, path, mode, major, minor);
      if (mknod(path, mode, makedev(major, minor)) == -1) {
        LogPrint(LOG_ERR, "Cannot create %s device: %s: %s",
                 description, path, strerror(errno));
      } else if ((file = open(path, flags)) == -1) {
        LogPrint(LOG_ERR, "Cannot open %s device: %s: %s",
                 description, path, strerror(errno));
        if (unlink(path) == -1)
          LogPrint(LOG_ERR, "Cannot remove %s device: %s: %s",
                   description, path, strerror(errno));
        else
          LogPrint(LOG_NOTICE, "Removed %s device: %s",
                   description, path);
      }
    }
  }
  return file;
}

static const char *consolePath;
static int consoleDescriptor;

static int
setConsolePath (void) {
  return setDevicePath(&consolePath, LINUX_CONSOLE_DEVICES, "Console", R_OK|W_OK);
}

static void
closeConsole (void) {
  if (consoleDescriptor != -1) {
    if (close(consoleDescriptor) == -1) {
      LogError("Console close");
    }
    LogPrint(LOG_DEBUG, "Console closed: fd=%d", consoleDescriptor);
    consoleDescriptor = -1;
  }
}

static int
openConsole (unsigned char vt) {
  char *path = vtPath(consolePath, vt);
  if (path) {
    int console = openDevice(path, "console", O_RDWR|O_NOCTTY, 4, vt);
    if (console != -1) {
      closeConsole();
      consoleDescriptor = console;
      LogPrint(LOG_DEBUG, "Console opened: %s: fd=%d", path, consoleDescriptor);
      free(path);
      return 1;
    }
    free(path);
  }
  return 0;
}

static const char *screenPath;
static int screenDescriptor;
static unsigned char virtualTerminal;

static int
setScreenPath (void) {
  return setDevicePath(&screenPath, LINUX_SCREEN_DEVICES, "Screen", R_OK|W_OK);
}

static void
closeScreen (void) {
  if (screenDescriptor != -1) {
    if (close(screenDescriptor) == -1) {
      LogError("Screen close");
    }
    LogPrint(LOG_DEBUG, "Screen closed: fd=%d", screenDescriptor);
    screenDescriptor = -1;
  }
}

static int
openScreen (unsigned char vt) {
  char *path = vtPath(screenPath, vt);
  if (path) {
    int screen = openDevice(path, "screen", O_RDWR, 7, 0X80|vt);
    if (screen != -1) {
      if (openConsole(vt)) {
        closeScreen();
        screenDescriptor = screen;
        LogPrint(LOG_DEBUG, "Screen opened: %s: fd=%d", path, screenDescriptor);
        free(path);
        virtualTerminal = vt;
        return 1;
      }
      close(screen);
    }
    free(path);
  }
  return 0;
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

static ApplicationCharacterMap applicationCharacterMap;
static int (*setApplicationCharacterMap) (int force);

static void
logApplicationCharacterMap (void) {
  if (debugApplicationCharacterMap) {
    char buffer[0X80];
    char *address = NULL;
    unsigned char character = 0;
    while (1) {
      if (!(character % 8)) {
        if (address) {
          LogPrint(LOG_DEBUG, "%s", buffer);
          if (!character) break;
        }
        address = buffer;
        address += sprintf(address, "acm[%02X]:", character);
      }
      address += sprintf(address, " %4.4X", applicationCharacterMap[character++]);
    }
  }
}

static int
getUserAcm (int force) {
  ApplicationCharacterMap map;
  if (controlConsole(GIO_UNISCRNMAP, &map) != -1) {
    if (force || (memcmp(applicationCharacterMap, map, sizeof(applicationCharacterMap)) != 0)) {
      memcpy(applicationCharacterMap, map, sizeof(applicationCharacterMap));
      if (!force) LogPrint(LOG_DEBUG, "User application character map changed.");
      logApplicationCharacterMap();
      return 1;
    }
  } else {
    LogError("ioctl GIO_UNISCRNMAP");
  }
  return 0;
}

static int
determineApplicationCharacterMap (int force) {
  const char *name = NULL;
  if (!getUserAcm(force)) return 0;
  {
    unsigned short character;
    for (character=0; character<0X100; ++character) {
      if (applicationCharacterMap[character] != (character | 0XF000)) {
        setApplicationCharacterMap = &getUserAcm;
        name = "user";
        break;
      }
    }
  }
  if (!name) {
    memcpy(applicationCharacterMap, iso01Map, sizeof(applicationCharacterMap));
    setApplicationCharacterMap = NULL;
    logApplicationCharacterMap();
    name = "iso01";
  }
  LogPrint(LOG_INFO, "Application Character Map: %s", name);
  return 1;
}

static struct unipair *screenFontMapTable;
static unsigned short screenFontMapCount;
static unsigned short screenFontMapSize;
static int
setScreenFontMap (int force) {
  struct unimapdesc sfm;
  unsigned short size = force? 0: screenFontMapCount;
  if (!size) size = 0X100;
  while (1) {
    sfm.entry_ct = size;
    if (!(sfm.entries = malloc(sfm.entry_ct * sizeof(*sfm.entries)))) {
      LogError("Screen font map allocation");
      return 0;
    }
    if (controlConsole(GIO_UNIMAP, &sfm) != -1) break;
    free(sfm.entries);
    if (errno != ENOMEM) {
      LogError("ioctl GIO_UNIMAP");
      return 0;
    }
    if (!(size <<= 1)) {
      LogPrint(LOG_ERR, "Screen font map too big.");
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

static const char *const screenParameters[] = {
  "acm",
  "debugacm",
  "debugsfm",
  "debugctt",
  NULL
};
typedef enum {
  PARM_ACM,
  PARM_DEBUGACM,
  PARM_DEBUGSFM,
  PARM_DEBUGCTT
} ScreenParameters;
static const char *const *
parameters_LinuxScreen (void) {
  return screenParameters;
}

static int
prepare_LinuxScreen (char **parameters) {
  validateYesNo(&debugApplicationCharacterMap, "debug application character map flag", parameters[PARM_DEBUGACM]);
  validateYesNo(&debugScreenFontMap, "debug screen font map flag", parameters[PARM_DEBUGSFM]);
  validateYesNo(&debugCharacterTranslationTable, "debug character translation table flag", parameters[PARM_DEBUGCTT]);
  setApplicationCharacterMap = &determineApplicationCharacterMap;
  {
    static const char *choices[] = {"auto", "iso01", "vt100", "cp437", "user", NULL};
    unsigned int choice;
    if (validateChoice(&choice, "character set", parameters[PARM_ACM], choices)) {
      if (choice) {
        static const unsigned short *maps[] = {iso01Map, vt100Map, cp437Map, NULL};
        const unsigned short *map = maps[choice-1];
        if (map) {
          memcpy(applicationCharacterMap, map, sizeof(applicationCharacterMap));
          setApplicationCharacterMap = NULL;
          logApplicationCharacterMap();
        } else {
          setApplicationCharacterMap = &getUserAcm;
        }
      }
    }
  }
  if (setScreenPath()) {
    screenDescriptor = -1;
    if (setConsolePath()) {
      consoleDescriptor = -1;
      return 1;
    }
  }
  return 0;
}

static int
open_LinuxScreen (void) {
  return openScreen(0);
}

/* 
 * The virtual screen devices return the actual font positions of the glyphs to
 * be drawn on the screen. The problem is that the font may not have been
 * designed for the character set being used. Most PC video cards have a
 * built-in font defined for the CP437 character set, but Linux users often use
 * the ISO-Latin-1 character set. In addition, the PSF format used by the newer
 * font files, which contains an internal unicode to font-position table,
 * allows the actual font positions to be unrelated to any known character set.
 * The kernel translates each character to be written to the screen from the
 * character set being used into unicode, and then from unicode into the
 * position within the font being used of the glyph to be drawn on the screen.
 * We need to reverse this translation in order to get the character code in
 * the expected character set.
 */
static unsigned char translationTable[0X200];
static int
setTranslationTable (int force) {
  int acmChanged = setApplicationCharacterMap && setApplicationCharacterMap(force);
  int sfmChanged = setScreenFontMap(force);
  int vccChanged = (sfmChanged || force)? setVgaCharacterCount(force): 0;

  if (acmChanged || sfmChanged || vccChanged) {
    unsigned short directPosition = 0XFF;
    if (vgaLargeTable) directPosition |= 0X100;

    memset(translationTable, '?', sizeof(translationTable));
    {
       int character;
       for (character=0XFF; character>=0; --character) {
         unsigned short unicode = applicationCharacterMap[character];
         int position = -1;
         if (!screenFontMapCount) {
           if (unicode < 0X100) position = unicode;
         } else if ((unicode & ~directPosition) == 0XF000) {
           position = unicode & directPosition;
         } else {
           int first = 0;
           int last = screenFontMapCount-1;
           while (first <= last) {
             int current = (first + last) / 2;
             struct unipair *map = &screenFontMapTable[current];
             if (map->unicode < unicode)
               first = current + 1;
             else if (map->unicode > unicode)
               last = current - 1;
             else {
               if (map->fontpos < vgaCharacterCount) position = map->fontpos;
               break;
             }
           }
         }
         if (position < 0) {
           if (debugCharacterTranslationTable) {
             LogPrint(LOG_DEBUG, "No character mapping: char=%2.2X unum=%4.4X", character, unicode);
           }
         } else {
           translationTable[position] = character;
           if (debugCharacterTranslationTable) {
             LogPrint(LOG_DEBUG, "Character mapping: char=%2.2X unum=%4.4X fpos=%2.2X",
                      character, unicode, position);
           }
         }
       }
    }
    if (debugCharacterTranslationTable) {
      const unsigned int count = 0X10;
      int position;
      for (position=0; position<vgaCharacterCount; position+=count) {
        char description[0X20];
        sprintf(description, "c2f[%02X]", position);
        LogBytes(description, &translationTable[position], count);
      }
    }
    return 1;
  }

  return 0;
}

static int
setup_LinuxScreen (void) {
  if (setTranslationTable(1)) {
    return 1;
  }
  return 0;
}

static void
close_LinuxScreen (void) {
  closeConsole();
  closeScreen();
}

static void
getScreenDescription (ScreenDescription *description) {
  if (lseek(screenDescriptor, 0, SEEK_SET) != -1) {
    unsigned char buffer[4];
    int count = read(screenDescriptor, buffer, sizeof(buffer));
    if (count == sizeof(buffer)) {
      description->rows = buffer[0];
      description->cols = buffer[1];
      description->posx = buffer[2];
      description->posy = buffer[3];
    } else if (count == -1) {
      LogError("Screen header read");
    } else {
      long int expected = sizeof(buffer);
      LogPrint(LOG_ERR, "Truncated screen header: expected %ld bytes, read %d.",
               expected, count);
    }
  } else {
    LogError("Screen seek");
  }
}

static void
getConsoleDescription (ScreenDescription *description) {
  if (virtualTerminal) {
    description->no = virtualTerminal;
  } else {
    struct vt_stat state;
    if (controlConsole(VT_GETSTATE, &state) != -1) {
      description->no = state.v_active;
    } else {
      LogError("ioctl VT_GETSTATE");
    }
  }
}

static void
describe_LinuxScreen (ScreenDescription *description) {
  getScreenDescription(description);
  getConsoleDescription(description);

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

static unsigned char *
read_LinuxScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  ScreenDescription description;
  describe_LinuxScreen(&description);
  if (validateScreenBox(&box, description.cols, description.rows)) {
    int text = mode == SCR_TEXT;
    off_t start = 4 + (box.top * description.cols + box.left) * 2;
    if (lseek(screenDescriptor, start, SEEK_SET) != -1) {
      int length = box.width * 2;
      unsigned char line[length];
      unsigned char *target = buffer;
      off_t increment = description.cols * 2 - length;
      int row;
      for (row=0; row<box.height; ++row) {
        int count;
        unsigned char *source;

        if (row) {
          if (lseek(screenDescriptor, increment, SEEK_CUR) == -1) {
            LogError("Screen seek");
            return NULL;
          }
        }

        count = read(screenDescriptor, line, length);
        if (count != length) {
          if (count == -1) {
            LogError("Screen data read");
          } else {
            LogPrint(LOG_ERR, "Truncated screen data: expected %d bytes, read %d.",
                     length, count);
          }
          return NULL;
        }

        source = line;
        if (text) {
          unsigned char src[box.width];
          unsigned char *trg = target;
          int column;
          for (column=0; column<box.width; ++column) {
            int position = *source;
            if (vgaLargeTable)
              if (source[1] & 0X08)
                position |= 0X100;
            src[column] = *source;

            *target++ = translationTable[position];
            source += 2;
          }
          if (debugScreenTextTranslation) {
            char desc[0X20];
            sprintf(desc, "fpos[%03d,%03d]", box.left, box.top+row);
            LogBytes(desc, src, box.width);
            memcpy(desc, "char", 4);
            LogBytes(desc, trg, box.width);
          }
        } else {
          int column;
          source++;
          for (column=0; column<box.width; ++column) {
            if (vgaLargeTable) *source &= 0XF7;
            *target++ = *source;
            source += 2;
          }
        }
      }
      return buffer;
    } else {
      LogError("Screen seek");
    }
  } else {
    LogPrint(LOG_ERR, "Invalid screen area: cols=%d left=%d width=%d rows=%d top=%d height=%d",
             description.cols, box.left, box.width,
             description.rows, box.top, box.height);
  }
  return NULL;
}

static int
insertByte (unsigned char byte) {
  if (controlConsole(TIOCSTI, &byte) != -1) {
    return 1;
  } else {
    LogError("ioctl TIOCSTI");
  }
  return 0;
}

static int
insertCode (ScreenKey key, int raw) {
  unsigned char prefix = 0X00;
  unsigned char code;
  int modShift = 0;
  int modControl = 0;
  int modMeta = 0;

  if (key < SCR_KEY_ENTER) {
    if (key & SCR_KEY_MOD_META) {
      key &= ~SCR_KEY_MOD_META;
      modMeta = 1;
    }

    if ((key >= 'A') && (key <= 'Z')) {
      key = (key - 'A') + 'a';
      modShift = 1;
    } else if (!(key & 0XE0)) {
      key |= 0X60;
      modControl = 1;
    }
  }

  switch (key) {
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
    case SCR_KEY_ENTER:        code = 0X1C; break;
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
      if (raw) {
        prefix = 0XE0;
        switch (key) {
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
            LogPrint(LOG_WARNING, "Key %4.4X not suported in scancode mode.", key);
            return 0;
        }
      } else {
        switch (key) {
          case SCR_KEY_INSERT:       code = 0X6E; break;
          case SCR_KEY_HOME:         code = 0X66; break;
          case SCR_KEY_PAGE_UP:      code = 0X68; break;
          case SCR_KEY_DELETE:       code = 0X6F; break;
          case SCR_KEY_END:          code = 0X6B; break;
          case SCR_KEY_PAGE_DOWN:    code = 0X6D; break;
          case SCR_KEY_CURSOR_UP:    code = 0X67; break;
          case SCR_KEY_CURSOR_LEFT:  code = 0X69; break;
          case SCR_KEY_CURSOR_DOWN:  code = 0X6C; break;
          case SCR_KEY_CURSOR_RIGHT: code = 0X6A; break;
          default:
            LogPrint(LOG_WARNING, "Key %4.4X not suported in keycode mode.", key);
            return 0;
        }
      }
      break;
  }

  {
    unsigned char buffer[10];
    unsigned short count = 0;
    const unsigned char *byte = buffer;

    if (modControl) buffer[count++] = 0X1D;
    if (modMeta) buffer[count++] = 0X38;
    if (modShift) buffer[count++] = 0X2A;
    if (prefix) buffer[count++] = prefix;
    buffer[count++] = code;

    if (prefix) buffer[count++] = prefix;
    buffer[count++] = code | 0X80;
    if (modShift) buffer[count++] = 0X2A | 0X80;
    if (modMeta) buffer[count++] = 0X38 | 0X80;
    if (modControl) buffer[count++] = 0X1D | 0X80;

    while (count--) {
      if (!insertByte(*byte++)) return 0;
    }
  }
  return 1;
}

static int
insertMapped (ScreenKey key, int (*byteInserter)(unsigned char byte)) {
  char buffer[2];
  char *sequence;
  char *end;

  if (key < SCR_KEY_ENTER) {
    sequence = end = buffer + sizeof(buffer);
    *--sequence = key & 0XFF;

    if (key & SCR_KEY_MOD_META) {
      long meta;
      if (controlConsole(KDGKBMETA, &meta) == -1) return 0;

      switch (meta) {
        case K_METABIT:
          *sequence |= 0X80;
          break;

        case K_ESCPREFIX:
          *--sequence = 0X1B;
          break;

        default:
          LogPrint(LOG_WARNING, "Unsupported keyboard meta mode: %ld", meta);
          return 0;
      }
    }
  } else {
    switch (key) {
      case SCR_KEY_ENTER:
        sequence = "\r";
        break;
      case SCR_KEY_TAB:
        sequence = "\t";
        break;
      case SCR_KEY_BACKSPACE:
        sequence = "\x7f";
        break;
      case SCR_KEY_ESCAPE:
        sequence = "\x1b";
        break;
      case SCR_KEY_CURSOR_LEFT:
        sequence = "\x1b[D";
        break;
      case SCR_KEY_CURSOR_RIGHT:
        sequence = "\x1b[C";
        break;
      case SCR_KEY_CURSOR_UP:
        sequence = "\x1b[A";
        break;
      case SCR_KEY_CURSOR_DOWN:
        sequence = "\x1b[B";
        break;
      case SCR_KEY_PAGE_UP:
        sequence = "\x1b[5~";
        break;
      case SCR_KEY_PAGE_DOWN:
        sequence = "\x1b[6~";
        break;
      case SCR_KEY_HOME:
        sequence = "\x1b[1~";
        break;
      case SCR_KEY_END:
        sequence = "\x1b[4~";
        break;
      case SCR_KEY_INSERT:
        sequence = "\x1b[2~";
        break;
      case SCR_KEY_DELETE:
        sequence = "\x1b[3~";
        break;
      case SCR_KEY_FUNCTION + 0:
        sequence = "\x1bOP";
        break;
      case SCR_KEY_FUNCTION + 1:
        sequence = "\x1bOQ";
        break;
      case SCR_KEY_FUNCTION + 2:
        sequence = "\x1bOR";
        break;
      case SCR_KEY_FUNCTION + 3:
        sequence = "\x1bOS";
        break;
      case SCR_KEY_FUNCTION + 4:
        sequence = "\x1b[15~";
        break;
      case SCR_KEY_FUNCTION + 5:
        sequence = "\x1b[17~";
        break;
      case SCR_KEY_FUNCTION + 6:
        sequence = "\x1b[18~";
        break;
      case SCR_KEY_FUNCTION + 7:
        sequence = "\x1b[19~";
        break;
      case SCR_KEY_FUNCTION + 8:
        sequence = "\x1b[20~";
        break;
      case SCR_KEY_FUNCTION + 9:
        sequence = "\x1b[21~";
        break;
      case SCR_KEY_FUNCTION + 10:
        sequence = "\x1b[23~";
        break;
      case SCR_KEY_FUNCTION + 11:
        sequence = "\x1b[24~";
        break;
      case SCR_KEY_FUNCTION + 12:
        sequence = "\x1b[25~";
        break;
      case SCR_KEY_FUNCTION + 13:
        sequence = "\x1b[26~";
        break;
      case SCR_KEY_FUNCTION + 14:
        sequence = "\x1b[28~";
        break;
      case SCR_KEY_FUNCTION + 15:
        sequence = "\x1b[29~";
        break;
      case SCR_KEY_FUNCTION + 16:
        sequence = "\x1b[31~";
        break;
      case SCR_KEY_FUNCTION + 17:
        sequence = "\x1b[32~";
        break;
      case SCR_KEY_FUNCTION + 18:
        sequence = "\x1b[33~";
        break;
      case SCR_KEY_FUNCTION + 19:
        sequence = "\x1b[34~";
        break;
      default:
        LogPrint(LOG_WARNING, "Key %4.4X not suported in ANSI mode.", key);
        return 0;
    }
    end = sequence + strlen(sequence);
  }

  while (sequence != end)
    if (!byteInserter(*sequence++))
      return 0;
  return 1;
}

static int
insertUtf8 (unsigned char byte) {
  if (byte & 0X80) {
    if (!insertByte(0XC0 | (byte >> 6))) return 0;
    byte &= 0XBF;
  }
  if (!insertByte(byte)) return 0;
  return 1;
}

static int
insert_LinuxScreen (ScreenKey key) {
  int ok = 0;
  LogPrint(LOG_DEBUG, "Insert key: %4.4X", key);
  if (rebindConsole()) {
    long mode;
    if (controlConsole(KDGKBMODE, &mode) != -1) {
      switch (mode) {
        case K_RAW:
          if (insertCode(key, 1)) ok = 1;
          break;
        case K_MEDIUMRAW:
          if (insertCode(key, 0)) ok = 1;
          break;
        case K_XLATE:
          if (insertMapped(key, &insertByte)) ok = 1;
          break;
        case K_UNICODE:
          if (insertMapped(key, &insertUtf8)) ok = 1;
          break;
        default:
          LogPrint(LOG_WARNING, "Unsupported keyboard mode: %ld", mode);
          break;
      }
    } else {
      LogError("ioctl KDGKBMODE");
    }
  }
  return ok;
}

static int
validateVt (int vt) {
  if ((vt >= 1) && (vt <= 0X3F)) return 1;
  LogPrint(LOG_DEBUG, "Virtual terminal %d is out of range.", vt);
  return 0;
}

static int
selectvt_LinuxScreen (int vt) {
  if (vt == virtualTerminal) return 1;
  if (vt && !validateVt(vt)) return 0;
  return openScreen(vt);
}

static int
switchvt_LinuxScreen (int vt) {
  if (validateVt(vt)) {
    if (selectvt_LinuxScreen(0)) {
      if (ioctl(consoleDescriptor, VT_ACTIVATE, vt) != -1) {
        LogPrint(LOG_DEBUG, "Switched to virtual tertminal %d.", vt);
        return 1;
      } else {
        LogError("ioctl VT_ACTIVATE");
      }
    }
  }
  return 0;
}

static int
currentvt_LinuxScreen (void) {
  ScreenDescription description;
  getConsoleDescription(&description);
  return description.no;
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

static const unsigned char *at2Keys;
static int at2Pressed;
#endif /* HAVE_LINUX_INPUT_H */

#ifdef HAVE_LINUX_UINPUT_H
#include <linux/uinput.h>

static int uinputDevice = -1;

static int
openUinputDevice (void) {
  if (uinputDevice != -1) return 1;

  if ((uinputDevice = openDevice("/dev/uinput", "uinput", O_WRONLY, 10, 223)) != -1) {
    struct uinput_user_dev device;
    
    memset(&device, 0, sizeof(device));
    strcpy(device.name, "brltty");
    if (write(uinputDevice, &device, sizeof(device)) == sizeof(device)) {
      ioctl(uinputDevice, UI_SET_EVBIT, EV_KEY);
      ioctl(uinputDevice, UI_SET_EVBIT, EV_REP);
      {
        int key;
        for (key=KEY_RESERVED; key<KEY_UNKNOWN; key++) {
          ioctl(uinputDevice, UI_SET_KEYBIT, key);
        }
      }

      if (ioctl(uinputDevice, UI_DEV_CREATE) != -1) {
        return 1;
      } else {
        LogError("ioctl UI_DEV_CREATE");
      }
    } else {
      LogError("uinput_user_dev write");
    }

    close(uinputDevice);
    uinputDevice = -1;
  }
  return 0;
}
#endif /* HAVE_LINUX_UINPUT_H */

static int
execute_LinuxScreen (int command) {
  int blk = command & BRL_MSK_BLK;
  int arg
#ifdef HAVE_ATTRIBUTE_UNUSED
      __attribute__((unused))
#endif /* HAVE_ATTRIBUTE_UNUSED */
      = command & BRL_MSK_ARG;

  switch (blk) {
    case BRL_BLK_PASSAT2:
#ifdef HAVE_LINUX_INPUT_H
      if (arg == 0XF0) {
        at2Pressed = 0;
      } else if (arg == 0XE0) {
        at2Keys = at2KeysE0;
      } else {
        unsigned char key = at2Keys[arg];
        int pressed
#ifdef HAVE_ATTRIBUTE_UNUSED
            __attribute__((unused))
#endif /* HAVE_ATTRIBUTE_UNUSED */
            = at2Pressed;

        at2Keys = at2KeysOriginal;
        at2Pressed = 1;

        if (key) {
#ifdef HAVE_LINUX_UINPUT_H
          if (openUinputDevice()) {
            struct input_event event;
            event.type = EV_KEY;
            event.code = key;
            event.value = pressed;
            write(uinputDevice, &event, sizeof(event));
            return 1;
          }
#endif /* HAVE_LINUX_UINPUT_H */
        }
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
  main->base.read = read_LinuxScreen;
  main->base.insert = insert_LinuxScreen;
  main->base.selectvt = selectvt_LinuxScreen;
  main->base.switchvt = switchvt_LinuxScreen;
  main->base.currentvt = currentvt_LinuxScreen;
  main->base.execute = execute_LinuxScreen;
  main->parameters = parameters_LinuxScreen;
  main->prepare = prepare_LinuxScreen;
  main->open = open_LinuxScreen;
  main->setup = setup_LinuxScreen;
  main->close = close_LinuxScreen;

#ifdef HAVE_LINUX_INPUT_H
  at2Keys = at2KeysOriginal;
  at2Pressed = 1;
#endif /* HAVE_LINUX_INPUT_H */
}

/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/* scr_vcsa.cc - screen library for use with Linux vcsa devices.
 *
 * Note: Although C++, this code requires no standard C++ library.
 * This is important as BRLTTY *must not* rely on too many
 * run-time shared libraries, nor be a huge executable.
 */

#define SCR_C 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <errno.h>
#include <linux/kd.h>

#include "misc.h"

#include "scr.h"
#include "scrdev.h"
#include "scr_vcsa.h"
#include "inskey.h"  /* for definition of CONSOLE */



/* Instanciation of the vcsa driver here */
static vcsa_Screen vcsa;

/* Pointer for external reference */
RealScreen *live = &vcsa;

int vcsa_Screen::set_screen_translation_tables (int force) 
/* 
 * The virtual console devices return the actual font value (font position)
 * used on the screen. The problem is that the font may not have been designed
 * for the character set being used. Most PC video cards have a built-in
 * font defined for the CP437 character set, but Linux usually operates with
 * the ISO-Latin-1 character set. So the kernel translates characters to be
 * printed on the screen from the charset being used to the charset the screen
 * font was designed for. We need to do the reverse translation to get the
 * character code in the expected charset. Not all
 * displays use CP437 and not all users use ISO-8859-1, so we wil query the
 * kernel for the character to font mapping and then reverse it.
 */
{
#if (E_TABSZ != 256)
  #error "E_TABSZ is not 256"
#endif
  unsigned char kernel_table[E_TABSZ];

  if (ioctl(cons_fd, GIO_SCRNMAP, kernel_table) == -1) {
    LogError("ioctl GIO_SCRNMAP");
    return 0;
  }

  if (force || (memcmp(kernel_table, output_table, 0X100) != 0)) {
    memcpy(output_table, kernel_table, 0X100);
    reverseTable(output_table, input_table);
    LogPrint(LOG_DEBUG, "Font changed.");
  }
  return 1;
}


int vcsa_Screen::open (int for_csr_routing)
{
  static char *screenDevice = NULL;
  static char *screenDevices = NULL;
  if (!screenDevices) {
    screenDevices = strdupWrapper(VCSADEV);
    LogPrint(LOG_DEBUG, "Screen device list: %s", screenDevices);
  }
  if (!screenDevice) {
    const char *delimiters = " ";
    char *devices = screenDevices;
    char *device;
    int exists = 0;
    while ((device = strtok(devices, delimiters))) {
      device = strdupWrapper(device);
      LogPrint(LOG_DEBUG, "Checking screen device '%s'.", device);
      if (access(device, R_OK|W_OK) != -1) {
	if (screenDevice) free(screenDevice);
        screenDevice = device;
	break;
      }
      LogPrint(LOG_DEBUG, "Cannot access screen device '%s': %s",
	       device, strerror(errno));
      if (errno != ENOENT) {
        if (!exists) {
	  exists = 1;
	  if (screenDevice) {
	    free(screenDevice);
	    screenDevice = NULL;
	  }
	}
      }
      if (screenDevice)
	free(device);
      else
	screenDevice = device;
      devices = NULL;
    }
  }
  if (screenDevice) {
    if ((fd = ::open(screenDevice, O_RDWR)) != -1) {
      if ((cons_fd = ::open(CONSOLE, O_RDWR|O_NOCTTY)) != -1) {
	if (for_csr_routing) return 0;
	LogPrint(LOG_INFO, "Screen Device: %s", screenDevice);
	if (set_screen_translation_tables(1)) return 0;
	::close(cons_fd);
      } else {
	LogPrint(LOG_ERR, "Cannot open console device '%s': %s",
		 CONSOLE, strerror(errno));
      }
      ::close(fd);
    } else {
      LogPrint(LOG_ERR, "Cannot open screen device '%s': %s",
	       screenDevice, strerror(errno));
    }
  } else {
    LogPrint(LOG_ERR, "Screen device not specified.");
  }
  return 1;
}


void vcsa_Screen::getstat (scrstat & stat)
{
  unsigned char buffer[4];
  struct vt_stat vtstat;

  /* Periodically recalculate font mapping. I don't know any way to be
   * notified when it changes, and the recalculation is not too
   * long/difficult.
   */
  {
    static int timer = 0;
    if (++timer > 100) {
      set_screen_translation_tables(0);
      timer = 0;
    }
  }

  lseek (fd, 0, SEEK_SET);	/* go to start of file */
  read (fd, buffer, 4);		/* get screen status bytes */
  stat.rows = buffer[0];
  stat.cols = buffer[1];
  stat.posx = buffer[2];
  stat.posy = buffer[3];
  ioctl (cons_fd, VT_GETSTATE, &vtstat);
  stat.no = vtstat.v_active;
}


unsigned char * 
vcsa_Screen::getscr (winpos pos, unsigned char *buffer, short mode)
{
  /* screen statistics */
  scrstat stat;
  getstat (stat);
  if (pos.left < 0 || pos.top < 0 || pos.width < 1 || pos.height < 1 \
      ||mode < 0 || mode > 1 || pos.left + pos.width > stat.cols \
      ||pos.top + pos.height > stat.rows)
    return NULL;
  /* start offset */
  off_t start = 4 + (pos.top * stat.cols + pos.left) * 2 + mode;
  /* number of bytes to read for one complete line */
  size_t linelen = 2 * pos.width - 1;
  /* line buffer */
  unsigned char linebuf[linelen];
  /* pointer to copy data to */
  unsigned char *dst = buffer;
  /* pointer to copy data from */
  unsigned char *src;
  /* indexes */
  int i, j;

  for (i = 0; i < pos.height; i++)
    {
      lseek (fd, start + i * stat.cols * 2, SEEK_SET);
      read (fd, linebuf, linelen);
      src = linebuf;
      if (mode == SCR_TEXT)
	for (j = 0; j < pos.width; j++) {
	  /* Conversion from font position to character code */
	  *dst++ = input_table[*src];
	  src += 2;
	}
      else
	for (j = 0; j < pos.width; j++) {
	  *dst++ = *src;
	  src += 2;
	}
    }
  return buffer;
}


void vcsa_Screen::close (void)
{
  ::close (fd);
  ::close (cons_fd);
}


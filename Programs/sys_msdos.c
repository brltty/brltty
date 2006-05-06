/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

#include "misc.h"
#include "system.h"

#include "sys_prog_none.h"

#include "sys_boot_none.h"

#include "sys_exec_none.h"

#include "sys_shlib_none.h"

#include "sys_beep_none.h"

#ifdef ENABLE_PCM_SUPPORT
#include "sys_pcm_none.h"
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
#include "sys_midi_none.h"
#endif /* ENABLE_MIDI_SUPPORT */

#include "sys_ports_always.h"
#include "sys_ports_x86.h"

#include <stdarg.h>
#include <string.h>

int
vsnprintf (char *str, size_t size, const char *format, va_list ap) {
  size_t alloc = 1024;
  char *buf;
  int ret;
  if (alloc < size)
    alloc = size;
  buf = alloca(alloc);
  ret = vsprintf(buf, format, ap);
  if (size > ret + 1)
    size = ret + 1;
  memcpy(str, buf, size);
  return ret;
}

int
snprintf (char *str, size_t size, const char *format, ...) {
  va_list argp;
  int ret;
  va_start(argp, format);
  ret = vsnprintf(str, size, format, argp);
  va_end(argp);
  return ret;
}

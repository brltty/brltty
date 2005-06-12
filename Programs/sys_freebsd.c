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

#include "prologue.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "misc.h"
#include "system.h"

#include "sys_prog_none.h"

#include "sys_boot_none.h"

#define SHARED_OBJECT_LOAD_FLAGS (RTLD_NOW | RTLD_GLOBAL)
#include "sys_shlib_dlfcn.h"

#include "sys_beep_spkr.h"

#ifdef ENABLE_PCM_SUPPORT
#define PCM_OSS_DEVICE_PATH "/dev/dsp"
#include "sys_pcm_oss.h"
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
#include "sys_midi_none.h"
#endif /* ENABLE_MIDI_SUPPORT */

#include "sys_ports_none.h"

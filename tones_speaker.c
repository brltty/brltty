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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "misc.h"
#include "tones.h"

static int fileDescriptor = -1;

static int openSpeaker (void) {
   if (fileDescriptor == -1) {
      if ((fileDescriptor = open("/dev/console", O_WRONLY)) == -1) {
         return 0;
      }
      setCloseOnExec(fileDescriptor);
   }
   return 1;
}

static int generateSpeaker (int frequency, int duration) {
   if (fileDescriptor != -1) {
      if (!frequency) {
         shortdelay(duration);
	 return 1;
      }
      if (ioctl(fileDescriptor, KIOCSOUND, 1190000/frequency) != -1) {
         shortdelay(duration);
	 if (ioctl(fileDescriptor, KDMKTONE, 0) != -1) {
	    return 1;
	 }
      }
   }
   return 0;
}

static void closeSpeaker (void) {
   if (fileDescriptor != -1) {
      close(fileDescriptor);
      fileDescriptor = -1;
   }
}

static ToneGenerator toneGenerator = {
   openSpeaker,
   generateSpeaker,
   closeSpeaker
};

ToneGenerator *toneSpeaker (void) {
   return &toneGenerator;
}

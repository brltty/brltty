/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "misc.h"
#include "tones.h"

static int fileDescriptor = -1;

static int openSpeaker (void) {
   if (fileDescriptor == -1) {
      char *device = "/dev/console";
      if ((fileDescriptor = open(device, O_WRONLY)) == -1) {
         LogPrint(LOG_ERR, "Cannot open speaker: %s: %s", device, strerror(errno));
         return 0;
      }
      setCloseOnExec(fileDescriptor);
      LogPrint(LOG_DEBUG, "Speaker opened: fd=%d", fileDescriptor);
   }
   return 1;
}

static int generateSpeaker (int note, int duration) {
   if (fileDescriptor != -1) {
      LogPrint(LOG_DEBUG, "Tone: msec=%d note=%d",
               duration, note);
      if (!note) {
         shortdelay(duration);
	 return 1;
      }
      if (ioctl(fileDescriptor, KIOCSOUND, (int)(1193180.0/noteFrequencies[note])) != -1) {
         shortdelay(duration);
	 if (ioctl(fileDescriptor, KDMKTONE, 0) != -1) {
	    return 1;
	 } else {
	    LogPrint(LOG_ERR, "Cannot stop speaker: %s", strerror(errno));
	 }
      } else {
	 LogPrint(LOG_ERR, "Cannot start speaker: %s", strerror(errno));
      }
   }
   return 0;
}

static int flushSpeaker (void) {
   return 1;
}

static void closeSpeaker (void) {
   if (fileDescriptor != -1) {
      close(fileDescriptor);
      LogPrint(LOG_DEBUG, "Speaker closed.");
      fileDescriptor = -1;
   }
}

static ToneGenerator toneGenerator = {
   openSpeaker,
   generateSpeaker,
   flushSpeaker,
   closeSpeaker
};

ToneGenerator *toneSpeaker (void) {
   return &toneGenerator;
}

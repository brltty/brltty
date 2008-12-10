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
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* ExternalSpeech/speech.h */

/* The following are the default parameters that will be used if no parameter
   is specified on the command-line (-p) or in the brltty.conf file
   (speech-driverparm option). */

/* Specify the path of the external program that will handle speech. */
#define HELPER_PROG_PATH "/usr/local/bin/externalspeech"

#define UID 65534
#define GID 65534
/* We setuid/setgid before exec'ing the external program. */

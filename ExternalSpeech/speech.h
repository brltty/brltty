/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team, All rights reserved.
 *
 * Web Page: http://www.cam.org/~nico/brltty
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 */

/* ExternalSpeech/speech.h */

#define SPKNAME "ExternalSpeech"

/* The following are the default parameters that will be used if no parameter
   is specified on the command-line (-p) or in the brltty.conf file
   (speech-driverparm option). */

/* Specify the path of the external program that will handle speech. */
#define HELPER_PROG_PATH "/usr/local/bin/externalspeech"

#define UID 65534
#define GID 65534
/* We setuid/setgid before exec'ing the external program. */

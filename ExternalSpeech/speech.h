/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
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

/* Specify the path of the external program that will handle speech.
   This setting can be overridden through the -p command-line parameter
   or speech-driverparm configuration option. */
#define HELPER_PROG_PATH "/usr/local/bin/externalspeech"

/*
 * BrlTty - A daemon providing access to the Linux console (when in text
 *          mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BrlTty Team. All rights reserved.
 *
 * BrlTty comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* this header file is used to create the driver structure */
/* for a dynamically loadable speech driver. */
/* SPKNAME must be defined - see spkconf.h */

/* Routines provided by this speech driver. */
static void identspk (void);/* print start-up messages */
static void initspk (char *parm); /* initialize speech device */
static void say (unsigned char *buffer, int len); /* speak text */
#ifdef SPK_HAVE_SAYATTRIBS
static void sayWithAttribs (unsigned char *buffer, int len); /* speak text */
#endif
static void mutespk (void); /* mute speech */
#ifdef SPK_HAVE_TRACK
static void processSpkTracking (void); /* Get current speaking position */
static int trackspk (void); /* Get current speaking position */
static int isSpeaking (void);
#else
static void processSpkTracking (void) { }
static int trackspk () { return 0; }
static int isSpeaking () { return 0; }
#endif
static void closespk (void); /* close speech device */

speech_driver spk_driver = 
{
  SPKNAME,
  SPKDRIVER,
  identspk,
  initspk,
  say,
#ifdef SPK_HAVE_SAYATTRIBS
  sayWithAttribs,
#else
  0,
#endif
  mutespk,
  processSpkTracking,
  trackspk,
  isSpeaking,
  closespk
};

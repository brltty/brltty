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

/* speech.h - Header file for the speech library
 */

typedef struct {
   char *name;
   char *identifier;
   char **parameters;
   void (*identify) (void);		/* print start-up messages */
   void (*initialize) (char **parameters);		/* initialize speech device */
   void (*say) (unsigned char *buffer, int len);	/* speak text */
   void (*sayWithAttribs) (unsigned char *buffer, int len);	/* speak text */
   void (*mute) (void);		/* mute speech */
   void (*processSpkTracking) (void);		/* get current speaking position */
   int (*track) (void);		/* get current speaking position */
   int (*isSpeaking) (void);		/* get current speaking position */
   void (*close) (void);		/* close speech device */
} speech_driver;

extern speech_driver *speech;
extern char *speech_libraryName;	/* name of library */
int load_speech_driver(void);
int list_speech_drivers(void);

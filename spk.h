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
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/* speech.h - Header file for the speech library
 */

typedef struct {
   char *name;
   char *identifier;
   void (*identify) (void);		/* print start-up messages */
   void (*initialize) (char *parm);		/* initialize speech device */
   void (*say) (unsigned char *buffer, int len);	/* speak text */
   void (*sayWithAttribs) (unsigned char *buffer, int len);	/* speak text */
   void (*mute) (void);		/* mute speech */
   void (*processSpkTracking) (void);		/* get current speaking position */
   int (*track) (void);		/* get current speaking position */
   int (*isSpeaking) (void);		/* get current speaking position */
   void (*close) (void);		/* close speech device */
} speech_driver;

extern speech_driver *speech;
extern char* speech_libname;	/* name of library */
extern char* speech_drvparm;	/* arbitraty init parameter */
int load_speech_driver(void);
int list_speech_drivers(void);

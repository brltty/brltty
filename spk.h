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

#ifndef _SPK_H
#define _SPK_H

/* spk.h - Header file for the speech library
 */

typedef struct {
  const char *name;
  const char *identifier;
  const char *const *parameters;
  void (*identify) (void);		/* print start-up messages */
  void (*initialize) (char **parameters);		/* initialize speech device */
  void (*say) (const unsigned char *buffer, int len);	/* speak text */
  void (*mute) (void);		/* mute speech */
  void (*close) (void);		/* close speech device */
  void (*express) (const unsigned char *buffer, int len);	/* speak text */
  void (*doTrack) (void);		/* get current speaking position */
  int (*getTrack) (void);		/* get current speaking position */
  int (*isSpeaking) (void);		/* get current speaking position */
} SpeechDriver;

extern const SpeechDriver *loadSpeechDriver (const char **libraryName);
extern int listSpeechDrivers (void);
extern const SpeechDriver *speech;
extern const SpeechDriver noSpeech;

#endif /* !defined(_SPK_H) */

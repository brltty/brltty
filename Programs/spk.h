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

#ifndef BRLTTY_INCLUDED_SPK
#define BRLTTY_INCLUDED_SPK

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* spk.h - Header file for the speech library
 */

typedef struct {
  const char *name;
  const char *code;
  const char *comment;
  const char *date;
  const char *time;
  const char *const *parameters;
  void (*identify) (void);
  int (*open) (char **parameters);
  void (*close) (void);
  void (*say) (const unsigned char *buffer, int len);
  void (*mute) (void);

  /* These require SPK_HAVE_EXPRESS. */
  void (*express) (const unsigned char *buffer, int len);

  /* These require SPK_HAVE_TRACK. */
  void (*doTrack) (void);
  int (*getTrack) (void);
  int (*isSpeaking) (void);

  /* These require SPK_HAVE_RATE. */
  void (*rate) (float setting);

  /* These require SPK_HAVE_VOLUME. */
  void (*volume) (float setting);
} SpeechDriver;

extern const SpeechDriver *loadSpeechDriver (const char *code, void **driverObject, const char *driverDirectory);
extern void identifySpeechDrivers (void);
extern const SpeechDriver *speech;
extern const SpeechDriver noSpeech;

extern void sayString (const char *string);

extern void setSpeechRate (int setting, int say);
#define SPK_DEFAULT_RATE 10
#define SPK_MAXIMUM_RATE (SPK_DEFAULT_RATE * 2)

extern void setSpeechVolume (int setting, int say);
#define SPK_DEFAULT_VOLUME 10
#define SPK_MAXIMUM_VOLUME (SPK_DEFAULT_VOLUME * 2)

extern int openSpeechFifo (const char *directory, const char *path);
extern void processSpeechFifo (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SPK */

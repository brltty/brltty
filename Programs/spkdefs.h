/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_SPKDEFS
#define BRLTTY_INCLUDED_SPKDEFS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SPK_VOLUME_DEFAULT 10
#define SPK_VOLUME_MAXIMUM (SPK_VOLUME_DEFAULT * 2)

#define SPK_RATE_DEFAULT 10
#define SPK_RATE_MAXIMUM (SPK_RATE_DEFAULT * 2)

#define SPK_PITCH_DEFAULT 10
#define SPK_PITCH_MAXIMUM (SPK_PITCH_DEFAULT * 2)

typedef enum {
  SPK_PUNCTUATION_NONE,
  SPK_PUNCTUATION_SOME,
  SPK_PUNCTUATION_ALL
} SpeechPunctuation;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SPKDEFS */

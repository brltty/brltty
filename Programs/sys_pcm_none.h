/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#warning sound card support not available on this platform

int
getPcmDevice (int errorLevel) {
  LogPrint(errorLevel, "PCM device not supported.");
  return -1;
}

int
getPcmBlockSize (int descriptor) {
  return 0X100;
}

int
getPcmSampleRate (int descriptor) {
  return 8000;
}

int
setPcmSampleRate (int descriptor, int rate) {
  return getPcmSampleRate(descriptor);
}

int
getPcmChannelCount (int descriptor) {
  return 1;
}

int
setPcmChannelCount (int descriptor, int channels) {
  return getPcmChannelCount(descriptor);
}

PcmAmplitudeFormat
getPcmAmplitudeFormat (int descriptor) {
  return PCM_FMT_UNKNOWN;
}

PcmAmplitudeFormat
setPcmAmplitudeFormat (int descriptor, PcmAmplitudeFormat format) {
  return getPcmAmplitudeFormat(descriptor);
}

void
forcePcmOutput (int descriptor) {
}

void
awaitPcmOutput (int descriptor) {
}

void
cancelPcmOutput (int descriptor) {
}

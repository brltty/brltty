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

package org.a11y.brltty.android;

import android.util.Log;

import android.media.AudioManager;
import android.media.AudioFormat;
import android.media.AudioTrack;

public final class PcmDevice {
  private static final String LOG_TAG = PcmDevice.class.getName();

  private static final int streamType = AudioManager.STREAM_NOTIFICATION;
  private static final int trackMode = AudioTrack.MODE_STREAM;
  private static final int sampleRate = 8000;
  private static final int channelConfiguration = AudioFormat.CHANNEL_OUT_MONO;
  private static final int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
  private static final int bufferSize = 0X2000;

  private AudioTrack audioTrack = null;

  private AudioTrack newAudioTrack () {
    return new AudioTrack(streamType, sampleRate, channelConfiguration,
                          audioFormat, bufferSize, trackMode);
  }

  public int getSampleRate () {
    return sampleRate;
  }

  public int getChannelCount () {
    switch (channelConfiguration) {
      default:
      case AudioFormat.CHANNEL_OUT_MONO:
        return 1;

      case AudioFormat.CHANNEL_OUT_STEREO:
        return 2;
    }
  }

  public int getBufferSize () {
    return bufferSize;
  }

  public boolean write (short[] samples) {
    if (audioTrack == null) audioTrack = newAudioTrack();

    int result = audioTrack.write(samples, 0, samples.length);
    audioTrack.play();
    return result >= 0;
  }

  public void cancel () {
    audioTrack.pause();
    audioTrack.flush();
  }

  public void close () {
    cancel();
    audioTrack.release();
    audioTrack = null;
  }

  public PcmDevice () {
  }
}

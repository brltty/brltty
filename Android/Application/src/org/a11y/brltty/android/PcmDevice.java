/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
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
  private static final int audioFormat = AudioFormat.ENCODING_PCM_16BIT;

  private int sampleRate = 8000;
  private int channelConfiguration = AudioFormat.CHANNEL_OUT_MONO;
  private AudioTrack audioTrack = null;

  public boolean isStarted () {
    return audioTrack != null;
  }

  public int getSampleRate () {
    return sampleRate;
  }

  public void setSampleRate (int rate) {
    if (!isStarted()) sampleRate = rate;
  }

  public int getChannelConfiguration () {
    return channelConfiguration;
  }

  public int getChannelCount () {
    switch (getChannelConfiguration()) {
      default:
      case AudioFormat.CHANNEL_OUT_MONO:
        return 1;

      case AudioFormat.CHANNEL_OUT_STEREO:
        return 2;
    }
  }

  public void setChannelCount (int count) {
    if (!isStarted()) {
      switch (count) {
        default:
        case 1:
          channelConfiguration = AudioFormat.CHANNEL_OUT_MONO;
          break;

        case 2:
          channelConfiguration = AudioFormat.CHANNEL_OUT_STEREO;
          break;
      }
    }
  }

  public int getBufferSize () {
    return AudioTrack.getMinBufferSize(getSampleRate(), getChannelConfiguration(), audioFormat);
  }

  private AudioTrack newAudioTrack () {
    return new AudioTrack(streamType, getSampleRate(), getChannelConfiguration(),
                          audioFormat, getBufferSize(), trackMode);
  }

  protected void start () {
    if (!isStarted()) audioTrack = newAudioTrack();
  }

  public boolean write (short[] samples) {
    start();

    int offset = 0;
    int length = samples.length;

    while (length > 0) {
      int result = audioTrack.write(samples, offset, length);
      if (result < 1) return false;

      audioTrack.play();
      offset += result;
      length -= result;
    }

    return true;
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

/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2019 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brltty.android;

import android.util.Log;

import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.AudioFormat;
import android.media.AudioAttributes;

public final class PcmDevice {
  private final static String LOG_TAG = PcmDevice.class.getName();

  private final static int streamType = AudioManager.STREAM_NOTIFICATION;
  private final static int trackMode = AudioTrack.MODE_STREAM;
  private final static int audioEncoding = AudioFormat.ENCODING_PCM_16BIT;

  private int sampleRate = 8000;
  private int channelCount = 1;
  private AudioTrack audioTrack = null;

  public final boolean isStarted () {
    return audioTrack != null;
  }

  public final boolean isPlaying () {
    return audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING;
  }

  public final int getSampleRate () {
    return sampleRate;
  }

  public final void setSampleRate (int rate) {
    if (!isStarted()) sampleRate = rate;
  }

  public final int getChannelPositionMask () {
    switch (channelCount) {
      case 1:
        return AudioFormat.CHANNEL_OUT_MONO;

      case 2:
        return AudioFormat.CHANNEL_OUT_STEREO;

      case 3:
        return AudioFormat.CHANNEL_OUT_STEREO | AudioFormat.CHANNEL_OUT_FRONT_CENTER;

      case 4:
        return AudioFormat.CHANNEL_OUT_QUAD;

      case 5:
        return AudioFormat.CHANNEL_OUT_QUAD | AudioFormat.CHANNEL_OUT_FRONT_CENTER;

      case 6:
        return AudioFormat.CHANNEL_OUT_5POINT1;

      case 7:
        return AudioFormat.CHANNEL_OUT_5POINT1 | AudioFormat.CHANNEL_OUT_BACK_CENTER;

      case 8:
        return AudioFormat.CHANNEL_OUT_7POINT1_SURROUND;

      default:
        return 0;
    }
  }

  public final int getChannelCount () {
    return channelCount;
  }

  public final void setChannelCount (int count) {
    if (!isStarted()) {
      channelCount = count;
    }
  }

  public final int getBufferSize () {
    return AudioTrack.getMinBufferSize(getSampleRate(),
                                       getChannelPositionMask(),
                                       audioEncoding);
  }

  private final AudioTrack newAudioTrack () {
    if (ApplicationUtilities.haveOreo) {
      AudioAttributes attributes =
        new AudioAttributes.Builder()
                           .setUsage(AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY)
                           .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                           .setFlags(AudioAttributes.FLAG_AUDIBILITY_ENFORCED)
                           .build();

      AudioFormat format =
        new AudioFormat.Builder()
                       .setEncoding(audioEncoding)
                       .setSampleRate(getSampleRate())
                       .setChannelMask(getChannelPositionMask())
                       .build();

      return new AudioTrack.Builder()
                           .setAudioAttributes(attributes)
                           .setAudioFormat(format)
                           .setBufferSizeInBytes(getBufferSize())
                           .setTransferMode(AudioTrack.MODE_STREAM)
                           .setPerformanceMode(AudioTrack.PERFORMANCE_MODE_LOW_LATENCY)
                           .build();
    }

    return new AudioTrack(streamType, getSampleRate(),
                          getChannelPositionMask(), audioEncoding,
                          getBufferSize(), trackMode);
  }

  protected final void start () {
    if (!isStarted()) audioTrack = newAudioTrack();
  }

  public final boolean write (short[] samples) {
    start();

    int offset = 0;
    int length = samples.length;

    while (length > 0) {
      int count = audioTrack.write(samples, offset, length);
      if (count < 1) return false;

      offset += count;
      length -= count;

      if (!isPlaying()) audioTrack.play();
    }

    return true;
  }

  public final void push () {
    if (isPlaying()) audioTrack.stop();
  }

  public final void cancel () {
    if (isPlaying()) audioTrack.pause();
    audioTrack.flush();
  }

  public final void close () {
    cancel();
    audioTrack.release();
    audioTrack = null;
  }

  public PcmDevice () {
  }
}

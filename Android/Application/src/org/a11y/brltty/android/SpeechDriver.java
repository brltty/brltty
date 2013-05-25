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

import java.util.HashMap;

import android.speech.tts.TextToSpeech;

public final class SpeechDriver {
  enum State {
    STOPPED,
    STARTED,
    FAILED
  }

  private static volatile State state = State.STOPPED;
  private static TextToSpeech tts = null;
  private static final HashMap<String, String> parameters = new HashMap<String, String>();

  public static boolean start () {
    TextToSpeech.OnInitListener listener = new TextToSpeech.OnInitListener() {
      @Override
      public void onInit (int status) {
        state = (status == TextToSpeech.SUCCESS)? State.STARTED: State.FAILED;
      }
    };

    state = State.STOPPED;
    tts = new TextToSpeech(BrailleService.getBrailleService(), listener);
    parameters.clear();
    return true;
  }

  public static void stop () {
    if (tts != null) {
      tts.shutdown();
      tts = null;
      state = State.STOPPED;
    }
  }

  public static boolean say (String text) {
    return tts.speak(text, TextToSpeech.QUEUE_ADD, parameters) == TextToSpeech.SUCCESS;
  }

  public static boolean mute () {
    return tts.stop() == TextToSpeech.SUCCESS;
  }

  public static boolean setVolume (float volume) {
    parameters.put(TextToSpeech.Engine.KEY_PARAM_VOLUME, Float.toString(volume));
    return true;
  }

  public static boolean setRate (float rate) {
    return tts.setSpeechRate(rate) == TextToSpeech.SUCCESS;
  }

  public static boolean setPitch (float pitch) {
    return tts.setPitch(pitch) == TextToSpeech.SUCCESS;
  }

  private SpeechDriver () {
  }
}

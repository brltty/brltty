/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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

package org.a11y.brltty.android.speech;
import org.a11y.brltty.android.*;

import android.util.Log;
import android.speech.tts.TextToSpeech;
import android.os.Bundle;

public class NewSpeechParadigm extends SpeechParadigm {
  private final static String LOG_TAG = NewSpeechParadigm.class.getName();

  public NewSpeechParadigm () {
    super();
  }

  private final Bundle parameters = new Bundle();

  @Override
  public final NewSpeechParadigm resetParameters () {
    synchronized (parameters) {
      parameters.clear();
    }

    return this;
  }

  @Override
  public final NewSpeechParadigm resetParameter (String key) {
    synchronized (parameters) {
      parameters.remove(key);
    }

    return this;
  }

  @Override
  public final NewSpeechParadigm setParameter (String key, String value) {
    synchronized (parameters) {
      parameters.putString(key, value);
    }

    return this;
  }

  @Override
  public final NewSpeechParadigm setParameter (String key, int value) {
    synchronized (parameters) {
      parameters.putInt(key, value);
    }

    return this;
  }

  @Override
  public final NewSpeechParadigm setParameter (String key, float value) {
    synchronized (parameters) {
      parameters.putFloat(key, value);
    }

    return this;
  }

  @Override
  public final String getParameter (String key, String defaultValue) {
    synchronized (parameters) {
      return parameters.getString(key, defaultValue);
    }
  }

  @Override
  public final int getParameter (String key, int defaultValue) {
    synchronized (parameters) {
      return parameters.getInt(key, defaultValue);
    }
  }

  @Override
  public final float getParameter (String key, float defaultValue) {
    synchronized (parameters) {
      return parameters.getFloat(key, defaultValue);
    }
  }

  @Override
  public final boolean sayText (TextToSpeech tts, CharSequence text, int queueMode) {
    int result = tts.speak(text, queueMode, parameters, getUtteranceIdentifier());
    if (result == TextToSpeech.SUCCESS) return true;

    logSpeechError("speak", result);
    return false;
  }
}

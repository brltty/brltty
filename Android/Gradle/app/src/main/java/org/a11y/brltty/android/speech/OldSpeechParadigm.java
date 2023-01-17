/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
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
import java.util.HashMap;

public class OldSpeechParadigm extends SpeechParadigm {
  private final static String LOG_TAG = OldSpeechParadigm.class.getName();

  public OldSpeechParadigm () {
    super();
  }

  private final HashMap<String, String> parameters = new HashMap<>();

  @Override
  public final OldSpeechParadigm resetParameters () {
    synchronized (parameters) {
      parameters.clear();
    }

    return this;
  }

  @Override
  public final OldSpeechParadigm resetParameter (String key) {
    synchronized (parameters) {
      parameters.remove(key);
    }

    return this;
  }

  @Override
  public final OldSpeechParadigm setParameter (String key, String value) {
    synchronized (parameters) {
      parameters.put(key, value);
    }

    return this;
  }

  @Override
  public final OldSpeechParadigm setParameter (String key, int value) {
    return setParameter(key, Integer.toString(value));
  }

  @Override
  public final OldSpeechParadigm setParameter (String key, float value) {
    return setParameter(key, Float.toString(value));
  }

  private final String getParameter (String key) {
    synchronized (parameters) {
      return parameters.get(key);
    }
  }

  @Override
  public final String getParameter (String key, String defaultValue) {
    String string = getParameter(key);
    if (string == null) return defaultValue;
    return string;
  }

  @Override
  public final int getParameter (String key, int defaultValue) {
    String string = getParameter(key);
    if (string == null) return defaultValue;
    return Integer.valueOf(string);
  }

  @Override
  public final float getParameter (String key, float defaultValue) {
    String string = getParameter(key);
    if (string == null) return defaultValue;
    return Float.valueOf(string);
  }

  @Override
  public final boolean sayText (TextToSpeech tts, CharSequence text, int queueMode) {
    int result = tts.speak((String)text, queueMode, parameters);
    if (result == TextToSpeech.SUCCESS) return true;

    logSpeechError("speak", result);
    return false;
  }
}

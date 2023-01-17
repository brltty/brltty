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

public abstract class SpeechParadigm extends SpeechComponent {
  private final static String LOG_TAG = SpeechParadigm.class.getName();

  protected SpeechParadigm () {
    super();
  }

  public abstract SpeechParadigm resetParameters ();
  public abstract SpeechParadigm resetParameter (String key);

  public abstract SpeechParadigm setParameter (String key, String value);
  public abstract SpeechParadigm setParameter (String key, int value);
  public abstract SpeechParadigm setParameter (String key, float value);

  public abstract String getParameter (String key, String defaultValue);
  public abstract int getParameter (String key, int defaultValue);
  public abstract float getParameter (String key, float defaultValue);

  public final String getUtteranceIdentifier () {
    return getParameter(TextToSpeech.Engine.KEY_PARAM_UTTERANCE_ID, null);
  }

  private final static Object UTTERANCE_IDENTIFIER_LOCK = new Object();
  private static int utteranceIdentifier = 0;

  public final String newUtteranceIdentifier () {
    synchronized (UTTERANCE_IDENTIFIER_LOCK) {
      return "ut#" + ++utteranceIdentifier;
    }
  }

  public final String setUtteranceIdentifier () {
    String identifier = newUtteranceIdentifier();
    setParameter(TextToSpeech.Engine.KEY_PARAM_UTTERANCE_ID, identifier);
    return identifier;
  }

  public abstract boolean sayText (TextToSpeech tts, CharSequence text, int queueMode);
}

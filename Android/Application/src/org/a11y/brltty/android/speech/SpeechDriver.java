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

import java.util.Map;
import java.util.HashMap;

import android.util.Log;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;

public class SpeechDriver extends SpeechComponent {
  private final static String LOG_TAG = SpeechDriver.class.getName();

  protected SpeechDriver () {
    super();
  }

  enum EngineState {
    STOPPED,
    STARTED,
    FAILED
  }

  private final static int maximumTextLength = (
    APITests.haveJellyBeanMR2?  TextToSpeech.getMaxSpeechInputLength():
    4000
  ) - 1;

  private final static SpeechParadigm ttsParadigm =
    APITests.haveLollipop? new NewSpeechParadigm():
    new OldSpeechParadigm();

  private static volatile EngineState engineState = EngineState.STOPPED;
  private static TextToSpeech ttsObject = null;

  private native static void tellLocation (long synthesizer, int location);
  private native static void tellFinished (long synthesizer);

  private static class Utterance {
    private final long speechSynthesizer;
    private final CharSequence utteranceText;

    public Utterance (long synthesizer, CharSequence text) {
      speechSynthesizer = synthesizer;
      utteranceText = text;
    }

    public final long getSynthesizer () {
      return speechSynthesizer;
    }

    public final CharSequence getText () {
      return utteranceText;
    }

    private int rangeStart = 0;
    private int rangeEnd = 0;

    public final int getRangeStart () {
      return rangeStart;
    }

    public final int getRangeEnd () {
      return rangeEnd;
    }

    public final void setRange (int start, int end) {
      rangeStart = start;
      rangeEnd = end;
      tellLocation(getSynthesizer(), start);
    }
  }

  private final static Map<CharSequence, Utterance> activeUtterances = new HashMap<>();

  private static Utterance getUtterance (String identifier) {
    synchronized (activeUtterances) {
      return activeUtterances.get(identifier);
    }
  }

  private static void logUtteranceState (String identifier, String state, boolean unexpected) {
    StringBuilder log = new StringBuilder();
    log.append("utterance ").append(state);

    if ((identifier != null) && !identifier.isEmpty()) {
      log.append(": ").append(identifier);
      Utterance utterance = getUtterance(identifier);

      if (utterance != null) {
        String text = utterance.getText().toString();
        text = Logger.shrinkText(text);
        log.append(": ").append(text);
      }
    }

    if (unexpected) {
      Log.w(LOG_TAG, log.toString());
    } else {
      Log.d(LOG_TAG, log.toString());
    }
  }

  private final static UtteranceProgressListener utteranceProgressListener =
    new UtteranceProgressListener() {
      private final void onUtteranceIncomplete (String identifier) {
        synchronized (activeUtterances) {
          Utterance utterance = activeUtterances.get(identifier);
          activeUtterances.clear();

          if (utterance != null) {
            tellFinished(utterance.getSynthesizer());
          }
        }
      }

      // API Level 15 - called when an utterance starts to be processed
      @Override
      public final void onStart (String identifier) {
        logUtteranceState(identifier, "started", false);
      }

      // API Level 15 - called when an utterance has successfully completed
      @Override
      public final void onDone (String identifier) {
        logUtteranceState(identifier, "finished", false);

        synchronized (activeUtterances) {
          Utterance utterance = activeUtterances.remove(identifier);

          if (utterance != null) {
            if (activeUtterances.isEmpty()) {
              long synthesizer = utterance.getSynthesizer();
              tellLocation(synthesizer, utterance.getRangeEnd());
              tellFinished(synthesizer);
            }
          }
        }
      }

      // API Level 23 - called when an utterance is stopped or flushed
      @Override
      public final void onStop (String identifier, boolean wasInterrupted) {
        logUtteranceState(identifier, (wasInterrupted? "interrupted": "cancelled"), false);
        onUtteranceIncomplete(identifier);
      }

      // API Level 15 - called when an error has occurred
      @Override
      public final void onError (String identifier) {
        logUtteranceState(identifier, "error", true);
        onUtteranceIncomplete(identifier);
      }

      // API Level 21 - called when an error has occurred
      @Override
      public final void onError (String identifier, int code) {
        logUtteranceState(identifier, ("error " + code), true);
        onUtteranceIncomplete(identifier);
      }

      // API Level 24 - called when the engine begins to synthesize a request
      @Override
      public final void onBeginSynthesis (String identifier, int rate, int format, int channels) {
      }

      // API Level 24 - called when a chunk of audio has been generated
      @Override
      public final void onAudioAvailable (String identifier, byte[] audio) {
      }

      // API Level 26 - called when the specified range is about to be spoken
      @Override
      public final void onRangeStart (String identifier, int start, int end, int frame) {
        Utterance utterance = getUtterance(identifier);
        if (utterance != null) utterance.setRange(start, end);
      }
    };

  public static boolean sayText (long synthesizer, CharSequence text) {
    synchronized (ttsParadigm) {
      String identifier = ttsParadigm.setUtteranceIdentifier();

      synchronized (activeUtterances) {
        activeUtterances.put(identifier, new Utterance(synthesizer, text));
      }

      if (ttsParadigm.sayText(ttsObject, text, TextToSpeech.QUEUE_ADD)) {
        return true;
      }

      synchronized (activeUtterances) {
        activeUtterances.remove(identifier);
      }

      logUtteranceState(identifier, "start failed", true);
      return false;
    }
  }

  public static boolean stopSpeaking () {
    int result = ttsObject.stop();
    if (result == TextToSpeech.SUCCESS) return true;

    logSpeechError("stop speaking", result);
    return false;
  }

  public static boolean setVolume (float volume) {
    ttsParadigm.setParameter(TextToSpeech.Engine.KEY_PARAM_VOLUME, volume);
    return true;
  }

  public static boolean setBalance (float balance) {
    ttsParadigm.setParameter(TextToSpeech.Engine.KEY_PARAM_PAN, balance);
    return true;
  }

  public static boolean setRate (float rate) {
    int result = ttsObject.setSpeechRate(rate);
    if (result == TextToSpeech.SUCCESS) return true;

    logSpeechError("set rate", result);
    return false;
  }

  public static boolean setPitch (float pitch) {
    int result = ttsObject.setPitch(pitch);
    if (result == TextToSpeech.SUCCESS) return true;

    logSpeechError("set pitch", result);
    return false;
  }

  public static boolean startEngine () {
    TextToSpeech.OnInitListener listener = new TextToSpeech.OnInitListener() {
      @Override
      public void onInit (int status) {
        synchronized (this) {
          switch (status) {
            case TextToSpeech.SUCCESS:
              engineState = EngineState.STARTED;
              ttsObject.setOnUtteranceProgressListener(utteranceProgressListener);
              break;

            default:
              Log.w(LOG_TAG, ("unknown text to speech engine startup status: " + status));
            case TextToSpeech.ERROR:
              engineState = EngineState.FAILED;
              break;
          }

          notify();
        }
      }
    };

    synchronized (listener) {
      Log.d(LOG_TAG, "starting text to speech engine");
      engineState = EngineState.STOPPED;
      ttsObject = new TextToSpeech(BrailleApplication.get(), listener);

      while (engineState == EngineState.STOPPED) {
        try {
          listener.wait();
        } catch (InterruptedException exception) {
        }
      }
    }

    if (engineState == EngineState.STARTED) {
      ttsParadigm.resetParameters();
      Log.d(LOG_TAG, "text to speech engine started");
      return true;
    }

    Log.w(LOG_TAG, "text to speech engine failed");
    ttsObject = null;
    return false;
  }

  public static void stopEngine () {
    if (ttsObject != null) {
      Log.d(LOG_TAG, "stopping text to speech engine");
      ttsObject.shutdown();
      ttsObject = null;
      engineState = EngineState.STOPPED;
    }
  }
}

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

package org.a11y.brltty.android;
import org.a11y.brltty.core.*;

import android.util.Log;
import android.content.Context;

import android.view.KeyEvent;
import android.view.ViewConfiguration;

import android.inputmethodservice.InputMethodService;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.EditorInfo;

public class InputService extends InputMethodService {
  private final static String LOG_TAG = InputService.class.getName();

  private static volatile InputService inputService = null;

  public static InputService getInputService () {
    return inputService;
  }

  @Override
  public void onCreate () {
    super.onCreate();
    inputService = this;
    Log.d(LOG_TAG, "input service started");
  }

  @Override
  public void onDestroy () {
    try {
      inputService = null;
      Log.d(LOG_TAG, "input service stopped");
    } finally {
      super.onDestroy();
    }
  }

  @Override
  public void onBindInput () {
    Log.d(LOG_TAG, "input service bound");
  }

  @Override
  public void onUnbindInput () {
    Log.d(LOG_TAG, "input service unbound");
  }

  @Override
  public void onStartInput (EditorInfo info, boolean restarting) {
    Log.d(LOG_TAG, "input service " + (restarting? "reconnected": "connected"));
    if (info.actionLabel != null) Log.d(LOG_TAG, "action label: " + info.actionLabel);
    Log.d(LOG_TAG, "action id: " + info.actionId);
  }

  @Override
  public void onFinishInput () {
    Log.d(LOG_TAG, "input service disconnected");
  }

  public static void logKeyEvent (int code, boolean press, String description) {
    if (ApplicationSettings.LOG_KEYBOARD_EVENTS) {
      StringBuilder sb = new StringBuilder();

      sb.append("key ");
      sb.append((press? "press": "release"));
      sb.append(' ');
      sb.append(description);

      sb.append(": ");
      sb.append(code);

      sb.append(" (");
      sb.append(KeyEvent.keyCodeToString(code));
      sb.append(")");

      Log.d(LOG_TAG, sb.toString());
    }
  }

  public static void logKeyEventReceived (int code, boolean press) {
    logKeyEvent(code, press, "received");
  }

  public static void logKeyEventSent (int code, boolean press) {
    logKeyEvent(code, press, "sent");
  }

  public final native boolean handleKeyEvent (int code, boolean press);

  public void forwardKeyEvent (int code, boolean press) {
    InputConnection connection = getCurrentInputConnection();

    if (connection != null) {
      int action = press? KeyEvent.ACTION_DOWN: KeyEvent.ACTION_UP;
      KeyEvent event = new KeyEvent(action, code);
      event = KeyEvent.changeFlags(event, KeyEvent.FLAG_SOFT_KEYBOARD);

      if (connection.sendKeyEvent(event)) {
        logKeyEvent(code, press, "forwarded");
      } else {
        logKeyEvent(code, press, "not forwarded");
      }
    } else {
      logKeyEvent(code, press, "unforwardable");
    }
  }

  public boolean acceptKeyEvent (final int code, final boolean press) {
    switch (code) {
      case KeyEvent.KEYCODE_POWER:
      case KeyEvent.KEYCODE_HOME:
      case KeyEvent.KEYCODE_BACK:
      case KeyEvent.KEYCODE_MENU:
        logKeyEvent(code, press, "rejected");
        return false;

      default:
        logKeyEvent(code, press, "accepted");
        break;
    }

    if (BrailleService.getBrailleService() == null) {
      Log.w(LOG_TAG, "braille service not started");
      return false;
    }

    CoreWrapper.runOnCoreThread(
      new Runnable() {
        @Override
        public void run () {
          logKeyEvent(code, press, "delivered");

          if (handleKeyEvent(code, press)) {
            logKeyEvent(code, press, "handled");
          } else {
            forwardKeyEvent(code, press);
          }
        }
      }
    );

    return true;
  }

  public static boolean isSystemKeyCode (int code) {
    switch (code) {
      case KeyEvent.KEYCODE_HOME:
      case KeyEvent.KEYCODE_BACK:
        return true;

      default:
        return false;
    }
  }

  @Override
  public boolean onKeyDown (int code, KeyEvent event) {
    logKeyEventReceived(code, true);

    if (!isSystemKeyCode(code)) {
      if (acceptKeyEvent(code, true)) {
        return true;
      }
    }

    return super.onKeyDown(code, event);
  }

  @Override
  public boolean onKeyUp (int code, KeyEvent event) {
    logKeyEventReceived(code, false);

    if (!isSystemKeyCode(code)) {
      if (acceptKeyEvent(code, false)) {
        return true;
      }
    }

    return super.onKeyUp(code, event);
  }

  public static InputMethodInfo getInputMethodInfo (Class classObject) {
    String className = classObject.getName();

    for (InputMethodInfo info : ApplicationUtilities.getInputMethodManager().getEnabledInputMethodList()) {
      if (info.getComponent().getClassName().equals(className)) {
        return info;
      }
    }

    return null;
  }

  public static InputMethodInfo getInputMethodInfo () {
    return getInputMethodInfo(InputService.class);
  }

  public static boolean isEnabled () {
    return getInputMethodInfo() != null;
  }

  public static boolean isSelected () {
    InputMethodInfo info = getInputMethodInfo();

    if (info != null) {
      if (info.getId().equals(ApplicationUtilities.getSelectedInputMethodIdentifier())) {
        return true;
      }
    }

    return false;
  }

  public static void switchInputMethod () {
    Log.w(LOG_TAG, "showing input method picker");
    ApplicationUtilities.getInputMethodManager().showInputMethodPicker();
  }

  private static void reportInputProblem (int message) {
    Context context = BrailleApplication.get();
    Log.w(LOG_TAG, context.getResources().getString(message));
    BrailleMessage.WARNING.show(context.getString(message));
  }

  public static InputConnection getInputConnection () {
    InputService service = InputService.getInputService();

    if (service != null) {
      InputConnection connection = service.getCurrentInputConnection();
      if (connection != null) return connection;
      reportInputProblem(R.string.inputService_not_connected);
    } else if (isSelected()) {
      reportInputProblem(R.string.inputService_not_started);
    } else if (isEnabled()) {
      reportInputProblem(R.string.inputService_not_selected);
    } else {
      reportInputProblem(R.string.inputService_not_enabled);
    }

    return null;
  }

  public static boolean injectKey (InputConnection connection, int keyCode, boolean longPress) {
    if (connection.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, keyCode))) {
      logKeyEventSent(keyCode, true);

      if (longPress) {
        try {
          Thread.sleep(ViewConfiguration.getLongPressTimeout() + ApplicationParameters.LONG_PRESS_DELAY);
        } catch (InterruptedException exception) {
        }
      }

      if (connection.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, keyCode))) {
        logKeyEventSent(keyCode, false);
        return true;
      }
    }

    return false;
  }

  public static boolean injectKey (int keyCode, boolean longPress) {
    InputConnection connection = getInputConnection();
    if (connection == null) return false;
    return injectKey(connection, keyCode, longPress);
  }

  public static boolean injectKey (int keyCode) {
    return injectKey(keyCode, false);
  }
}

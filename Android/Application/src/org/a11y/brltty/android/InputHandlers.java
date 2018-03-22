/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brltty.android;

import android.os.Build;
import android.accessibilityservice.AccessibilityService;
import android.view.KeyEvent;
import android.text.Editable;

public abstract class InputHandlers {
  private InputHandlers () {
  }

  public static boolean inputCharacter (final char character) {
    try {
      return new InputTextEditor() {
        @Override
        protected Integer performEdit (Editable editor, int start, int end) {
          editor.replace(start, end, Character.toString(character));
          return start + 1;
        }
      }.wasPerformed();
    } catch (UnsupportedOperationException exception) {
      return InputService.insertCharacter(character);
    }
  }

  private static boolean inputKey (int code) {
    return InputService.inputKey(code);
  }

  public static boolean inputKey_enter () {
    return inputKey(KeyEvent.KEYCODE_ENTER);
  }

  public static boolean inputKey_tab () {
    return inputKey(KeyEvent.KEYCODE_TAB);
  }

  public static boolean inputKey_backspace () {
    try {
      return new InputTextEditor() {
        @Override
        protected Integer performEdit (Editable editor, int start, int end) {
          if (start == end) {
            if (start < 1) return null;
            start -= 1;
          }

          editor.delete(start, end);
          return start;
        }
      }.wasPerformed();
    } catch (UnsupportedOperationException exception) {
      return InputService.deletePreviousCharacter();
    }
  }

  public static boolean inputKey_escape () {
    return inputKey(KeyEvent.KEYCODE_ESCAPE);
  }

  public static boolean inputKey_cursorLeft () {
    return inputKey(KeyEvent.KEYCODE_DPAD_LEFT);
  }

  public static boolean inputKey_cursorRight () {
    return inputKey(KeyEvent.KEYCODE_DPAD_RIGHT);
  }

  public static boolean inputKey_cursorUp () {
    return inputKey(KeyEvent.KEYCODE_DPAD_UP);
  }

  public static boolean inputKey_cursorDown () {
    return inputKey(KeyEvent.KEYCODE_DPAD_DOWN);
  }

  public static boolean inputKey_pageUp () {
    return inputKey(KeyEvent.KEYCODE_PAGE_UP);
  }

  public static boolean inputKey_pageDown () {
    return inputKey(KeyEvent.KEYCODE_PAGE_DOWN);
  }

  public static boolean inputKey_home () {
    return inputKey(KeyEvent.KEYCODE_MOVE_HOME);
  }

  public static boolean inputKey_end () {
    return inputKey(KeyEvent.KEYCODE_MOVE_END);
  }

  public static boolean inputKey_insert () {
    return inputKey(KeyEvent.KEYCODE_INSERT);
  }

  public static boolean inputKey_delete () {
    try {
      return new InputTextEditor() {
        @Override
        protected Integer performEdit (Editable editor, int start, int end) {
          if (start == end) {
            if (end == editor.length()) return null;
            end += 1;
          }

          editor.delete(start, end);
          return start;
        }
      }.wasPerformed();
    } catch (UnsupportedOperationException exception) {
      return InputService.deleteNextCharacter();
    }
  }

  private interface Action {
    public boolean performAction ();
  }

  private final static Action brlttySettingsAction = new Action() {
    @Override
    public boolean performAction () {
      return BrailleService.getBrailleService().launchSettingsActivity();
    }
  };

  private static boolean performGlobalAction (int action) {
    return BrailleService.getBrailleService().performGlobalAction(action);
  }

  private final static Action backAction = new Action() {
    @Override
    public boolean performAction () {
      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        return performGlobalAction(AccessibilityService.GLOBAL_ACTION_BACK);
      }

      return false;
    }
  };

  private final static Action homeAction = new Action() {
    @Override
    public boolean performAction () {
      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        return performGlobalAction(AccessibilityService.GLOBAL_ACTION_HOME);
      }

      return false;
    }
  };

  private final static Action notificationsAction = new Action() {
    @Override
    public boolean performAction () {
      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        return performGlobalAction(AccessibilityService.GLOBAL_ACTION_NOTIFICATIONS);
      }

      return false;
    }
  };

  private final static Action powerDialogAction = new Action() {
    @Override
    public boolean performAction () {
      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.LOLLIPOP)) {
        return performGlobalAction(AccessibilityService.GLOBAL_ACTION_POWER_DIALOG);
      }

      return false;
    }
  };

  private final static Action quickSettingsAction = new Action() {
    @Override
    public boolean performAction () {
      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR1)) {
        return performGlobalAction(AccessibilityService.GLOBAL_ACTION_QUICK_SETTINGS);
      }

      return false;
    }
  };

  private final static Action recentApplicationsAction = new Action() {
    @Override
    public boolean performAction () {
      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        return performGlobalAction(AccessibilityService.GLOBAL_ACTION_RECENTS);
      }

      return false;
    }
  };

  public static boolean changeFocus (RenderedScreen.ChangeFocusDirection direction) {
    RenderedScreen screen = ScreenDriver.getScreen();

    if (screen != null) {
      if (screen.changeFocus(direction)) {
        return true;
      }
    }

    return false;
  }

  private final static Action backwardAction = new Action() {
    @Override
    public boolean performAction () {
      return changeFocus(RenderedScreen.ChangeFocusDirection.BACKWARD);
    }
  };

  private final static Action forwardAction = new Action() {
    @Override
    public boolean performAction () {
      return changeFocus(RenderedScreen.ChangeFocusDirection.FORWARD);
    }
  };

  private final static Action menuAction = new Action() {
    @Override
    public boolean performAction () {
      return inputKey(KeyEvent.KEYCODE_MENU);
    }
  };

  private final static Action logScreenAction = new Action() {
    @Override
    public boolean performAction () {
      ScreenLogger.log();
      return true;
    }
  };

  private final static Action[] functionKeyActions = new Action[] {
    homeAction,
    backAction,
    notificationsAction,
    recentApplicationsAction,
    brlttySettingsAction,
    quickSettingsAction,
    backwardAction,
    forwardAction,
    powerDialogAction,
    menuAction,
    logScreenAction
  };

  public static boolean inputKey_function (int key) {
    if (key < 0) return false;
    if (key >= functionKeyActions.length) return false;

    Action action = functionKeyActions[key];
    if (action == null) return false;
    return action.performAction();
  }
}

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
import android.os.Bundle;

import android.accessibilityservice.AccessibilityService;
import android.view.accessibility.AccessibilityNodeInfo;

import android.view.KeyEvent;

import android.text.Editable;
import android.text.SpannableStringBuilder;

public abstract class InputHandlers {
  private InputHandlers () {
  }

  private static abstract class TextEditor {
    private final boolean editWasPerformed;

    public final boolean wasPerformed () {
      return editWasPerformed;
    }

    protected abstract Integer performEdit (Editable editor, int start, int end);

    private final boolean performEdit (AccessibilityNodeInfo node) {
      if (node.isFocused()) {
        CharSequence text = node.getText();
        int start = node.getTextSelectionStart();
        int end = node.getTextSelectionEnd();

        if (text == null) {
          text = "";
          start = end = 0;
        }

        Editable editor = (text instanceof Editable)?
                          (Editable)text:
                          new SpannableStringBuilder(text);

        if ((0 <= start) && (start <= end) && (end <= text.length())) {
          Integer position = performEdit(editor, start, end);

          if (position != null) {
            Bundle arguments = new Bundle();
            arguments.putCharSequence(AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE, editor);

            if (node.performAction(AccessibilityNodeInfo.ACTION_SET_TEXT, arguments)) {
              if (position == editor.length()) return true;

              arguments.clear();
              arguments.putInt(AccessibilityNodeInfo.ACTION_ARGUMENT_SELECTION_START_INT, position);
              arguments.putInt(AccessibilityNodeInfo.ACTION_ARGUMENT_SELECTION_END_INT, position);
              return node.performAction(AccessibilityNodeInfo.ACTION_SET_SELECTION, arguments);
            }
          }
        }
      }

      return false;
    }

    public TextEditor () {
      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.LOLLIPOP)) {
        RenderedScreen screen = ScreenDriver.getScreen();

        if (screen != null) {
          AccessibilityNodeInfo node = screen.getCursorNode();

          if (node != null) {
            if (node.isEditable()) {
              editWasPerformed = performEdit(node);
              return;
            }
          }
        }
      }

      throw new UnsupportedOperationException();
    }
  }

  public static boolean inputCharacter (final char character) {
    try {
      return new TextEditor() {
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
    return InputService.injectKey(code);
  }

  public static boolean inputKey_enter () {
    return inputKey(KeyEvent.KEYCODE_ENTER);
  }

  public static boolean inputKey_tab () {
    return inputKey(KeyEvent.KEYCODE_TAB);
  }

  public static boolean inputKey_backspace () {
    try {
      return new TextEditor() {
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
      return new TextEditor() {
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

  private interface FunctionKeyAction {
    public boolean performAction ();
  }

  private final static FunctionKeyAction logScreenAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        ScreenLogger.log();
        return true;
      }
    };

  private final static FunctionKeyAction brlttySettingsAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        return BrailleService.getBrailleService().launchSettingsActivity();
      }
    };

  private static boolean performGlobalAction (int action) {
    return BrailleService.getBrailleService().performGlobalAction(action);
  }

  private final static FunctionKeyAction backAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          return performGlobalAction(AccessibilityService.GLOBAL_ACTION_BACK);
        }

        return false;
      }
    };

  private final static FunctionKeyAction homeAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          return performGlobalAction(AccessibilityService.GLOBAL_ACTION_HOME);
        }

        return false;
      }
    };

  private final static FunctionKeyAction notificationsAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          return performGlobalAction(AccessibilityService.GLOBAL_ACTION_NOTIFICATIONS);
        }

        return false;
      }
    };

  private final static FunctionKeyAction powerDialogAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.LOLLIPOP)) {
          return performGlobalAction(AccessibilityService.GLOBAL_ACTION_POWER_DIALOG);
        }

        return false;
      }
    };

  private final static FunctionKeyAction quickSettingsAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR1)) {
          return performGlobalAction(AccessibilityService.GLOBAL_ACTION_QUICK_SETTINGS);
        }

        return false;
      }
    };

  private final static FunctionKeyAction recentApplicationsAction =
    new FunctionKeyAction() {
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

  private final static FunctionKeyAction backwardAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        return changeFocus(RenderedScreen.ChangeFocusDirection.BACKWARD);
      }
    };

  private final static FunctionKeyAction forwardAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        return changeFocus(RenderedScreen.ChangeFocusDirection.FORWARD);
      }
    };

  private final static FunctionKeyAction menuAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        return inputKey(KeyEvent.KEYCODE_MENU);
      }
    };

  private final static FunctionKeyAction[] functionKeyActions =
    new FunctionKeyAction[] {
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

    FunctionKeyAction action = functionKeyActions[key];
    if (action == null) return false;
    return action.performAction();
  }
}

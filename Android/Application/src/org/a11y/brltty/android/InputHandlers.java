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

  private static boolean performGlobalAction (int action) {
    return BrailleService.getBrailleService().performGlobalAction(action);
  }

  private static boolean performNodeAction (AccessibilityNodeInfo node, int action) {
    node = AccessibilityNodeInfo.obtain(node);

    while (node != null) {
      AccessibilityNodeInfo parent;

      try {
        if ((node.getActions() & action) != 0) {
          return node.performAction(action);
        }

        parent = node.getParent();
      } finally {
        node.recycle();
        node = null;
      }

      if (parent == null) break;
      node = parent;
      parent = null;
    }

    return false;
  }

  private static boolean moveFocus (RenderedScreen.SearchDirection direction) {
    RenderedScreen screen = ScreenDriver.getScreen();

    if (screen != null) {
      if (screen.moveFocus(direction)) {
        return true;
      }
    }

    return false;
  }

  private static AccessibilityNodeInfo getCursorNode () {
    RenderedScreen screen = ScreenDriver.getScreen();
    if (screen == null) return null;
    return screen.getCursorNode();
  }

  private static int getTextStartOffset (AccessibilityNodeInfo node) {
    return node.getTextSelectionStart();
  }

  private static int getTextEndOffset (AccessibilityNodeInfo node) {
    int position = node.getTextSelectionEnd();
    if (position != node.getTextSelectionStart()) position -= 1;
    return position;
  }

  private static boolean placeTextCursor (AccessibilityNodeInfo node, int position) {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
      Bundle arguments = new Bundle();
      arguments.putInt(AccessibilityNodeInfo.ACTION_ARGUMENT_SELECTION_START_INT, position);
      arguments.putInt(AccessibilityNodeInfo.ACTION_ARGUMENT_SELECTION_END_INT, position);
      return node.performAction(AccessibilityNodeInfo.ACTION_SET_SELECTION, arguments);
    }

    return false;
  }

  private static Integer findNextLine (CharSequence text, int offset) {
    int length = text.length();

    while (offset < length) {
      if (text.charAt(offset++) == '\n') return offset;
    }

    return null;
  }

  private static int findCurrentLine (CharSequence text, int offset) {
    while (offset > 0) {
      if (text.charAt(--offset) == '\n') {
        offset += 1;
        break;
      }
    }

    return offset;
  }

  private static Integer findPreviousLine (CharSequence text, int offset) {
    offset = findCurrentLine(text, offset);
    if (offset == 0) return null;
    return findCurrentLine(text, offset-1);
  }

  private abstract static class TextEditor {
    public TextEditor () {
    }

    protected abstract Integer editText (Editable editor, int start, int end);

    private final boolean editText (AccessibilityNodeInfo node) {
      if (node.isFocused()) {
        CharSequence text = node.getText();
        int start = node.getTextSelectionStart();
        int end = node.getTextSelectionEnd();

        if (text == null) text = "";
        if (text.length() == 0) start = end = 0;

        Editable editor = (text instanceof Editable)?
                          (Editable)text:
                          new SpannableStringBuilder(text);

        if ((0 <= start) && (start <= end) && (end <= text.length())) {
          Integer position = editText(editor, start, end);

          if (position != null) {
            Bundle arguments = new Bundle();
            arguments.putCharSequence(AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE, editor);

            if (node.performAction(AccessibilityNodeInfo.ACTION_SET_TEXT, arguments)) {
              if (position == editor.length()) return true;
              return placeTextCursor(node, position);
            }
          }
        }
      }

      return false;
    }

    public final boolean editText () {
      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.LOLLIPOP)) {
        AccessibilityNodeInfo node = getCursorNode();

        if (node != null) {
          try {
            if (node.isEditable()) return editText(node);
          } finally {
            node.recycle();
            node = null;
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
        protected Integer editText (Editable editor, int start, int end) {
          editor.replace(start, end, Character.toString(character));
          return start + 1;
        }
      }.editText();
    } catch (UnsupportedOperationException exception) {
      return InputService.insertCharacter(character);
    }
  }

  private static boolean injectKey (int code) {
    return InputService.injectKey(code);
  }

  private abstract static class KeyHandler {
    private final int keyCode;

    public KeyHandler (int code) {
      keyCode = code;
    }

    protected boolean performNavigationAction (AccessibilityNodeInfo node) {
      throw new UnsupportedOperationException();
    }

    protected boolean performEditAction (AccessibilityNodeInfo node) {
      return performNavigationAction(node);
    }

    public final boolean handleKey () {
      try {
        AccessibilityNodeInfo node = getCursorNode();

        if (node != null) {
          try {
            return node.isEditable()?
                   performEditAction(node):
                   performNavigationAction(node);
          } finally {
            node.recycle();
            node = null;
          }
        }

        throw new UnsupportedOperationException();
      } catch (UnsupportedOperationException exception) {
        return injectKey(keyCode);
      }
    }
  }

  public static boolean inputKey_enter () {
    return new KeyHandler(KeyEvent.KEYCODE_ENTER) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          return performNodeAction(node, AccessibilityNodeInfo.ACTION_CLICK);
        }

        return super.performNavigationAction(node);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.LOLLIPOP)) {
          return inputCharacter('\n');
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean inputKey_tab () {
    return new KeyHandler(KeyEvent.KEYCODE_TAB) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return moveFocus(RenderedScreen.SearchDirection.FORWARD);
      }
    }.handleKey();
  }

  public static boolean inputKey_backspace () {
    try {
      return new TextEditor() {
        @Override
        protected Integer editText (Editable editor, int start, int end) {
          if (start == end) {
            if (start < 1) return null;
            start -= 1;
          }

          editor.delete(start, end);
          return start;
        }
      }.editText();
    } catch (UnsupportedOperationException exception) {
      return InputService.deletePreviousCharacter();
    }
  }

  public static boolean inputKey_escape () {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      return performGlobalAction(AccessibilityService.GLOBAL_ACTION_BACK);
    }

    return injectKey(KeyEvent.KEYCODE_ESCAPE);
  }

  public static boolean inputKey_cursorLeft () {
    return new KeyHandler(KeyEvent.KEYCODE_DPAD_LEFT) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return super.performNavigationAction(node);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
          int position = node.getTextSelectionStart();
          if (position == node.getTextSelectionEnd()) position -= 1;
          if (position < 0) return false;
          return placeTextCursor(node, position);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean inputKey_cursorRight () {
    return new KeyHandler(KeyEvent.KEYCODE_DPAD_RIGHT) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return super.performNavigationAction(node);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
          int position = node.getTextSelectionEnd();
          if (position == node.getTextSelectionStart()) position += 1;
          if (position > node.getText().length()) return false;
          return placeTextCursor(node, position);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean inputKey_cursorUp () {
    return new KeyHandler(KeyEvent.KEYCODE_DPAD_UP) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return super.performNavigationAction(node);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
          int offset = getTextStartOffset(node);

          CharSequence text = node.getText();
          int current = findCurrentLine(text, offset);
          if (current == 0) return false;

          int end = current - 1;
          int previous = findCurrentLine(text, end);

          int position = Math.min(offset-current, end-previous);
          return placeTextCursor(node, previous+position);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean inputKey_cursorDown () {
    return new KeyHandler(KeyEvent.KEYCODE_DPAD_DOWN) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return super.performNavigationAction(node);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
          int offset = getTextEndOffset(node);

          CharSequence text = node.getText();
          Integer next = findNextLine(text, offset);
          if (next == null) return false;

          int current = findCurrentLine(text, offset);
          int position = offset - current;

          current = next;
          next = findNextLine(text, current);

          int end = ((next != null)? next-1: text.length()) - current;
          if (position > end) position = end;
          return placeTextCursor(node, current+position);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean inputKey_pageUp () {
    return new KeyHandler(KeyEvent.KEYCODE_PAGE_UP) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          return performNodeAction(node, AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
        }

        return super.performNavigationAction(node);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
          int from = getTextStartOffset(node);

          final int to = 0;
          if (to == from) return false;

          return placeTextCursor(node, to);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean inputKey_pageDown () {
    return new KeyHandler(KeyEvent.KEYCODE_PAGE_DOWN) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          return performNodeAction(node, AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
        }

        return super.performNavigationAction(node);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
          int from = getTextEndOffset(node);

          final int to = node.getText().length();
          if (to == from) return false;

          return placeTextCursor(node, to);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean inputKey_home () {
    return new KeyHandler(KeyEvent.KEYCODE_MOVE_HOME) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return moveFocus(RenderedScreen.SearchDirection.FIRST);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
          int from = getTextStartOffset(node);
          if (from == 0) return false;

          CharSequence text = node.getText();
          int to = findCurrentLine(text, from);
          if (to == from) return false;

          return placeTextCursor(node, to);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean inputKey_end () {
    return new KeyHandler(KeyEvent.KEYCODE_MOVE_END) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return moveFocus(RenderedScreen.SearchDirection.LAST);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
          int from = getTextEndOffset(node);

          CharSequence text = node.getText();
          Integer next = findNextLine(text, from);

          int to = (next != null)? next-1: text.length();
          if (from == to) return false;

          return placeTextCursor(node, to);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean inputKey_insert () {
    return new KeyHandler(KeyEvent.KEYCODE_INSERT) {
    }.handleKey();
  }

  public static boolean inputKey_delete () {
    try {
      return new TextEditor() {
        @Override
        protected Integer editText (Editable editor, int start, int end) {
          if (start == end) {
            if (end == editor.length()) return null;
            end += 1;
          }

          editor.delete(start, end);
          return start;
        }
      }.editText();
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

  private final static FunctionKeyAction moveBackwardAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        return moveFocus(RenderedScreen.SearchDirection.BACKWARD);
      }
    };

  private final static FunctionKeyAction moveForwardAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        return moveFocus(RenderedScreen.SearchDirection.FORWARD);
      }
    };

  private final static FunctionKeyAction menuAction =
    new FunctionKeyAction() {
      @Override
      public boolean performAction () {
        return new KeyHandler(KeyEvent.KEYCODE_MENU) {
        }.handleKey();
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
      moveBackwardAction,
      moveForwardAction,
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

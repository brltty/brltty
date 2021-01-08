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
import org.a11y.brltty.android.activities.ActionsActivity;
import org.a11y.brltty.core.Braille;

import java.util.Arrays;
import java.util.Comparator;

import java.util.Collections;
import java.util.List;

import android.os.Bundle;
import android.graphics.Rect;

import android.accessibilityservice.AccessibilityService;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityWindowInfo;

import android.view.inputmethod.InputConnection;
import android.view.KeyEvent;

import android.text.Editable;
import android.text.SpannableStringBuilder;

public abstract class InputHandlers {
  private InputHandlers () {
  }

  private final static int NO_SELECTION = -1;

  private static boolean performGlobalAction (int action) {
    return BrailleService.getBrailleService().performGlobalAction(action);
  }

  private static AccessibilityNodeInfo getCursorNode () {
    RenderedScreen screen = ScreenDriver.getCurrentRenderedScreen();
    if (screen == null) return null;
    return screen.getCursorNode();
  }

  private static CharSequence getWindowTitle (AccessibilityWindowInfo window) {
    if (APITests.haveNougat) {
      CharSequence title = window.getTitle();
      if ((title != null) && (title.length() > 0)) return title;
    }

    if (APITests.havePie) {
      AccessibilityNodeInfo node = window.getRoot();

      if (node != null) {
        try {
          while (true) {
            {
              CharSequence title = node.getPaneTitle();
              if ((title != null) && (title.length() > 0)) return title;
            }

            if (node.getChildCount() != 1) break;
            AccessibilityNodeInfo child = node.getChild(0);
            if (child == null) break;

            node.recycle();
            node = child;
            child = null;
          }
        } finally {
          node.recycle();
          node = null;
        }
      }
    }

    return "unnamed";
  }

  private static void showWindowTitle (AccessibilityWindowInfo window) {
    CharSequence title = getWindowTitle(window);
    BrailleMessage.WINDOW.show(title.toString());
  }

  private static void showWindowTitle (ScreenWindow window) {
    AccessibilityWindowInfo info = window.getWindowInfo();

    if (info != null) {
      try {
        showWindowTitle(info);
      } finally {
        info.recycle();
        info = null;
      }
    }
  }

  private static void showWindowTitle () {
    showWindowTitle(ScreenDriver.getCurrentScreenWindow());
  }

  private static List<AccessibilityWindowInfo> getVisibleWindows () {
    if (APITests.haveLollipop) {
      return BrailleService.getBrailleService().getWindows();
    } else {
      return Collections.EMPTY_LIST;
    }
  }

  private static AccessibilityWindowInfo[] getVisibleWindows (final Comparator<Integer> comparator) {
    List<AccessibilityWindowInfo> list = getVisibleWindows();
    AccessibilityWindowInfo[] array = list.toArray(new AccessibilityWindowInfo[list.size()]);

    Arrays.sort(array,
      new Comparator<AccessibilityWindowInfo>() {
        @Override
        public int compare (AccessibilityWindowInfo window1, AccessibilityWindowInfo window2) {
          return comparator.compare(window1.getId(), window2.getId());
        }
      }
    );

    return array;
  }

  private static boolean switchToWindow (AccessibilityWindowInfo window) {
    AccessibilityNodeInfo root = window.getRoot();

    if (root != null) {
      try {
        RenderedScreen screen = new RenderedScreen(root);
        showWindowTitle(window);

        ScreenDriver.lockScreenWindow(
          ScreenWindow.getScreenWindow(window)
                      .setRenderedScreen(screen)
        );

        ScreenDriver.setCurrentNode(root);
        return true;
      } finally {
        root.recycle();
        root = null;
      }
    }

    return false;
  }

  private static boolean switchToWindow (Comparator<Integer> comparator) {
    boolean found = false;
    AccessibilityNodeInfo cursorNode = getCursorNode();

    if (cursorNode != null) {
      try {
        int referenceIdentifier = cursorNode.getWindowId();

        for (AccessibilityWindowInfo window : getVisibleWindows(comparator)) {
          try {
            if (!found) {
              if (comparator.compare(window.getId(), referenceIdentifier) > 0) {
                if (switchToWindow(window)) {
                  found = true;
                }
              }
            }
          } finally {
            window.recycle();
            window = null;
          }
        }
      } finally {
        cursorNode.recycle();
        cursorNode = null;
      }
    }

    return found;
  }

  private static boolean moveFocus (RenderedScreen.SearchDirection direction) {
    RenderedScreen screen = ScreenDriver.getCurrentRenderedScreen();

    if (screen != null) {
      if (screen.moveFocus(direction)) {
        return true;
      }
    }

    return false;
  }

  private static int getTextStartOffset (AccessibilityNodeInfo node) {
    int offset = node.getTextSelectionStart();
    if (offset == NO_SELECTION) return 0;
    return offset;
  }

  private static int getTextEndOffset (AccessibilityNodeInfo node) {
    int offset = node.getTextSelectionEnd();
    if (offset == NO_SELECTION) return 0;
    if (offset != getTextStartOffset(node)) offset -= 1;
    return offset;
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

  public static boolean setSelection (AccessibilityNodeInfo node, int start, int end) {
    if (APITests.haveJellyBeanMR2) {
      Bundle arguments = new Bundle();
      arguments.putInt(AccessibilityNodeInfo.ACTION_ARGUMENT_SELECTION_START_INT, start);
      arguments.putInt(AccessibilityNodeInfo.ACTION_ARGUMENT_SELECTION_END_INT, end);
      return node.performAction(AccessibilityNodeInfo.ACTION_SET_SELECTION, arguments);
    }

    {
      InputConnection connection = InputService.getInputConnection();
      if (connection != null) return connection.setSelection(start, end);
    }

    return false;
  }

  public static boolean placeCursor (AccessibilityNodeInfo node, int offset) {
    return setSelection(node, offset, offset);
  }

  private abstract static class TextEditor {
    public TextEditor () {
    }

    protected abstract boolean editText (InputConnection connection);
    protected abstract Integer editText (Editable editor, int start, int end);

    private final boolean editText (AccessibilityNodeInfo node) {
      if (node.isFocused()) {
        CharSequence text = node.getText();
        int start = node.getTextSelectionStart();
        int end = node.getTextSelectionEnd();

        if (start == NO_SELECTION) text = null;
        if (text == null) text = "";
        if (text.length() == 0) start = end = 0;

        Editable editor = (text instanceof Editable)?
                          (Editable)text:
                          new SpannableStringBuilder(text);

        if ((0 <= start) && (start <= end) && (end <= text.length())) {
          Integer offset = editText(editor, start, end);

          if (offset != null) {
            Bundle arguments = new Bundle();
            arguments.putCharSequence(AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE, editor);

            if (node.performAction(AccessibilityNodeInfo.ACTION_SET_TEXT, arguments)) {
              if (offset == editor.length()) return true;
              return placeCursor(node, offset);
            }
          }
        }
      }

      return false;
    }

    public final boolean editText () {
      {
        AccessibilityNodeInfo node = getCursorNode();

        if (node != null) {
          try {
            if (ScreenUtilities.isEditable(node)) {
              if (ApplicationParameters.ENABLE_SET_TEXT) {
                if (APITests.haveLollipop) {
                  if (!node.isPassword()) {
                    return editText(node);
                  }
                }
              }
            }
          } finally {
            node.recycle();
            node = null;
          }
        }
      }

      {
        InputConnection connection = InputService.getInputConnection();
        if (connection != null) return editText(connection);
      }

      return false;
    }
  }

  public static boolean inputCharacter (final char character) {
    return new TextEditor() {
      @Override
      protected boolean editText (InputConnection connection) {
        return connection.commitText(Character.toString(character), 1);
      }

      @Override
      protected Integer editText (Editable editor, int start, int end) {
        editor.replace(start, end, Character.toString(character));
        return start + 1;
      }
    }.editText();
  }

  private static char structuralMotionDirectionDot = StructuralMotion.DOT_NEXT;

  public static boolean performStructuralMotion (byte dots) {
    char character = (char)dots;
    character &= Braille.DOTS_ALL;

    boolean next = (character & StructuralMotion.DOT_NEXT) != 0;
    boolean previous = (character & StructuralMotion.DOT_PREVIOUS) != 0;

    if (next && previous) return false;
    boolean noDirection = !(next || previous);

    if ((character & ~StructuralMotion.DOTS_DIRECTION) == 0) {
      if (noDirection) return false;
      structuralMotionDirectionDot = next? StructuralMotion.DOT_NEXT: StructuralMotion.DOT_PREVIOUS;
      return true;
    }

    if (noDirection) character |= structuralMotionDirectionDot;
    character |= Braille.ROW;

    StructuralMotion motion = StructuralMotion.get(character);
    if (motion == null) return false;

    AccessibilityNodeInfo node = getCursorNode();
    if (node == null) return false;

    try {
      return motion.apply(node);
    } finally {
      node.recycle();
      node = null;
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

  public static boolean keyHandler_enter () {
    return new KeyHandler(KeyEvent.KEYCODE_ENTER) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        if (APITests.haveJellyBean) {
          return ScreenUtilities.performClick(node);
        }

        return super.performNavigationAction(node);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (APITests.haveLollipop) {
          if (node.isMultiLine()) {
            return inputCharacter('\n');
          }
        }

        throw new UnsupportedOperationException();
      }
    }.handleKey();
  }

  public static boolean keyHandler_tab () {
    return new KeyHandler(KeyEvent.KEYCODE_TAB) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return moveFocus(RenderedScreen.SearchDirection.FORWARD);
      }
    }.handleKey();
  }

  public static boolean keyHandler_backspace () {
    return new TextEditor() {
      @Override
      protected boolean editText (InputConnection connection) {
        return InputService.injectKey(connection, KeyEvent.KEYCODE_DEL, false);
      }

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
  }

  public static boolean keyHandler_escape () {
    if (APITests.haveJellyBean) {
      return performGlobalAction(AccessibilityService.GLOBAL_ACTION_BACK);
    }

    return injectKey(KeyEvent.KEYCODE_ESCAPE);
  }

  public static boolean keyHandler_cursorLeft () {
    return new KeyHandler(KeyEvent.KEYCODE_DPAD_LEFT) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return moveFocus(RenderedScreen.SearchDirection.LEFT);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (APITests.haveJellyBeanMR2) {
          int offset = node.getTextSelectionStart();
          if (offset == NO_SELECTION) return false;
          if (offset == node.getTextSelectionEnd()) offset -= 1;
          if (offset < 0) return false;
          return placeCursor(node, offset);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean keyHandler_cursorRight () {
    return new KeyHandler(KeyEvent.KEYCODE_DPAD_RIGHT) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return moveFocus(RenderedScreen.SearchDirection.RIGHT);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (APITests.haveJellyBeanMR2) {
          int offset = node.getTextSelectionEnd();
          if (offset == NO_SELECTION) return false;
          if (offset == node.getTextSelectionStart()) offset += 1;
          if (offset > node.getText().length()) return false;
          return placeCursor(node, offset);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean keyHandler_cursorUp () {
    return new KeyHandler(KeyEvent.KEYCODE_DPAD_UP) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return moveFocus(RenderedScreen.SearchDirection.UP);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (APITests.haveJellyBeanMR2) {
          int offset = getTextStartOffset(node);

          CharSequence text = node.getText();
          int current = findCurrentLine(text, offset);
          if (current == 0) return false;

          int end = current - 1;
          int previous = findCurrentLine(text, end);

          int position = Math.min(offset-current, end-previous);
          return placeCursor(node, previous+position);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean keyHandler_cursorDown () {
    return new KeyHandler(KeyEvent.KEYCODE_DPAD_DOWN) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return moveFocus(RenderedScreen.SearchDirection.DOWN);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (APITests.haveJellyBeanMR2) {
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
          return placeCursor(node, current+position);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean keyHandler_pageUp () {
    return new KeyHandler(KeyEvent.KEYCODE_PAGE_UP) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        if (APITests.haveJellyBean) {
          return ScreenUtilities.performScrollBackward(node);
        }

        return super.performNavigationAction(node);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (APITests.haveJellyBeanMR2) {
          int from = getTextStartOffset(node);

          final int to = 0;
          if (to == from) return false;

          return placeCursor(node, to);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean keyHandler_pageDown () {
    return new KeyHandler(KeyEvent.KEYCODE_PAGE_DOWN) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        if (APITests.haveJellyBean) {
          return ScreenUtilities.performScrollForward(node);
        }

        return super.performNavigationAction(node);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (APITests.haveJellyBeanMR2) {
          int from = getTextEndOffset(node);

          final int to = node.getText().length();
          if (to == from) return false;

          return placeCursor(node, to);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean keyHandler_home () {
    return new KeyHandler(KeyEvent.KEYCODE_MOVE_HOME) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return moveFocus(RenderedScreen.SearchDirection.FIRST);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (APITests.haveJellyBeanMR2) {
          int from = getTextStartOffset(node);
          if (from == 0) return false;

          CharSequence text = node.getText();
          int to = findCurrentLine(text, from);
          if (to == from) return false;

          return placeCursor(node, to);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean keyHandler_end () {
    return new KeyHandler(KeyEvent.KEYCODE_MOVE_END) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        return moveFocus(RenderedScreen.SearchDirection.LAST);
      }

      @Override
      protected boolean performEditAction (AccessibilityNodeInfo node) {
        if (APITests.haveJellyBeanMR2) {
          int from = getTextEndOffset(node);

          CharSequence text = node.getText();
          Integer next = findNextLine(text, from);

          int to = (next != null)? next-1: text.length();
          if (from == to) return false;

          return placeCursor(node, to);
        }

        return super.performEditAction(node);
      }
    }.handleKey();
  }

  public static boolean keyHandler_insert () {
    return new KeyHandler(KeyEvent.KEYCODE_INSERT) {
      @Override
      protected boolean performNavigationAction (AccessibilityNodeInfo node) {
        if (APITests.haveJellyBean) {
          return ScreenUtilities.performLongClick(node);
        }

        return super.performNavigationAction(node);
      }
    }.handleKey();
  }

  public static boolean keyHandler_delete () {
    return new TextEditor() {
      @Override
      protected boolean editText (InputConnection connection) {
        return InputService.injectKey(connection, KeyEvent.KEYCODE_FORWARD_DEL, false);
      }

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
  }

  public static boolean globalAction_brailleActions () {
    ActionsActivity.launch();
    return true;
  }

  public static boolean globalAction_showStatusIndicators () {
    String indicators = StatusIndicators.get();
    if (indicators == null) return false;
    if (indicators.isEmpty()) return false;

    BrailleMessage.PLAIN.show(indicators);
    return true;
  }

  public static boolean globalAction_showWindowTitle () {
    showWindowTitle();
    return true;
  }

  public static boolean globalAction_toActiveWindow () {
    ScreenDriver.unlockScreenWindow();
    AccessibilityNodeInfo root = ScreenUtilities.getRootNode();

    try {
      ScreenDriver.setCurrentNode(root);
    } finally {
      root.recycle();
      root = null;
    }

    return true;
  }

  public static boolean globalAction_toPreviousWindow () {
    Comparator<Integer> comparator =
      new Comparator<Integer>() {
        @Override
        public int compare (Integer id1, Integer id2) {
          return -Integer.compare(id1, id2);
        }
      };

    return switchToWindow(comparator);
  }

  public static boolean globalAction_toNextWindow () {
    Comparator<Integer> comparator =
      new Comparator<Integer>() {
        @Override
        public int compare (Integer id1, Integer id2) {
          return Integer.compare(id1, id2);
        }
      };

    return switchToWindow(comparator);
  }

  public static boolean globalAction_toFirstItem () {
    return moveFocus(RenderedScreen.SearchDirection.FIRST);
  }

  public static boolean globalAction_toPreviousItem () {
    return moveFocus(RenderedScreen.SearchDirection.BACKWARD);
  }

  public static boolean globalAction_toNextItem () {
    return moveFocus(RenderedScreen.SearchDirection.FORWARD);
  }

  public static boolean globalAction_toLastItem () {
    return moveFocus(RenderedScreen.SearchDirection.LAST);
  }

  public static boolean globalAction_homeScreen () {
    if (APITests.haveJellyBean) {
      return performGlobalAction(AccessibilityService.GLOBAL_ACTION_HOME);
    }

    return false;
  }

  public static boolean globalAction_notificationsShade () {
    if (APITests.haveJellyBean) {
      return performGlobalAction(AccessibilityService.GLOBAL_ACTION_NOTIFICATIONS);
    }

    return false;
  }

  public static boolean globalAction_quickSettings () {
    if (APITests.haveJellyBeanMR1) {
      return performGlobalAction(AccessibilityService.GLOBAL_ACTION_QUICK_SETTINGS);
    }

    return false;
  }

  public static boolean globalAction_recentApplications () {
    if (APITests.haveJellyBean) {
      return performGlobalAction(AccessibilityService.GLOBAL_ACTION_RECENTS);
    }

    return false;
  }

  public static boolean globalAction_deviceOptions () {
    if (APITests.haveLollipop) {
      return performGlobalAction(AccessibilityService.GLOBAL_ACTION_POWER_DIALOG);
    }

    return false;
  }

  public static boolean globalAction_backButton () {
    if (APITests.haveJellyBean) {
      return performGlobalAction(AccessibilityService.GLOBAL_ACTION_BACK);
    }

    return false;
  }

  public static boolean globalAction_menuButton () {
    return new KeyHandler(KeyEvent.KEYCODE_MENU) {
    }.handleKey();
  }

  public static boolean globalAction_logScreen () {
    ScreenLogger.log();
    return true;
  }

  public static boolean keyHandler_function (int key) {
    switch (key + 1) {
      case  1: return globalAction_homeScreen();
      case  2: return globalAction_backButton();
      case  3: return globalAction_notificationsShade();
      case  4: return globalAction_recentApplications();
      case  5: return globalAction_brailleActions();
      case  6: return globalAction_toFirstItem();
      case  7: return globalAction_toPreviousItem();
      case  8: return globalAction_toNextItem();
      case  9: return globalAction_toLastItem();
      case 10: return globalAction_menuButton();
      case 11: return globalAction_toActiveWindow();
      case 12: return globalAction_toPreviousWindow();
      case 13: return globalAction_toNextWindow();
      case 14: return globalAction_showWindowTitle();
      case 15: return globalAction_showStatusIndicators();
      case 16: return globalAction_quickSettings();
      case 17: return globalAction_deviceOptions();
      case 19: return globalAction_logScreen();
      default: return false;
    }
  }

  public static boolean textHandler_setSelection (int startColumn, int startRow, int endColumn, int endRow) {
    RenderedScreen screen = ScreenDriver.getCurrentRenderedScreen();
    if (screen == null) return false;

    ScreenElement element = screen.findScreenElement(startColumn, startRow);
    if (element == null) return false;

    {
      ScreenElement end = screen.findScreenElement(endColumn, endRow);
      if (end != element) return false;
    }

    AccessibilityNodeInfo node = element.getAccessibilityNode();
    if (node == null) return false;

    try {
      Rect location = element.getBrailleLocation();
      int left = location.left;
      int top = location.top;

      int start = element.getTextOffset((startColumn - left), (startRow - top));
      int end = element.getTextOffset((endColumn - left), (endRow - top)) + 1;

      return setSelection(node, start, end);
    } finally {
      node.recycle();
      node = null;
    }
  }

  public abstract static class NodeHandler {
    public abstract boolean handleNode (AccessibilityNodeInfo node);

    public final boolean handleNode () {
      AccessibilityNodeInfo node = getCursorNode();
      if (node == null) return false;

      try {
        return handleNode(node);
      } finally {
        node.recycle();
        node = null;
      }
    }

    public NodeHandler () {
    }
  }

  public static boolean textHandler_selectAll () {
    return new NodeHandler() {
      @Override
      public boolean handleNode (AccessibilityNodeInfo node) {
        return setSelection(node, 0, node.getText().length());
      }
    }.handleNode();
  }

  private static boolean performNodeAction (final int action) {
    return new NodeHandler() {
      @Override
      public boolean handleNode (AccessibilityNodeInfo node) {
        return node.performAction(action);
      }
    }.handleNode();
  }

  public static boolean textHandler_clearSelection () {
    if (APITests.haveJellyBeanMR2) {
      return new NodeHandler() {
        @Override
        public boolean handleNode (AccessibilityNodeInfo node) {
          int start = node.getTextSelectionStart();
          int end = node.getTextSelectionEnd();
          if (end <= start) return false;
          return placeCursor(node, end);
        }
      }.handleNode();
    }

    return false;
  }

  public static boolean textHandler_copySelection () {
    if (APITests.haveJellyBeanMR2) {
      return performNodeAction(AccessibilityNodeInfo.ACTION_COPY);
    }

    return false;
  }

  public static boolean textHandler_cutSelection () {
    if (APITests.haveJellyBeanMR2) {
      return performNodeAction(AccessibilityNodeInfo.ACTION_CUT);
    }

    return false;
  }

  public static boolean textHandler_pasteClipboard () {
    if (APITests.haveJellyBeanMR2) {
      return performNodeAction(AccessibilityNodeInfo.ACTION_PASTE);
    }

    return false;
  }
}

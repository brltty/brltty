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

import java.util.Map;
import java.util.HashMap;

import android.util.Log;

import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityWindowInfo;

import android.graphics.Rect;

public abstract class ScreenLogger {
  private static final String LOG_TAG = ScreenLogger.class.getName();

  private ScreenLogger () {
  }

  private static String getText (AccessibilityNodeInfo node) {
    String text = ScreenUtilities.getText(node);
    if (text != null) return text;
    return ScreenUtilities.normalizeText(node.getContentDescription());
  }

  private static final void add (StringBuilder sb, String value) {
    if (value != null) {
      if (sb.length() > 0) sb.append(' ');
      sb.append(value);
    }
  }

  private static final void add (StringBuilder sb, boolean condition, String trueValue, String falseValue) {
    add(sb, (condition? trueValue: falseValue));
  }

  private static final void add (StringBuilder sb, boolean condition, String trueValue) {
    add(sb, condition, trueValue, null);
  }

  private static final void add (StringBuilder sb, String label, CharSequence value) {
    add(sb, String.format("%s=%s", label, value));
  }

  private static final void add (StringBuilder sb, String label, int value) {
    add(sb, label, Integer.toString(value));
  }

  private static final void add (StringBuilder sb, String label, int value, Map<Integer, String> names) {
    String name = names.get(value);

    if (name != null) {
      add(sb, label, name);
    } else {
      add(sb, label, value);
    }
  }

  private static void log (String message) {
    Log.d(LOG_TAG, message);
  }

  private static void log (String label, String data) {
    log((label + ": " + data));
  }

  private static void log (AccessibilityNodeInfo node, String name, boolean descend) {
    StringBuilder sb = new StringBuilder();

    {
      String object = node.getClassName().toString();
      int index = object.lastIndexOf(".");
      sb.append(object.substring(index+1));
    }

    {
      CharSequence text = node.getText();

      if (ScreenUtilities.isEditable(node)) {
        if (text == null) text = "";
      } else if ((text != null) && (text.length() == 0)) {
        text = null;
      }

      if (text != null) {
        sb.append(' ');
        sb.append('"');
        sb.append(text);
        sb.append('"');
      }
    }

    {
      CharSequence description = node.getContentDescription();

      if ((description != null) && (description.length() > 0)) {
        sb.append(' ');
        sb.append('(');
        sb.append(description);
        sb.append(')');
      }
    }

    {
      AccessibilityNodeInfo parent = node.getParent();

      if (parent != null) {
        parent.recycle();
        parent = null;
      } else {
        add(sb, "root");
      }
    }

    {
      int count = node.getChildCount();
      if (count > 0) add(sb, "cld", count);
    }

    if (ApplicationUtilities.haveJellyBean) {
      add(sb, !node.isVisibleToUser(), "inv");
    }

    add(sb, !node.isEnabled(), "dsb");
    add(sb, node.isSelected(), "sld");
    add(sb, node.isScrollable(), "scr");
    add(sb, node.isFocusable(), "ifb");
    add(sb, node.isFocused(), "ifd");

    if (ApplicationUtilities.haveJellyBean) {
      add(sb, node.isAccessibilityFocused(), "afd");
    }

    add(sb, node.isClickable(), "clb");
    add(sb, node.isLongClickable(), "lcb");
    add(sb, node.isCheckable(), "ckb");
    add(sb, node.isChecked(), "ckd");
    add(sb, node.isPassword(), "pwd");

    if (ApplicationUtilities.haveJellyBeanMR2) {
      add(sb, ScreenUtilities.isEditable(node), "edb");
    }

    {
      int actions = node.getActions();

      if (ApplicationUtilities.haveJellyBean) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_CLICK) != 0), "clk");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_LONG_CLICK) != 0), "lck");
      }

      if (ApplicationUtilities.haveJellyBean) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_SCROLL_FORWARD) != 0), "scf");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD) != 0), "scb");
      }

      if (ApplicationUtilities.haveJellyBean) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_NEXT_AT_MOVEMENT_GRANULARITY) != 0), "mvn");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY) != 0), "mvp");
      }

      if (ApplicationUtilities.haveJellyBean) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_NEXT_HTML_ELEMENT) != 0), "mhn");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_PREVIOUS_HTML_ELEMENT) != 0), "mhp");
      }

      if (ApplicationUtilities.haveIceCreamSandwich) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_SELECT) != 0), "sel");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_CLEAR_SELECTION) != 0), "slc");
      }

      if (ApplicationUtilities.haveIceCreamSandwich) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_FOCUS) != 0), "ifs");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_CLEAR_FOCUS) != 0), "ifc");
      }

      if (ApplicationUtilities.haveJellyBean) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS) != 0), "afs");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS) != 0), "afc");
      }

      if (ApplicationUtilities.haveJellyBeanMR2) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_SET_SELECTION) != 0), "sls");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_COPY) != 0), "cpy");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_CUT) != 0), "cut");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_PASTE) != 0), "pst");
      }

      if (ApplicationUtilities.haveKitkat) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_DISMISS) != 0), "dsm");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_COLLAPSE) != 0), "col");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_EXPAND) != 0), "exp");
      }

      if (ApplicationUtilities.haveLollipop) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_SET_TEXT) != 0), "txs");
      }
    }

    if (ApplicationUtilities.haveJellyBeanMR1) {
      AccessibilityNodeInfo subnode = node.getLabelFor();

      if (subnode != null) {
        add(sb, "lbf", getText(subnode));
        subnode.recycle();
        subnode = null;
      }
    }

    if (ApplicationUtilities.haveJellyBeanMR1) {
      AccessibilityNodeInfo subnode = node.getLabeledBy();

      if (subnode != null) {
        add(sb, "lbd", getText(subnode));
        subnode.recycle();
        subnode = null;
      }
    }

    {
      Rect location = new Rect();
      node.getBoundsInScreen(location);
      add(sb, location.toShortString());
    }

    add(sb, "obj", node.getClassName());
    add(sb, "pkg", node.getPackageName());
    add(sb, "win", node.getWindowId());
    log(name, sb.toString());

    if (descend) {
      int childCount = node.getChildCount();

      for (int childIndex=0; childIndex<childCount; childIndex+=1) {
        AccessibilityNodeInfo child = node.getChild(childIndex);

        if (child != null) {
          log(child, (name + "." + childIndex), true);
          child.recycle();
          child = null;
        }
      }
    }
  }

  private static void log (AccessibilityNodeInfo root) {
    log("begin node tree");
    log(root, "root", true);
    log("end node tree");
  }

  private static final Map<Integer, String> windowTypeNames =
               new HashMap<Integer, String>()
  {
    {
      if (ApplicationUtilities.haveLollipop) {
        put(AccessibilityWindowInfo.TYPE_APPLICATION, "app");
        put(AccessibilityWindowInfo.TYPE_INPUT_METHOD, "im");
        put(AccessibilityWindowInfo.TYPE_SYSTEM, "sys");
      }
    }
  };

  private static void log (AccessibilityWindowInfo window, String name, boolean descend, boolean nodes) {
    StringBuilder sb = new StringBuilder();
    add(sb, "id", window.getId());

    {
      AccessibilityWindowInfo parent = window.getParent();

      if (parent != null) {
        parent.recycle();
        parent = null;
      } else {
        add(sb, "root");
      }
    }

    {
      int count = window.getChildCount();
      if (count > 0) add(sb, "cld", count);
    }

    add(sb, "type", window.getType(), windowTypeNames);
    add(sb, "layer", window.getLayer());
    add(sb, window.isActive(), "act");
    add(sb, window.isFocused(), "ifd");
    add(sb, window.isAccessibilityFocused(), "afd");

    {
      Rect location = new Rect();
      window.getBoundsInScreen(location);
      add(sb, location.toShortString());
    }

    log(name, sb.toString());

    if (nodes) {
      AccessibilityNodeInfo root = window.getRoot();

      if (root != null) {
        log(root);
        root.recycle();
        root = null;
      }
    }

    if (descend) {
      int childCount = window.getChildCount();

      for (int childIndex=0; childIndex<childCount; childIndex+=1) {
        AccessibilityWindowInfo child = window.getChild(childIndex);

        if (child != null) {
          log(child, (name + '.' + childIndex), true, nodes);
          child.recycle();
          child = null;
        }
      }
    }
  }

  public static void log () {
    log("begin screen log");

    if (ApplicationUtilities.haveLollipop) {
      int index = 0;

      for (AccessibilityWindowInfo window : BrailleService.getBrailleService().getWindows()) {
        log(window, ("window." + index), true, true);
        index += 1;
      }
    } else if (ApplicationUtilities.haveJellyBean) {
      AccessibilityNodeInfo root = BrailleService.getBrailleService().getRootInActiveWindow();

      if (root != null) {
        log(root);
        root.recycle();
        root = null;
      }
    }

    log("end screen log");
  }

  public static void log (AccessibilityEvent event) {
    log("accessibility event", event.toString());

    {
      AccessibilityNodeInfo node = event.getSource();

      if (node != null) {
        log(node, "event node", false);

        if (ApplicationUtilities.haveLollipop) {
          AccessibilityWindowInfo window = node.getWindow();

          if (window != null) {
            log(window, "window", true, true);
            window.recycle();
            window = null;
          }
        } else {
          AccessibilityNodeInfo root = ScreenUtilities.findRootNode(node);

          if (root != null) {
            log(root);
            root.recycle();
            root = null;
          }
        }

        node.recycle();
        node = null;
      }
    }
  }
}

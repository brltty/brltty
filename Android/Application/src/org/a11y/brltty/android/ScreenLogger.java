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

import android.os.Build;
import android.util.Log;

import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityWindowInfo;

import android.graphics.Rect;

public class ScreenLogger {
  private final String logTag;

  public ScreenLogger (String tag) {
    logTag = tag;
  }

  public final void log (String message) {
    Log.d(logTag, message);
  }

  public final void log (String label, String data) {
    log((label + ": " + data));
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
    add(sb, String.format("%s=%d", label, value));
  }

  private static final void add (StringBuilder sb, String label, int value, Map<Integer, String> names) {
    String name = names.get(value);

    if (name != null) {
      add(sb, label, name);
    } else {
      add(sb, label, value);
    }
  }

  public final void logNode (AccessibilityNodeInfo node, String name) {
    StringBuilder sb = new StringBuilder();

    {
      String object = node.getClassName().toString();
      int index = object.lastIndexOf(".");
      sb.append(object.substring(index+1));
    }

    {
      String text = ScreenUtilities.getNodeText(node);

      if ((text != null) && (text.length() > 0)) {
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

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      add(sb, !node.isVisibleToUser(), "inv");
    }

    add(sb, !node.isEnabled(), "dsb");
    add(sb, node.isSelected(), "sld");
    add(sb, node.isScrollable(), "scb");
    add(sb, node.isFocusable(), "ifb");
    add(sb, node.isFocused(), "ifd");

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      add(sb, node.isAccessibilityFocused(), "afd");
    }

    add(sb, node.isClickable(), "clb");
    add(sb, node.isLongClickable(), "lcb");
    add(sb, node.isCheckable(), "ckb");
    add(sb, node.isChecked(), "ckd");
    add(sb, node.isPassword(), "pwd");

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
      add(sb, node.isEditable(), "edb");
    }

    {
      int actions = node.getActions();

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_CLICK) != 0), "clk");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_LONG_CLICK) != 0), "lck");
      }

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_SCROLL_FORWARD) != 0), "scf");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD) != 0), "scb");
      }

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_NEXT_AT_MOVEMENT_GRANULARITY) != 0), "mvn");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY) != 0), "mvp");
      }

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_NEXT_HTML_ELEMENT) != 0), "mhn");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_PREVIOUS_HTML_ELEMENT) != 0), "mhp");
      }

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_SELECT) != 0), "sls");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_CLEAR_SELECTION) != 0), "slc");
      }

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_FOCUS) != 0), "ifs");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_CLEAR_FOCUS) != 0), "ifc");
      }

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS) != 0), "afs");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS) != 0), "afc");
      }

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_SET_SELECTION) != 0), "sel");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_COPY) != 0), "cpy");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_CUT) != 0), "cut");
        add(sb, ((actions & AccessibilityNodeInfo.ACTION_PASTE) != 0), "pst");
      }
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR1)) {
      AccessibilityNodeInfo subnode = node.getLabelFor();

      if (subnode != null) {
        add(sb, "lbf", ScreenUtilities.getNodeText(subnode));
        subnode.recycle();
        subnode = null;
      }
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR1)) {
      AccessibilityNodeInfo subnode = node.getLabeledBy();

      if (subnode != null) {
        add(sb, "lbd", ScreenUtilities.getNodeText(subnode));
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
  }

  private final void logTree (AccessibilityNodeInfo root, String name) {
    if (root != null) {
      logNode(root, name);
      int childCount = root.getChildCount();

      for (int childIndex=0; childIndex<childCount; childIndex+=1) {
        AccessibilityNodeInfo child = root.getChild(childIndex);

        if (child != null) {
          logTree(child, (name + "." + childIndex));
          child.recycle();
          child = null;
        }
      }
    }
  }

  public final void logTree (AccessibilityNodeInfo root) {
    logTree(root, "node");
  }

  private static final Map<Integer, String> windowTypeNames =
               new HashMap<Integer, String>()
  {
    {
      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.LOLLIPOP)) {
        put(AccessibilityWindowInfo.TYPE_APPLICATION, "app");
        put(AccessibilityWindowInfo.TYPE_INPUT_METHOD, "im");
        put(AccessibilityWindowInfo.TYPE_SYSTEM, "sys");
      }
    }
  };

  private final void logTree (AccessibilityWindowInfo root, String name) {
    StringBuilder sb = new StringBuilder();
    add(sb, "id", root.getId());

    {
      AccessibilityWindowInfo parent = root.getParent();

      if (parent != null) {
        parent.recycle();
        parent = null;
      } else {
        add(sb, "root");
      }
    }

    {
      int count = root.getChildCount();
      if (count > 0) add(sb, "cld", count);
    }

    add(sb, "layer", root.getLayer());
    add(sb, "type", root.getType(), windowTypeNames);
    add(sb, root.isActive(), "act");
    add(sb, root.isFocused(), "ifd");
    add(sb, root.isAccessibilityFocused(), "afd");

    {
      Rect location = new Rect();
      root.getBoundsInScreen(location);
      add(sb, location.toShortString());
    }

    log(name, sb.toString());
    logTree(root.getRoot(), "root");

    {
      int count = root.getChildCount();

      for (int index=0; index<count; index+=1) {
        AccessibilityWindowInfo child = root.getChild(index);

        if (child != null) {
          logTree(child, (name + '.' + index));
          child.recycle();
        }
      }
    }
  }

  public final void logScreen () {
    log("begin screen log");

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.LOLLIPOP)) {
      int index = 0;

      for (AccessibilityWindowInfo window : BrailleService.getBrailleService().getWindows()) {
        logTree(window, ("window." + index));
        index += 1;
      }
    } else if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      logTree(BrailleService.getBrailleService().getRootInActiveWindow(), "root");
    }

    log("end screen log");
  }

  public final void logEvent (AccessibilityEvent event) {
    log("accessibility event", event.toString());

    {
      AccessibilityNodeInfo node = event.getSource();

      if (node != null) {
        logNode(node, "event node");

        {
          AccessibilityNodeInfo root = ScreenUtilities.findRootNode(node);

          if (root != null) {
            logTree(root);
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

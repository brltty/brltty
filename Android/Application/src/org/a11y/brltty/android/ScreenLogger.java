/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brltty.android;

import java.util.List;
import java.util.ArrayList;

import android.os.Build;
import android.util.Log;

import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;

public class ScreenLogger {
  private final String logTag;

  private static final String ROOT_NODE_NAME = "root";

  public static String getRootNodeName () {
    return ROOT_NODE_NAME;
  }

  public final void log (String message) {
    Log.d(logTag, message);
  }

  public final void addValue (
    List<CharSequence> values,
    boolean condition,
    CharSequence onValue,
    CharSequence offValue
  ) {
    CharSequence value = condition? onValue: offValue;

    if (value != null) {
      values.add(value);
    }
  }

  public final void addValue (
    List<CharSequence> values,
    boolean condition,
    CharSequence onValue
  ) {
    addValue(values, condition, onValue, null);
  }

  public final void addValue (
    List<CharSequence> values,
    CharSequence value
  ) {
    addValue(values, true, value);
  }

  public final void logProperty (CharSequence name, CharSequence ... values) {
    if (values.length > 0) {
      StringBuilder sb = new StringBuilder();

      sb.append(name);
      sb.append(':');

      for (CharSequence value : values) {
        sb.append(' ');

        if (value == null) {
          sb.append("null");
        } else if (value.length() == 0) {
          sb.append("nil");
        } else {
          sb.append(value);
        }
      }

      log(sb.toString());
    }
  }

  public final void logProperty (CharSequence name, List<CharSequence> values) {
    logProperty(name, values.toArray(new CharSequence[values.size()]));
  }

  public final void logProperty (CharSequence name, int value) {
    logProperty(name, Integer.toString(value));
  }

  public final void logNodeProperties (AccessibilityNodeInfo node) {
    {
      List<CharSequence> values = new ArrayList<CharSequence>();

      {
        AccessibilityNodeInfo parent = node.getParent();

        if (parent != null) {
          parent.recycle();
          parent = null;
        } else {
          addValue(values, ROOT_NODE_NAME);
        }
      }

      {
        int count = node.getChildCount();
        addValue(values, count>0, "cld=" + count);
      }

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        addValue(values, node.isVisibleToUser(), "vis", "inv");
      }

      addValue(values, node.isEnabled(), "enb", "dsb");
      addValue(values, node.isSelected(), "sld");
      addValue(values, node.isScrollable(), "scb");
      addValue(values, node.isFocusable(), "ifb");
      addValue(values, node.isFocused(), "ifd");

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
        addValue(values, node.isAccessibilityFocused(), "afd");
      }

      addValue(values, node.isClickable(), "clb");
      addValue(values, node.isLongClickable(), "lcb");
      addValue(values, node.isCheckable(), "ckb");
      addValue(values, node.isChecked(), "ckd");
      addValue(values, node.isPassword(), "pwd");

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
        addValue(values, node.isEditable(), "edb");
      }

      logProperty("flgs", values);
    }

    {
      List<CharSequence> values = new ArrayList<CharSequence>();

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.LOLLIPOP)) {
        List<AccessibilityNodeInfo.AccessibilityAction> actions = node.getActionList();

        for (AccessibilityNodeInfo.AccessibilityAction action : actions) {
          addValue(values, action.getLabel());
        }
      } else {
        int actions = node.getActions();

        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_CLICK) != 0), "clk");
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_LONG_CLICK) != 0), "lng");
        }

        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_SCROLL_FORWARD) != 0), "scf");
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD) != 0), "scb");
        }

        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_NEXT_AT_MOVEMENT_GRANULARITY) != 0), "mvn");
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY) != 0), "mvp");
        }

        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_NEXT_HTML_ELEMENT) != 0), "mhn");
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_PREVIOUS_HTML_ELEMENT) != 0), "mhp");
        }

        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_SELECT) != 0), "sls");
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_CLEAR_SELECTION) != 0), "slc");
        }

        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_FOCUS) != 0), "ifs");
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_CLEAR_FOCUS) != 0), "ifc");
        }

        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS) != 0), "afs");
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS) != 0), "afc");
        }

        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR2)) {
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_SET_SELECTION) != 0), "sel");
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_COPY) != 0), "cpy");
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_CUT) != 0), "cut");
          addValue(values, ((actions & AccessibilityNodeInfo.ACTION_PASTE) != 0), "pst");
        }
      }

      logProperty("actn", values);
    }

    logProperty("desc", node.getContentDescription());
    logProperty("text", node.getText());
    logProperty("obj", node.getClassName());
    logProperty("app", node.getPackageName());

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR1)) {
      AccessibilityNodeInfo subnode = node.getLabelFor();

      if (subnode != null) {
        logProperty("lbl", ScreenUtilities.getNodeText(subnode));
        subnode.recycle();
      }
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR1)) {
      AccessibilityNodeInfo subnode = node.getLabeledBy();

      if (subnode != null) {
        logProperty("lbd", ScreenUtilities.getNodeText(subnode));
        subnode.recycle();
      }
    }

    {
      Rect location = new Rect();
      node.getBoundsInScreen(location);
      logProperty("locn", location.toShortString());
    }
  }

  public final void logTreeProperties (
    AccessibilityNodeInfo root,
    CharSequence name
  ) {
    logProperty("name", name);

    if (root != null) {
      logNodeProperties(root);

      {
        int childCount = root.getChildCount();

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          AccessibilityNodeInfo child = root.getChild(childIndex);

          if (child != null) {
            logTreeProperties(child, name + "." + childIndex);

            child.recycle();
            child = null;
          }
        }
      }
    }
  }

  public final void logTreeProperties (AccessibilityNodeInfo root) {
    logTreeProperties(root, ROOT_NODE_NAME);
  }

  public final void logNodeIdentity (AccessibilityNodeInfo node, String description) {
    StringBuilder sb = new StringBuilder();

    sb.append(description);
    sb.append(": ");

    if (node == null) {
      sb.append("null");
    } else {
      String text = ScreenUtilities.getNodeText(node);

      if ((text != null) && (text.length() > 0)) {
        sb.append(text);
      } else {
        sb.append(node.getClassName());
      }

      Rect location = new Rect();
      node.getBoundsInScreen(location);
      sb.append(' ');
      sb.append(location.toShortString());
    }

    log(sb.toString());
  }

  public final void logEvent (AccessibilityEvent event) {
    logProperty("accessibility event", event.toString());

    AccessibilityNodeInfo node = event.getSource();
    if (node != null) {
      logProperty("current window", node.getWindowId());
      logNodeIdentity(node, "event node");

      AccessibilityNodeInfo root = ScreenUtilities.findRootNode(node);
      if (root != null) {
        logTreeProperties(root);
        root.recycle();
        root = null;
      }

      node.recycle();
      node = null;
    }
  }

  public ScreenLogger (String tag) {
    logTag = tag;
  }
}

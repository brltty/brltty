/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2020 by The BRLTTY Developers.
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

import android.view.accessibility.AccessibilityNodeInfo;
import android.os.Bundle;

import java.util.Map;
import java.util.HashMap;

public enum ChromeRole {
  anchor(),
  button("btn"),
  caption("cap"),
  cell("col"),
  checkBox(),
  columnHeader("hdr"),
  form("frm"),

  heading("hdg",
    "heading 1", "hd1",
    "heading 2", "hd2",
    "heading 3", "hd3",
    "heading 4", "hd4",
    "heading 5", "hd5",
    "heading 6", "hd6"
  ),

  lineBreak(),
  link("lnk"),
  paragraph(),
  popUpButton("pop"),
  radioButton(),
  rootWebArea(),
  row("row"),
  splitter("--------"),
  staticText(),
  table("tbl"),

  textField("txt",
    new LabelMaker() {
      @Override
      public String makeLabel (AccessibilityNodeInfo node) {
        if ((node.getActions() & AccessibilityNodeInfo.ACTION_EXPAND) == 0) return "";
        if (node.isPassword()) return "pwd";
        return null;
      }
    }
  ),

  ; // end of enumeration

  public final static String EXTRA_CHROME_ROLE = "AccessibilityNodeInfo.chromeRole";
  public final static String EXTRA_ROLE_DESCRIPTION = "AccessibilityNodeInfo.roleDescription";
  public final static String EXTRA_HINT = "AccessibilityNodeInfo.hint";
  public final static String EXTRA_TARGET_URL = "AccessibilityNodeInfo.targetUrl";

  private interface LabelMaker {
    public String makeLabel (AccessibilityNodeInfo node);
  }

  private final String genericLabel;
  private final LabelMaker labelMaker;
  private final Map<String, String> descriptionLabels;

  ChromeRole (String generic, LabelMaker maker, String... descriptions) {
    genericLabel = generic;
    labelMaker = maker;

    {
      int count = descriptions.length;

      if (count > 0) {
        descriptionLabels = new HashMap<String, String>();
        int index = 0;

        while (index < count) {
          String description = descriptions[index++];
          String label = descriptions[index++];
          descriptionLabels.put(description, label);
        }
      } else {
        descriptionLabels = null;
      }
    }
  }

  ChromeRole (String generic, String... descriptions) {
    this(generic, null, descriptions);
  }

  ChromeRole () {
    this(null);
  }

  private final static Bundle getExtras (AccessibilityNodeInfo node) {
    if (node != null) {
      if (APITests.haveKitkat) {
        return node.getExtras();
      }
    }

    return null;
  }

  public static String getStringExtra (AccessibilityNodeInfo node, String extra) {
    Bundle extras = getExtras(node);
    if (extras == null) return null;
    return extras.getString(extra);
  }

  public static ChromeRole getChromeRole (AccessibilityNodeInfo node) {
    String name = getStringExtra(node, EXTRA_CHROME_ROLE);
    if (name == null) return null;

    try {
      return ChromeRole.valueOf(name);
    } catch (IllegalArgumentException exception) {
      return null;
    }
  }

  public static String getLabel (AccessibilityNodeInfo node) {
    if (node == null) return null;

    ChromeRole role = getChromeRole(node);
    if (role == null) return null;

    if (role.labelMaker != null) {
      String label = role.labelMaker.makeLabel(node);
      if (label != null) return label;
    }

    if (role.descriptionLabels != null) {
      final String description = getStringExtra(node, EXTRA_ROLE_DESCRIPTION);

      if (description != null) {
        String label = role.descriptionLabels.get(description);
        if (label != null) return label;
      }
    }

    return role.genericLabel;
  }
}

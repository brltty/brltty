/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
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

import android.util.Log;
import android.os.Bundle;
import android.view.accessibility.AccessibilityNodeInfo;

import java.util.Map;
import java.util.HashMap;

public enum ChromeRole {
  cell("col",
    new LabelMaker() {
      @Override
      public String makeLabel (ChromeRole role, AccessibilityNodeInfo node) {
        return makeCoordinatesLabel(role, node);
      }
    }
  ),

  columnHeader("hdr",
    new LabelMaker() {
      @Override
      public String makeLabel (ChromeRole role, AccessibilityNodeInfo node) {
        return makeCoordinatesLabel(role, node);
      }
    }
  ),

  heading("hdg",
    "heading 1", "hd1",
    "heading 2", "hd2",
    "heading 3", "hd3",
    "heading 4", "hd4",
    "heading 5", "hd5",
    "heading 6", "hd6"
  ),

  link("lnk",
    new LabelMaker() {
      @Override
      public String makeLabel (ChromeRole role, AccessibilityNodeInfo node) {
        StringBuilder label = new StringBuilder(role.genericLabel);

        if (false) {
          String url = ScreenUtilities.getStringExtra(node, EXTRA_TARGET_URL);

          if ((url != null) && !url.isEmpty()) {
            label.append(' ').append(url);
          }
        }

        return label.toString();
      }
    }
  ),

  list("lst",
    new LabelMaker() {
      @Override
      public String makeLabel (ChromeRole role, AccessibilityNodeInfo node) {
        StringBuilder label = new StringBuilder(role.genericLabel);

        if (APITests.haveKitkat) {
          AccessibilityNodeInfo.CollectionInfo collection = node.getCollectionInfo();

          if (collection != null) {
            int columns = collection.getColumnCount();
            if (columns == 0) columns = 1;

            int rows = collection.getRowCount();
            if (rows == 0) rows = 1;

            label.append(' ')
                 .append(columns)
                 .append('x')
                 .append(rows)
                 ;
          }
        }

        return label.toString();
      }
    }
  ),

  listItem("lsi",
    new LabelMaker() {
      @Override
      public String makeLabel (ChromeRole role, AccessibilityNodeInfo node) {
        StringBuilder label = new StringBuilder(role.genericLabel);

        if (APITests.haveKitkat) {
          AccessibilityNodeInfo.CollectionItemInfo item = node.getCollectionItemInfo();

          if (item != null) {
            label.append(' ')
                 .append(item.getColumnIndex() + 1)
                 .append('@')
                 .append(item.getRowIndex() + 1)
                 ;
          }
        }

        return label.toString();
      }
    }
  ),

  table("tbl",
    new LabelMaker() {
      @Override
      public String makeLabel (ChromeRole role, AccessibilityNodeInfo node) {
        return makeDimensionsLabel(role, node);
      }
    }
  ),

  textField("txt",
    new LabelMaker() {
      @Override
      public String makeLabel (ChromeRole role, AccessibilityNodeInfo node) {
        if ((node.getActions() & AccessibilityNodeInfo.ACTION_EXPAND) == 0) return "";
        if (node.isPassword()) return "pwd";
        return null;
      }
    }
  ),

  anchor(),
  button("btn"),
  caption("cap"),
  checkBox(),
  form("frm"),
  genericContainer(""),
  lineBreak(),
  listMarker("lsm"),
  paragraph(),
  popUpButton("pop"),
  radioButton(),
  rootWebArea(),
  row("row"),
  splitter("--------"),
  staticText(),
  ; // end of enumeration

  private final static String LOG_TAG = ChromeRole.class.getName();

  public final static String EXTRA_CHROME_ROLE = "AccessibilityNodeInfo.chromeRole";
  public final static String EXTRA_ROLE_DESCRIPTION = "AccessibilityNodeInfo.roleDescription";
  public final static String EXTRA_HINT = "AccessibilityNodeInfo.hint";
  public final static String EXTRA_TARGET_URL = "AccessibilityNodeInfo.targetUrl";

  private interface LabelMaker {
    public String makeLabel (ChromeRole role, AccessibilityNodeInfo node);
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

  public static String makeCoordinatesLabel (ChromeRole role, AccessibilityNodeInfo node) {
    if (APITests.haveKitkat) {
      AccessibilityNodeInfo.CollectionItemInfo item = node.getCollectionItemInfo();

      if (item != null) {
        StringBuilder label = new StringBuilder(role.genericLabel);
        label.append(' ')
             .append(item.getColumnIndex() + 1)
             .append('@')
             .append(item.getRowIndex() + 1)
             ;
        return label.toString();
      }
    }

    return null;
  }

  public static String makeDimensionsLabel (ChromeRole role, AccessibilityNodeInfo node) {
    if (APITests.haveKitkat) {
      AccessibilityNodeInfo.CollectionInfo collection = node.getCollectionInfo();

      if (collection != null) {
        StringBuilder label = new StringBuilder(role.genericLabel);
        label.append(' ')
             .append(collection.getColumnCount())
             .append('x')
             .append(collection.getRowCount())
             ;

        return label.toString();
      }
    }

    return null;
  }

  public static ChromeRole getChromeRole (AccessibilityNodeInfo node) {
    String name = ScreenUtilities.getStringExtra(node, EXTRA_CHROME_ROLE);
    if (name == null) return null;
    if (name.isEmpty()) return null;

    try {
      return ChromeRole.valueOf(name);
    } catch (IllegalArgumentException exception) {
      Log.w(LOG_TAG, ("unrecognized Chrome role: " + name));
    }

    return null;
  }

  public static String getLabel (AccessibilityNodeInfo node) {
    if (node == null) return null;

    ChromeRole role = getChromeRole(node);
    if (role == null) return null;

    if (role.labelMaker != null) {
      String label = role.labelMaker.makeLabel(role, node);
      if (label != null) return label;
    }

    if (role.descriptionLabels != null) {
      final String description = ScreenUtilities.getStringExtra(node, EXTRA_ROLE_DESCRIPTION);

      if (description != null) {
        String label = role.descriptionLabels.get(description);
        if (label != null) return label;
      }
    }

    return role.genericLabel;
  }
}

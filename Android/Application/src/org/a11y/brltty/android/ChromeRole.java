/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2019 by The BRLTTY Developers.
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
  heading("hdg",
    "heading 1", "hd1",
    "heading 2", "hd2",
    "heading 3", "hd3",
    "heading 4", "hd4",
    "heading 5", "hd5",
    "heading 6", "hd6"
  ),

  link("lnk"),
  splitter("--------"),
  ; // end of enumeration

  private final String genericLabel;
  private final Map<String, String> descriptionLabels;

  ChromeRole (String generic, String... descriptions) {
    genericLabel = generic;

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

  public static String getLabel (Bundle extras) {
    if (extras == null) return null;

    final String name = extras.getString("AccessibilityNodeInfo.chromeRole");
    if (name == null) return null;

    final ChromeRole role;
    try {
      role = ChromeRole.valueOf(name);
    } catch (IllegalArgumentException exception) {
      return null;
    }

    if (role.descriptionLabels != null) {
      final String description = extras.getString("AccessibilityNodeInfo.roleDescription");

      if (description != null) {
        final String label = role.descriptionLabels.get(description);
        if (label != null) return label;
      }
    }

    return role.genericLabel;
  }

  public static String getLabel (AccessibilityNodeInfo node) {
    if (node != null) {
      if (ApplicationUtilities.haveKitkat) {
        return getLabel(node.getExtras());
      }
    }

    return null;
  }
}

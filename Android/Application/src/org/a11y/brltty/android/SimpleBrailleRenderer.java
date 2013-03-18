/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brltty.android;

import java.util.List;

import android.graphics.Rect;

import android.accessibilityservice.AccessibilityService;

public class SimpleBrailleRenderer extends BrailleRenderer {
  @Override
  public void renderElements (
    List<CharSequence> rows,
    ScreenElementList elements
  ) {
    elements.sortByVisualLocation();
    elements.groupByContainer();

    elements.addAtTop("Notifications", AccessibilityService.GLOBAL_ACTION_NOTIFICATIONS);
    elements.addAtTop("Quick Settings", AccessibilityService.GLOBAL_ACTION_QUICK_SETTINGS);

    elements.addAtBottom("Back", AccessibilityService.GLOBAL_ACTION_BACK);
    elements.addAtBottom("Home", AccessibilityService.GLOBAL_ACTION_HOME);
    elements.addAtBottom("Recent Apps", AccessibilityService.GLOBAL_ACTION_RECENTS);

    for (ScreenElement element : elements) {
      String text = element.getRenderedText();

      if (text.length() > 0) {
        int rowIndex = rows.size();
        element.setBrailleLocation(new Rect(0, rowIndex, text.length()-1, rowIndex));
        rows.add(text);
      }
    }
  }

  public SimpleBrailleRenderer () {
    super();
  }
}

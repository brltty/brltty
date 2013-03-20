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

public class SimpleBrailleRenderer extends BrailleRenderer {
  @Override
  public void renderScreenElements (
    List<CharSequence> rows,
    ScreenElementList elements
  ) {
    elements.sortByVisualLocation();
    elements.groupByContainer();
    addVirtualElements(elements);

    for (ScreenElement element : elements) {
      String text = element.getBrailleText();

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

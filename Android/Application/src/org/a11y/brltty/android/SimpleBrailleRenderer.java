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
  public void renderScreenElements (List<CharSequence> rows, ScreenElementList elements) {
    elements.sortByVisualLocation();
    elements.groupByContainer();
    addVirtualElements(elements);

    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    boolean wasVirtual = false;

    for (ScreenElement element : elements) {
      String text = element.getBrailleText();

      boolean isVirtual = element.getVisualLocation() == null;
      boolean append = wasVirtual && isVirtual;
      wasVirtual = isVirtual;

      if (text.length() > 0) {
        List<CharSequence> lines = makeTextLines(text);
        int width = getTextWidth(lines);

        if (append) {
          left = right + 3;
          int row = top;

          for (CharSequence line : lines) {
            StringBuilder sb = new StringBuilder();
            while (row >= rows.size()) rows.add("");
            sb.append(rows.get(row));
            while (sb.length() < left) sb.append(' ');
            sb.append(line);
            rows.set(row++, sb.toString());
          }
        } else {
          left = 0;
          top = rows.size();
          rows.addAll(lines);
        }

        right = left + width - 1;
        bottom = top + lines.size() - 1;
        element.setBrailleLocation(new Rect(left, top, right, bottom));
      }
    }
  }

  public SimpleBrailleRenderer () {
    super();
  }
}

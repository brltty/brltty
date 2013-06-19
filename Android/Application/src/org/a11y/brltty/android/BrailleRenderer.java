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

import android.util.Log;

import android.os.Build;
import android.accessibilityservice.AccessibilityService;
import android.view.KeyEvent;

import android.graphics.Rect;

public abstract class BrailleRenderer {
  private static final String LOG_TAG = BrailleRenderer.class.getName();

  private static volatile BrailleRenderer brailleRenderer = new ListBrailleRenderer();

  public static BrailleRenderer getBrailleRenderer () {
    return brailleRenderer;
  }

  public static void setBrailleRenderer (BrailleRenderer renderer) {
    brailleRenderer = renderer;
  }

  public static void setBrailleRenderer (String type) {
    StringBuilder name = new StringBuilder(BrailleRenderer.class.getName());
    int index = name.lastIndexOf(".");
    name.insert(index+1, type);
    setBrailleRenderer((BrailleRenderer)LanguageUtilities.newInstance(name.toString()));
  }

  protected abstract void setBrailleLocations (ScreenElementList elements);

  protected static int getTextWidth (String[] lines) {
    int width = 1;

    for (String line : lines) {
      width = Math.max(width, line.length());
    }

    return width;
  }

  private void addVirtualElements (ScreenElementList elements) {
    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      elements.addAtTop(
        R.string.VIRTUAL_BUTTON_NOTIFICATIONS,
        AccessibilityService.GLOBAL_ACTION_NOTIFICATIONS
      );
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      elements.addAtTop(
        R.string.VIRTUAL_BUTTON_QUICK_SETTINGS,
        AccessibilityService.GLOBAL_ACTION_QUICK_SETTINGS
      );
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      elements.addAtBottom(
        R.string.VIRTUAL_BUTTON_BACK,
        AccessibilityService.GLOBAL_ACTION_BACK
      );
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      elements.addAtBottom(
        R.string.VIRTUAL_BUTTON_HOME,
        AccessibilityService.GLOBAL_ACTION_HOME
      );
    }

    if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      elements.addAtBottom(
        R.string.VIRTUAL_BUTTON_RECENT_APPS,
        AccessibilityService.GLOBAL_ACTION_RECENTS
      );
    }
  }

  private void setScreenRegion (List<CharSequence> rows, Rect location, String[] lines) {
    int width = location.right - location.left + 1;

    while (rows.size() <= location.bottom) {
      rows.add("");
    }

    for (int rowIndex=location.top; rowIndex<=location.bottom; rowIndex+=1) {
      StringBuilder row = new StringBuilder(rows.get(rowIndex));
      while (row.length() <= location.right) row.append(' ');

      int lineIndex = rowIndex - location.top;
      String line = (lineIndex < lines.length)? lines[lineIndex]: "";
      if (line.length() > width) line = line.substring(0, width);

      int end = location.left + line.length();
      row.replace(location.left, end, line);
      while (end <= location.right) row.setCharAt(end++, ' ');

      rows.set(rowIndex, row.toString());
    }
  }

  public final void renderScreenElements (List<CharSequence> rows, ScreenElementList elements) {
    setBrailleLocations(elements);
    addVirtualElements(elements);

    boolean wasVirtual = false;
    int horizontalOffset = 0;
    int verticalOffset = 0;

    for (ScreenElement element : elements) {
      boolean isVirtual = element.getVisualLocation() == null;

      if (isVirtual != wasVirtual) {
        horizontalOffset = 0;
        verticalOffset = rows.size();
        wasVirtual = isVirtual;
      }

      if (isVirtual) {
        String[] text = element.getBrailleText();
        int left = horizontalOffset;
        int top = 0;
        int right = left + getTextWidth(text) - 1;
        int bottom = top + text.length - 1;
        element.setBrailleLocation(left, top, right, bottom);
        horizontalOffset = right + 1 + ApplicationParameters.BRAILLE_COLUMN_SPACING;
      }

      Rect location = element.getBrailleLocation();
      if (location != null) {
        location.top += verticalOffset;
        location.bottom += verticalOffset;
        setScreenRegion(rows, location, element.getBrailleText());
      }
    }
  }

  protected BrailleRenderer () {
  }
}

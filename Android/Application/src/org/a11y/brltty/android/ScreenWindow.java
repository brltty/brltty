/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

import android.graphics.Point;
import android.graphics.Rect;

public class ScreenWindow {
  private final int windowIdentifier;

  private final Point windowSize = new Point();
  private final Rect windowRectangle = new Rect();

  public final int getWindowIdentifier () {
    return windowIdentifier;
  }

  public int getWindowWidth () {
    return windowSize.x;
  }

  public int getWindowHeight () {
    return windowSize.y;
  }

  public boolean contains (Rect location) {
    return windowRectangle.contains(location);
  }

  public ScreenWindow (int identifier) {
    windowIdentifier = identifier;

    ApplicationUtilities.getWindowManager().getDefaultDisplay().getSize(windowSize);
    windowRectangle.set(0, 0, getWindowWidth(), getWindowHeight());
  }
}

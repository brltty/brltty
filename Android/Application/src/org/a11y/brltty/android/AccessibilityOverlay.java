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

import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;

import android.view.View;
import android.view.Gravity;
import android.graphics.PixelFormat;

import android.view.LayoutInflater;

public abstract class AccessibilityOverlay {
  private final WindowManager windowManager = BrailleService.getWindowManager();
  private final LayoutParams layoutParameters = new LayoutParams();
  private View currentView = null;

  protected AccessibilityOverlay () {
    layoutParameters.format = PixelFormat.TRANSPARENT;
    layoutParameters.gravity = Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL;

    layoutParameters.width = LayoutParams.WRAP_CONTENT;
    layoutParameters.height = LayoutParams.WRAP_CONTENT;

    layoutParameters.flags |= LayoutParams.FLAG_NOT_TOUCHABLE;
    layoutParameters.flags |= LayoutParams.FLAG_NOT_FOCUSABLE;

    if (ApplicationUtilities.haveLollipopMR1) {
      layoutParameters.type = LayoutParams.TYPE_ACCESSIBILITY_OVERLAY;
    } else {
      layoutParameters.type = LayoutParams.TYPE_SYSTEM_OVERLAY;
    }
  }

  protected final boolean setView (View newView) {
    synchronized (this) {
      if (newView == currentView) return false;
      if (currentView != null) windowManager.removeView(currentView);
      currentView = newView;
      if (currentView != null) windowManager.addView(currentView, layoutParameters);
      return true;
    }
  }

  protected final boolean removeView () {
    return setView(null);
  }

  protected final boolean setView (int resource) {
    LayoutInflater inflater = LayoutInflater.from(BrailleService.getBrailleService());
    return setView(inflater.inflate(resource, null));
  }
}

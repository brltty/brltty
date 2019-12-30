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

import android.content.Context;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;

import android.view.View;
import android.view.LayoutInflater;

import android.view.Gravity;
import android.graphics.PixelFormat;

public abstract class AccessibilityOverlay {
  protected static Context getContext () {
    return BrailleService.getBrailleService();
  }

  protected static WindowManager getWindowManager () {
    return (WindowManager)getContext().getSystemService(Context.WINDOW_SERVICE);
  }

  private final LayoutParams layoutParameters = new LayoutParams();
  private View currentView = null;

  protected AccessibilityOverlay () {
    layoutParameters.format = PixelFormat.TRANSPARENT;
    layoutParameters.gravity = Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL;

    layoutParameters.width = LayoutParams.WRAP_CONTENT;
    layoutParameters.height = LayoutParams.WRAP_CONTENT;

    layoutParameters.flags |= LayoutParams.FLAG_NOT_TOUCHABLE;

    if (ApplicationUtilities.haveLollipopMR1) {
      layoutParameters.type = LayoutParams.TYPE_ACCESSIBILITY_OVERLAY;
    } else {
      layoutParameters.type = LayoutParams.TYPE_SYSTEM_OVERLAY;
    }
  }

  protected final void setView (View newView) {
    synchronized (this) {
      if (newView != currentView) {
        if (currentView != null) getWindowManager().removeView(currentView);
        currentView = newView;
        if (currentView != null) getWindowManager().addView(currentView, layoutParameters);
      }
    }
  }

  protected final void removeView () {
    setView(null);
  }

  protected final View setView (int resource) {
    LayoutInflater inflater = LayoutInflater.from(getContext());
    View view = inflater.inflate(resource, null);
    setView(view);

    view.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
    view.requestFocus();
    return view;
  }
}

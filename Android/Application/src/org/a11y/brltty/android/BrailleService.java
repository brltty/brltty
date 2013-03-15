/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

import org.a11y.brltty.core.*;

import android.util.Log;

import android.accessibilityservice.AccessibilityService;
import android.view.accessibility.AccessibilityEvent;

import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

import android.content.Intent;

public class BrailleService extends AccessibilityService {
  private static final String LOG_TAG = BrailleService.class.getName();

  private static volatile AccessibilityService accessibilityService = null;
  private Thread coreThread = null;

  public static AccessibilityService getAccessibilityService () {
    return accessibilityService;
  }

  @Override
  public void onCreate () {
    super.onCreate();
    Log.d(LOG_TAG, "braille accessibility service created");
    accessibilityService = this;
  }

  @Override
  protected void onServiceConnected () {
    Log.d(LOG_TAG, "braille accessibility service connected");

    coreThread = new CoreThread(this);
    coreThread.start();
  }

  @Override
  public void onAccessibilityEvent (AccessibilityEvent event) {
    Log.d(LOG_TAG, "accessibility event: " + event.getEventType() + "(" + event.toString() + ")");
    ScreenDriver.handleAccessibilityEvent(event);
  }

  @Override
  public void onInterrupt () {
  }

  @Override
  public boolean onUnbind (Intent intent) {
    Log.d(LOG_TAG, "braille accessibility service disconnected");
    CoreWrapper.stop();

    try {
      coreThread.join();
    } catch (InterruptedException exception) {
    }

    return false;
  }
}

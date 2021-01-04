/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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
import org.a11y.brltty.core.*;

import android.util.Log;

import android.accessibilityservice.AccessibilityService;
import android.view.accessibility.AccessibilityEvent;

import android.content.Intent;

public class BrailleService extends AccessibilityService {
  private final static String LOG_TAG = BrailleService.class.getName();

  private static volatile BrailleService brailleService = null;

  public static BrailleService getBrailleService () {
    return brailleService;
  }

  @Override
  public void onCreate () {
    super.onCreate();
    brailleService = this;
    Log.d(LOG_TAG, "braille service started");
  }

  @Override
  public void onDestroy () {
    try {
      brailleService = null;
      Log.d(LOG_TAG, "braille service stopped");
    } finally {
      super.onDestroy();
    }
  }

  private Thread coreThread = null;
  private AccessibilityButton accessibilityButton = null;

  @Override
  protected void onServiceConnected () {
    Log.d(LOG_TAG, "braille service connected");

    coreThread = new CoreThread(this);
    coreThread.start();

    if (APITests.haveOreo) {
      accessibilityButton = new AccessibilityButton();
      getAccessibilityButtonController().registerAccessibilityButtonCallback(accessibilityButton);
    }
  }

  @Override
  public boolean onUnbind (Intent intent) {
    Log.d(LOG_TAG, "braille service disconnected");

    if (accessibilityButton != null) {
      getAccessibilityButtonController().unregisterAccessibilityButtonCallback(accessibilityButton);
      accessibilityButton = null;
    }

    try {
      CoreWrapper.stop();
      Log.d(LOG_TAG, "waiting for core to stop");

      try {
        coreThread.join();
        Log.d(LOG_TAG, "core stopped");
      } catch (InterruptedException exception) {
        Log.w(LOG_TAG, "core thread join interrupted", exception);
      }
    } finally {
      coreThread = null;
    }

    return false;
  }

  @Override
  public void onAccessibilityEvent (AccessibilityEvent event) {
    ScreenDriver.onAccessibilityEvent(event);
  }

  @Override
  public void onInterrupt () {
  }
}

/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
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
import android.accessibilityservice.AccessibilityButtonController;
import android.view.accessibility.AccessibilityEvent;

import android.content.Intent;

public class BrailleService extends AccessibilityService {
  private final static String LOG_TAG = BrailleService.class.getName();

  private static volatile BrailleService brailleService = null;
  private AccessibilityButtonCallbacks accessibilityButtonCallbacks = null;
  private Thread coreThread = null;

  public static BrailleService getBrailleService () {
    return brailleService;
  }

  private final void enableAccessibilityButton () {
    if (APITests.haveOreo) {
      AccessibilityButtonController controller = getAccessibilityButtonController();

      if (controller != null) {
        synchronized (controller) {
          if (accessibilityButtonCallbacks == null) {
            accessibilityButtonCallbacks = new AccessibilityButtonCallbacks();
            controller.registerAccessibilityButtonCallback(accessibilityButtonCallbacks);
          }
        }
      }
    }
  }

  private final void disableAccessibilityButton () {
    if (APITests.haveOreo) {
      AccessibilityButtonController controller = getAccessibilityButtonController();

      if (controller != null) {
        synchronized (controller) {
          if (accessibilityButtonCallbacks != null) {
            controller.unregisterAccessibilityButtonCallback(accessibilityButtonCallbacks);
            accessibilityButtonCallbacks = null;
          }
        }
      }
    }
  }

  private final void startCoreThread () {
    Log.d(LOG_TAG, "starting core thread");
    coreThread = new CoreThread(this);
    coreThread.start();
  }

  private final void stopCoreThread () {
    try {
      Log.d(LOG_TAG, "stopping core thread");
      CoreWrapper.stop();
      Log.d(LOG_TAG, "waiting for core thread to stop");

      try {
        coreThread.join();
        Log.d(LOG_TAG, "core thread stopped");
      } catch (InterruptedException exception) {
        Log.w(LOG_TAG, "core thread join interrupted", exception);
      }
    } finally {
      coreThread = null;
    }
  }

  @Override
  public void onCreate () {
    super.onCreate();

    Log.d(LOG_TAG, "braille service starting");
    brailleService = this;

    enableAccessibilityButton();
    startCoreThread();
    BrailleNotification.create();
  }

  @Override
  public void onDestroy () {
    try {
      stopCoreThread();
      disableAccessibilityButton();
      BrailleNotification.destroy();

      brailleService = null;
      Log.d(LOG_TAG, "braille service stopped");
    } finally {
      super.onDestroy();
    }
  }

  @Override
  public void onAccessibilityEvent (AccessibilityEvent event) {
    ScreenDriver.onAccessibilityEvent(event);
  }

  @Override
  public void onInterrupt () {
  }
}

/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brltty.android;

import org.a11y.brltty.core.*;

import android.util.Log;

import android.accessibilityservice.AccessibilityService;
import android.view.accessibility.AccessibilityEvent;

import android.content.Intent;
import android.app.PendingIntent;
import android.app.Notification;

public class BrailleService extends AccessibilityService {
  private static final String LOG_TAG = BrailleService.class.getName();

  private static volatile BrailleService brailleService = null;
  private Thread coreThread = null;

  public static BrailleService getBrailleService () {
    return brailleService;
  }

  @Override
  public void onCreate () {
    super.onCreate();
    Log.d(LOG_TAG, "braille service started");
    brailleService = this;
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

  private final Intent makeSettingsIntent () {
    Intent intent = new Intent(this, SettingsActivity.class);
    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    return intent;
  }

  private final Notification makeForegroundNotification () {
    Intent settingsIntent = makeSettingsIntent();
    settingsIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
    PendingIntent contentIntent = PendingIntent.getActivity(this, 0, makeSettingsIntent(), 0);

    return new Notification.Builder(this)
              .setSmallIcon(R.drawable.ic_launcher)
              .setContentTitle("BRLTTY")
              .setContentText("Running")
              .setContentIntent(contentIntent)
              .setPriority(Notification.PRIORITY_LOW)
              .build();
  }

  @Override
  protected void onServiceConnected () {
    Log.d(LOG_TAG, "braille service connected");
    startForeground(1, makeForegroundNotification());

    coreThread = new CoreThread();
    coreThread.start();
  }

  @Override
  public boolean onUnbind (Intent intent) {
    Log.d(LOG_TAG, "braille service disconnected");
    CoreWrapper.stop();

    try {
      Log.d(LOG_TAG, "waiting for core to finish");
      coreThread.join();
      Log.d(LOG_TAG, "core finished");
    } catch (InterruptedException exception) {
      Log.d(LOG_TAG, "core join failed", exception);
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

  public boolean launchSettingsActivity () {
    Intent intent = makeSettingsIntent();

    intent.addFlags(
      Intent.FLAG_ACTIVITY_CLEAR_TOP |
      Intent.FLAG_ACTIVITY_SINGLE_TOP |
      Intent.FLAG_FROM_BACKGROUND
    );

    startActivity(intent);
    return true;
  }
}

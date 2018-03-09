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

import android.os.Build;
import android.util.Log;

import android.accessibilityservice.AccessibilityService;
import android.view.accessibility.AccessibilityEvent;

import android.content.Intent;
import android.app.PendingIntent;
import android.app.Notification;
import android.app.NotificationManager;

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
    return new Intent(this, SettingsActivity.class)
      .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
      ;
  }

  private static final Integer notificationIdentifier = 1;
  private static Notification.Builder notificationBuilder = null;
  private static NotificationManager notificationManager = null;

  public final void showState () {
    synchronized (notificationIdentifier) {
      if (notificationBuilder == null) {
        notificationBuilder = new Notification.Builder(this)
          .setSmallIcon(R.drawable.ic_launcher)
          .setContentTitle("BRLTTY")
          .setSubText("Tap for Settings.")

          .setContentIntent(
            PendingIntent.getActivity(
              this,
              0,
              makeSettingsIntent().addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK),
              0
            )
          )

          .setPriority(Notification.PRIORITY_LOW)
          .setOngoing(true)
          .setOnlyAlertOnce(true)
          ;

        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR1)) {
          notificationBuilder.setShowWhen(false);
        }

        if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.LOLLIPOP)) {
          notificationBuilder.setCategory(Notification.CATEGORY_SERVICE);
        }
      }

      {
        int state;

        if (ApplicationSettings.RELEASE_BRAILLE_DEVICE) {
          state = R.string.braille_state_released;
        } else if (ApplicationSettings.BRAILLE_DEVICE_ONLINE) {
          state = R.string.braille_state_connected;
        } else {
          state = R.string.braille_state_waiting;
        }

        notificationBuilder.setContentText(getString(state));
      }

      {
        Notification notification = notificationBuilder.build();

        if (notificationManager == null) {
          startForeground(notificationIdentifier, notification);
          notificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
        } else {
          notificationManager.notify(notificationIdentifier, notification);
        }
      }
    }
  }

  @Override
  protected void onServiceConnected () {
    Log.d(LOG_TAG, "braille service connected");

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

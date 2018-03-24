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

import android.os.Build;

import android.content.Context;
import android.content.Intent;
import android.app.PendingIntent;

import android.app.Notification;
import android.app.NotificationManager;

public abstract class BrailleNotification {
  private BrailleNotification () {
  }

  private static Context applicationContext = null;
  private static final Integer notificationIdentifier = 1;
  private static Notification.Builder notificationBuilder = null;
  private static NotificationManager notificationManager = null;

  private static Context getContext () {
    if (applicationContext == null) {
      applicationContext = ApplicationContext.get();
    }

    return applicationContext;
  }

  private static String getString (int identifier) {
    return getContext().getString(identifier);
  }

  private static Notification.Builder getNotificationBuilder () {
    if (notificationBuilder == null) {
      Context context = getContext();

      notificationBuilder = new Notification.Builder(context)
        .setPriority(Notification.PRIORITY_LOW)
        .setOngoing(true)
        .setOnlyAlertOnce(true)

        .setSmallIcon(R.drawable.braille_notification)
        .setSubText(getString(R.string.braille_hint_tap))

        .setContentIntent(
          PendingIntent.getActivity(
            context,
            0, // request code
            SettingsActivity.makeIntent().addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK),
            0 // flags
          )
        );

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN_MR1)) {
        notificationBuilder.setShowWhen(false);
      }

      if (ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.LOLLIPOP)) {
        notificationBuilder.setCategory(Notification.CATEGORY_SERVICE);
      }
    }

    return notificationBuilder;
  }

  private static NotificationManager getNotificationManager () {
    if (notificationManager == null) {
      notificationManager = (NotificationManager)
                            getContext().getSystemService(Context.NOTIFICATION_SERVICE);
    }

    return notificationManager;
  }

  private static void updateNotification (Notification.Builder nb) {
    getNotificationManager().notify(notificationIdentifier, nb.build());
  }

  private static void setState (Notification.Builder nb) {
    int state;

    if (ApplicationSettings.RELEASE_BRAILLE_DEVICE) {
      state = R.string.braille_state_released;
    } else if (ApplicationSettings.BRAILLE_DEVICE_ONLINE) {
      state = R.string.braille_state_connected;
    } else {
      state = R.string.braille_state_waiting;
    }

    nb.setContentTitle(getString(state));
  }

  public static void setState () {
    synchronized (notificationIdentifier) {
      Notification.Builder nb = getNotificationBuilder();
      setState(nb);
      updateNotification(nb);
    }
  }

  private static void setDevice (Notification.Builder nb, String device) {
    if (device == null) device = "";
    if (device.isEmpty()) device = getString(R.string.SELECTED_DEVICE_UNSELECTED);
    nb.setContentText(device);
  }

  public static void setDevice (String device) {
    synchronized (notificationIdentifier) {
      Notification.Builder nb = getNotificationBuilder();
      setDevice(nb, device);
      updateNotification(nb);
    }
  }

  public static void unsetDevice () {
    setDevice(null);
  }

  public static void create () {
    synchronized (notificationIdentifier) {
      Notification.Builder nb = getNotificationBuilder();
      setState(nb);
      setDevice(nb, DeviceManager.getSelectedDevice());

      BrailleService.getBrailleService()
                    .startForeground(notificationIdentifier, nb.build());
    }
  }

  public static void destroy () {
    synchronized (notificationIdentifier) {
      getNotificationManager().cancel(notificationIdentifier);
    }
  }
}

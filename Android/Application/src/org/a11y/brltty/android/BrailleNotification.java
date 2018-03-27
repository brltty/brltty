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

import android.content.Context;
import android.content.Intent;
import android.app.PendingIntent;

import android.app.Notification;
import android.app.NotificationManager;

public abstract class BrailleNotification {
  private BrailleNotification () {
  }

  private static Context applicationContext = null;
  private final static Integer notificationIdentifier = 1;
  private static NotificationManager notificationManager = null;
  private static Notification.Builder notificationBuilder = null;

  private static Context getContext () {
    if (applicationContext == null) {
      applicationContext = ApplicationContext.get();
    }

    return applicationContext;
  }

  private static String getString (int identifier) {
    return getContext().getString(identifier);
  }

  private static NotificationManager getManager () {
    if (notificationManager == null) {
      notificationManager = (NotificationManager)
                            getContext().getSystemService(Context.NOTIFICATION_SERVICE);
    }

    return notificationManager;
  }

  private static boolean isActive () {
    return notificationBuilder != null;
  }

  private static void makeBuilder () {
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

    if (ApplicationUtilities.haveJellyBeanMR1) {
      notificationBuilder.setShowWhen(false);
    }

    if (ApplicationUtilities.haveLollipop) {
      notificationBuilder.setCategory(Notification.CATEGORY_SERVICE);
    }
  }

  private static Notification buildNotification () {
    return notificationBuilder.build();
  }

  private static void updateNotification () {
    getManager().notify(notificationIdentifier, buildNotification());
  }

  private static void setState () {
    int state;

    if (ApplicationSettings.RELEASE_BRAILLE_DEVICE) {
      state = R.string.braille_state_released;
    } else if (ApplicationSettings.BRAILLE_DEVICE_ONLINE) {
      state = R.string.braille_state_connected;
    } else {
      state = R.string.braille_state_waiting;
    }

    notificationBuilder.setContentTitle(getString(state));
  }

  public static void updateState () {
    synchronized (notificationIdentifier) {
      if (isActive()) {
        setState();
        updateNotification();
      }
    }
  }

  private static void setDevice (String device) {
    if (device == null) device = "";
    if (device.isEmpty()) device = getString(R.string.SELECTED_DEVICE_UNSELECTED);
    notificationBuilder.setContentText(device);
  }

  public static void updateDevice (String device) {
    synchronized (notificationIdentifier) {
      if (isActive()) {
        setDevice(device);
        updateNotification();
      }
    }
  }

  public static void create () {
    synchronized (notificationIdentifier) {
      if (isActive()) {
        throw new IllegalStateException("already active");
      }

      makeBuilder();
      setState();
      setDevice(DeviceManager.getSelectedDevice());

      BrailleService.getBrailleService()
                    .startForeground(notificationIdentifier, buildNotification());
    }
  }

  public static void destroy () {
    synchronized (notificationIdentifier) {
      if (!isActive()) {
        throw new IllegalStateException("not active");
      }

      notificationBuilder = null;
      getManager().cancel(notificationIdentifier);
    }
  }
}

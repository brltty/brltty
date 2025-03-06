/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
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
import org.a11y.brltty.android.activities.ActionsActivity;
import org.a11y.brltty.android.settings.DeviceManager;

import android.util.Log;

import android.content.Context;
import android.content.Intent;
import android.app.PendingIntent;
import android.app.Activity;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.NotificationChannel;

public abstract class BrailleNotification {
  private final static String LOG_TAG = BrailleNotification.class.getName();

  private BrailleNotification () {
  }

  private final static Integer NOTIFICATION_IDENTIFIER = 1;
  private final static String NOTIFICATION_CHANNEL = "braille";

  private static boolean isCreated = false;
  private static Context applicationContext = null;
  private static NotificationManager notificationManager = null;
  private static Notification.Builder notificationBuilder = null;

  private static Context getContext () {
    if (applicationContext == null) {
      applicationContext = BrailleApplication.get();
    }

    return applicationContext;
  }

  private static String getString (int identifier) {
    return getContext().getString(identifier);
  }

  private static PendingIntent newPendingIntent (Class<? extends Activity> activityClass) {
    Context context = getContext();
    Intent intent = new Intent(context, activityClass);

    intent.addFlags(
      Intent.FLAG_ACTIVITY_CLEAR_TASK |
      Intent.FLAG_ACTIVITY_NEW_TASK
    );

    return PendingIntent.getActivity(
      context, 0, intent,
      PendingIntent.FLAG_IMMUTABLE
    );
  }

  private static NotificationManager getManager () {
    if (notificationManager == null) {
      notificationManager = (NotificationManager)
                            getContext().getSystemService(Context.NOTIFICATION_SERVICE);
    }

    return notificationManager;
  }

  private static void makeBuilder () {
    if (notificationBuilder == null) {
      Context context = getContext();
      Notification.Builder builder;

      if (APITests.haveOreo) {
        NotificationManager nm = getManager();
        NotificationChannel channel = nm.getNotificationChannel(NOTIFICATION_CHANNEL);

        if (channel == null) {
          channel = new NotificationChannel(
            NOTIFICATION_CHANNEL,
            getString(R.string.braille_channel_name),
            NotificationManager.IMPORTANCE_LOW
          );

          nm.createNotificationChannel(channel);
        }

        builder = new Notification.Builder(context, NOTIFICATION_CHANNEL);
      } else {
        builder = new Notification.Builder(context)
                                  .setPriority(Notification.PRIORITY_LOW)
                                  ;
      }

      builder.setOngoing(true)
             .setOnlyAlertOnce(true)
             .setSmallIcon(R.drawable.braille_notification)
             .setSubText(getString(R.string.braille_hint_tap))
             .setContentIntent(newPendingIntent(ActionsActivity.class))
             ;

      if (APITests.haveJellyBeanMR1) {
        builder.setShowWhen(false);
      }

      if (APITests.haveLollipop) {
        builder.setCategory(Notification.CATEGORY_SERVICE);
      }

      notificationBuilder = builder;
    }
  }

  private static Notification buildNotification () {
    return notificationBuilder.build();
  }

  private static void updateNotification () {
    getManager().notify(NOTIFICATION_IDENTIFIER, buildNotification());
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
    synchronized (NOTIFICATION_IDENTIFIER) {
      if (isCreated) {
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
    synchronized (NOTIFICATION_IDENTIFIER) {
      if (isCreated) {
        setDevice(device);
        updateNotification();
      }
    }
  }

  public static void create () {
    synchronized (NOTIFICATION_IDENTIFIER) {
      if (!isCreated) {
        makeBuilder();

        setState();
        setDevice(DeviceManager.getSelectedDevice());

        try {
          BrailleService.getBrailleService()
                        .startForeground(NOTIFICATION_IDENTIFIER, buildNotification());

          isCreated = true;
        } catch (SecurityException exception) {
          Log.w(LOG_TAG, ("can't become foreground service: " + exception.getMessage()));
        }
      }
    }
  }

  public static void destroy () {
    synchronized (NOTIFICATION_IDENTIFIER) {
      if (isCreated) {
        getManager().cancel(NOTIFICATION_IDENTIFIER);
        isCreated = false;
      }
    }
  }
}

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

import android.util.Log;
import android.os.SystemClock;

import android.content.Context;
import android.content.ContentResolver;
import android.provider.Settings;

import android.os.PowerManager;
import android.app.KeyguardManager;
import android.view.inputmethod.InputMethodManager;
import android.view.WindowManager;
import android.hardware.usb.UsbManager;

import android.content.Intent;
import android.app.Activity;

public abstract class ApplicationUtilities {
  private final static String LOG_TAG = ApplicationUtilities.class.getName();

  private ApplicationUtilities () {
  }

  public static long getAbsoluteTime () {
    return SystemClock.uptimeMillis();
  }

  public static long getRelativeTime (long from) {
    return getAbsoluteTime() - from;
  }

  public static String getResourceString (int identifier) {
    return BrailleApplication.get().getString(identifier);
  }

  public static ContentResolver getContentResolver () {
    return BrailleApplication.get().getContentResolver();
  }

  public static String getSecureSetting (String setting) {
    return Settings.Secure.getString(getContentResolver(), setting);
  }

  public static String getSelectedInputMethodIdentifier () {
    return getSecureSetting(Settings.Secure.DEFAULT_INPUT_METHOD);
  }

  private final static SingletonReference<PowerManager> powerManagerReference
    = new SystemServiceReference<PowerManager>(Context.POWER_SERVICE);
  public static PowerManager getPowerManager () {
    return powerManagerReference.get();
  }

  private final static SingletonReference<KeyguardManager> keyguardManagerReference
    = new SystemServiceReference<KeyguardManager>(Context.KEYGUARD_SERVICE);
  public static KeyguardManager getKeyguardManager () {
    return keyguardManagerReference.get();
  }

  private final static SingletonReference<InputMethodManager> inputMethodManagerReference
    = new SystemServiceReference<InputMethodManager>(Context.INPUT_METHOD_SERVICE);
  public static InputMethodManager getInputMethodManager () {
    return inputMethodManagerReference.get();
  }

  private final static SingletonReference<WindowManager> windowManagerReference
    = new SystemServiceReference<WindowManager>(Context.WINDOW_SERVICE);
  public static WindowManager getWindowManager () {
    return windowManagerReference.get();
  }

  private final static SingletonReference<UsbManager> usbManagerReference
    = new SystemServiceReference<UsbManager>(Context.USB_SERVICE);
  public static UsbManager getUsbManager () {
    return usbManagerReference.get();
  }

  public static void launch (Class<? extends Activity> activityClass) {
    Intent intent = new Intent(BrailleApplication.get(), activityClass);

    intent.addFlags(
      Intent.FLAG_ACTIVITY_NEW_TASK |
      Intent.FLAG_ACTIVITY_CLEAR_TOP |
      Intent.FLAG_FROM_BACKGROUND
    );

    BrailleApplication.get().startActivity(intent);
  }
}

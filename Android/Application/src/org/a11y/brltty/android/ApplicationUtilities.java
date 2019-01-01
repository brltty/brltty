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

import android.util.Log;
import android.os.Build;
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

  private static boolean haveAPILevel (int level) {
    return Build.VERSION.SDK_INT >= level;
  }

  public static final boolean haveIceCreamSandwich
  = haveAPILevel(Build.VERSION_CODES.ICE_CREAM_SANDWICH);

  public static final boolean haveJellyBean
  = haveAPILevel(Build.VERSION_CODES.JELLY_BEAN);

  public static final boolean haveJellyBeanMR1
  = haveAPILevel(Build.VERSION_CODES.JELLY_BEAN_MR1);

  public static final boolean haveJellyBeanMR2
  = haveAPILevel(Build.VERSION_CODES.JELLY_BEAN_MR2);

  public static final boolean haveKitkat
  = haveAPILevel(Build.VERSION_CODES.KITKAT);

  public static final boolean haveLollipop
  = haveAPILevel(Build.VERSION_CODES.LOLLIPOP);

  public static final boolean haveLollipopMR1
  = haveAPILevel(Build.VERSION_CODES.LOLLIPOP_MR1);

  public static final boolean haveMarshmallow
  = haveAPILevel(Build.VERSION_CODES.M);

  public static final boolean haveNougat
  = haveAPILevel(Build.VERSION_CODES.N);

  public static final boolean haveNougatMR1
  = haveAPILevel(Build.VERSION_CODES.N_MR1);

  public static final boolean haveOreo
  = haveAPILevel(Build.VERSION_CODES.O);

  public static long getAbsoluteTime () {
    return SystemClock.uptimeMillis();
  }

  public static long getRelativeTime (long from) {
    return getAbsoluteTime() - from;
  }

  public static String getResourceString (int identifier) {
    return ApplicationContext.get().getString(identifier);
  }

  public static ContentResolver getContentResolver () {
    return ApplicationContext.get().getContentResolver();
  }

  public static String getSecureSetting (String setting) {
    return Settings.Secure.getString(getContentResolver(), setting);
  }

  public static String getSelectedInputMethodIdentifier () {
    return getSecureSetting(Settings.Secure.DEFAULT_INPUT_METHOD);
  }

  private static final SingletonReference<PowerManager> powerManagerReference
    = new SystemServiceReference<PowerManager>(Context.POWER_SERVICE);
  public static PowerManager getPowerManager () {
    return powerManagerReference.get();
  }

  private static final SingletonReference<KeyguardManager> keyguardManagerReference
    = new SystemServiceReference<KeyguardManager>(Context.KEYGUARD_SERVICE);
  public static KeyguardManager getKeyguardManager () {
    return keyguardManagerReference.get();
  }

  private static final SingletonReference<InputMethodManager> inputMethodManagerReference
    = new SystemServiceReference<InputMethodManager>(Context.INPUT_METHOD_SERVICE);
  public static InputMethodManager getInputMethodManager () {
    return inputMethodManagerReference.get();
  }

  private static final SingletonReference<WindowManager> windowManagerReference
    = new SystemServiceReference<WindowManager>(Context.WINDOW_SERVICE);
  public static WindowManager getWindowManager () {
    return windowManagerReference.get();
  }

  private static final SingletonReference<UsbManager> usbManagerReference
    = new SystemServiceReference<UsbManager>(Context.USB_SERVICE);
  public static UsbManager getUsbManager () {
    return usbManagerReference.get();
  }

  public static void launch (Class<? extends Activity> activityClass) {
    Intent intent = new Intent(ApplicationContext.get(), activityClass);

    intent.addFlags(
      Intent.FLAG_ACTIVITY_NEW_TASK |
      Intent.FLAG_ACTIVITY_CLEAR_TOP |
      Intent.FLAG_FROM_BACKGROUND
    );

    ApplicationContext.get().startActivity(intent);
  }
}

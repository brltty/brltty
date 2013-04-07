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

import android.content.Context;
import android.os.PowerManager;
import android.app.KeyguardManager;
import android.view.inputmethod.InputMethodManager;
import android.hardware.usb.UsbManager;

public abstract class ApplicationUtilities {
  public static Context getContext () {
    return BrailleService.getBrailleService();
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

  private static final SingletonReference<UsbManager> usbManagerReference
    = new SystemServiceReference<UsbManager>(Context.USB_SERVICE);
  public static UsbManager getUsbManager () {
    return usbManagerReference.get();
  }

  private ApplicationUtilities () {
  }
}

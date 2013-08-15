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

import android.os.Build;

public abstract class ApplicationParameters {
  public static volatile int BRAILLE_COLUMN_SPACING = 2;
  public static volatile int BRAILLE_ROW_SPACING = 0;

  public static volatile boolean LOG_ACCESSIBILITY_EVENTS = false;
  public static volatile boolean LOG_KEYBOARD_EVENTS = false;

  public static volatile int KEY_RETRY_TIMEOUT = 1000;
  public static volatile int KEY_RETRY_INTERVAL = 100;
  public static volatile int LONG_PRESS_DELAY = 100;

  public static volatile int SDK_VERSION = Build.VERSION.SDK_INT;
//public static volatile int SDK_VERSION = Build.VERSION_CODES.ICE_CREAM_SANDWICH;

  public static volatile String B2G_DEVICE_PATH = "/dev/braille0";
  public static volatile String B2G_DRIVER_CODE = "bg";

  private ApplicationParameters () {
  }
}

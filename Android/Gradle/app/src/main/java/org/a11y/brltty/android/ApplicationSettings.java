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

import android.os.Build;

public abstract class ApplicationSettings {
  private ApplicationSettings () {
  }

  public static volatile boolean RELEASE_BRAILLE_DEVICE = false;
  public static volatile boolean BRAILLE_DEVICE_ONLINE = false;

  public static volatile boolean SHOW_NOTIFICATIONS = false;
  public static volatile boolean SHOW_ALERTS = false;
  public static volatile boolean SHOW_ANNOUNCEMENTS = false;

  public static volatile boolean LOG_ACCESSIBILITY_EVENTS = false;
  public static volatile boolean LOG_RENDERED_SCREEN = false;
  public static volatile boolean LOG_KEYBOARD_EVENTS = false;
  public static volatile boolean LOG_UNHANDLED_EVENTS = false;
}

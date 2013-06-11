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
  public static final int SDK_VERSION = Build.VERSION.SDK_INT;
//public static final int SDK_VERSION = Build.VERSION_CODES.ICE_CREAM_SANDWICH;

  public static final boolean LOG_ACCESSIBILITY_EVENTS = false;

  public static final BrailleRenderer BRAILLE_RENDERER = new SimpleBrailleRenderer();
//public static final BrailleRenderer BRAILLE_RENDERER = new GridBrailleRenderer();
  public static final int COLUMN_SPACING = 2;

  public static final int keyRetryTimeout = 1000;
  public static final int keyRetryInterval = 100;

  private ApplicationParameters () {
  }
}

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

public abstract class ApplicationParameters {
  private ApplicationParameters () {
  }

  public final static boolean ENABLE_SET_TEXT = false;
  public final static boolean ENABLE_MOTION_LOGGING = true;

  public final static int BRAILLE_COLUMN_SPACING = 2;
  public final static int BRAILLE_ROW_SPACING = 0;

  public final static int CORE_WAIT_DURATION = Integer.MAX_VALUE;

  public final static long FOCUS_WAIT_TIMEOUT = 1000;
  public final static long FOCUS_WAIT_INTERVAL = 100;

  public final static long LONG_PRESS_DELAY = 100;
  public final static long FOCUS_SETTER_DELAY = 500;
}

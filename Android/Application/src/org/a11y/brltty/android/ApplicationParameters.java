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

public abstract class ApplicationParameters {
  private ApplicationParameters () {
  }

  public static final int BRAILLE_COLUMN_SPACING = 2;
  public static final int BRAILLE_ROW_SPACING = 0;

  public static final int CORE_WAIT_DURATION = Integer.MAX_VALUE;

  public static final int FOCUS_WAIT_TIMEOUT = 1000;
  public static final int FOCUS_WAIT_INTERVAL = 100;

  public static final int LONG_PRESS_DELAY = 100;
}

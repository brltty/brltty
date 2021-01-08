/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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

import android.os.PowerManager;

public final class LockUtilities {
  private final static PowerManager.WakeLock wakeLock =
    ApplicationUtilities.getPowerManager().newWakeLock(
      PowerManager.SCREEN_DIM_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE,
      BrailleApplication.get().getString(R.string.app_name)
    );

  public static boolean isLocked () {
    if (!ApplicationUtilities.getPowerManager().isScreenOn()) {
      if (ApplicationUtilities.getKeyguardManager().inKeyguardRestrictedInputMode()) {
        return true;
      }
    }

    return false;
  }

  public static void resetTimer () {
    wakeLock.acquire();
    wakeLock.release();
  }

  private LockUtilities () {
  }
}

/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

import android.os.PowerManager;

public final class LockUtilities {
  private static final PowerManager.WakeLock wakeLock =
    ApplicationUtilities.getPowerManager().newWakeLock(
      PowerManager.SCREEN_DIM_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE,
      ApplicationHooks.getContext().getString(R.string.app_name)
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

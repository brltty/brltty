/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

import android.util.Log;

import android.media.AudioManager;
import android.media.AudioFormat;
import android.media.AudioTrack;

public final class PcmDevice {
  private static final String LOG_TAG = PcmDevice.class.getName();

  public PcmDevice () {
    Log.d(LOG_TAG, "pcm device constructor");
  }

  public void close () {
    Log.d(LOG_TAG, "pcm device close");
  }
}

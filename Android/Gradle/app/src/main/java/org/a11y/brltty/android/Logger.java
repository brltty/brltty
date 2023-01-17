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

import android.util.Log;
import java.io.IOException;

public abstract class Logger {
  private final static String LOG_TAG = Logger.class.getName();

  protected String getLogTag () {
    return LOG_TAG;
  }

  protected void log (String message) throws IOException {
    Log.d(LOG_TAG, message);
  }

  protected Logger () {
  }

  public static String shrinkText (String text) {
    if (text == null) return null;

    final int threshold = 50;
    final char delimiter = '\n';

    int length = text.length();
    int to = text.lastIndexOf(delimiter);
    int from = (to == -1)? length: text.indexOf(delimiter);
    to += 1;

    from = Math.min(from, threshold);
    to = Math.max(to, (length - threshold));

    if (from < to) {
      text = text.substring(0, from) + "[...]" + text.substring(to);
    }

    return text;
  }
}

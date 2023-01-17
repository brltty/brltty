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

import java.util.Locale;

import android.app.Application;
import android.os.Handler;

public class BrailleApplication extends Application {
  private static BrailleApplication applicationObject = null;
  private static Handler applicationHandler = null;

  @Override
  public void onCreate () {
    super.onCreate();
    applicationObject = this;
    applicationHandler = new Handler();
  }

  public static BrailleApplication get () {
    return applicationObject;
  }

  public static void unpost (Runnable callback) {
    applicationHandler.removeCallbacks(callback);
  }

  public static boolean post (Runnable callback) {
    return applicationHandler.post(callback);
  }

  public static boolean postIn (long delay, Runnable callback) {
    return applicationHandler.postDelayed(callback, delay);
  }

  public static boolean postAt (long when, Runnable callback) {
    return applicationHandler.postAtTime(callback, when);
  }

  public static String getCurrentLocale () {
    Locale locale;

    if (APITests.haveNougat) {
      locale = get().getResources().getConfiguration().getLocales().get(0);
    } else {
      locale = get().getResources().getConfiguration().locale;
    }

    if (locale == null) return null;
    return locale.toString();
  }
}

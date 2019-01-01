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

import android.util.Log;

import android.app.Activity;
import android.content.Intent;
import android.content.ActivityNotFoundException;
import android.net.Uri;

public abstract class InternalActivity extends Activity {
  private final static String LOG_TAG = InternalActivity.class.getName();

  protected final String getResourceString (int identifier) {
    return getResources().getString(identifier);
  }

  protected final void launch (Intent intent) {
    try {
      startActivity(intent);
    } catch (ActivityNotFoundException exception) {
      Log.w(LOG_TAG, "activity not found", exception);
    }
  }

  protected final void launch (Uri uri) {
    launch(new Intent(Intent.ACTION_VIEW, uri));
  }

  protected final void launch (String url) {
    launch(Uri.parse(url));
  }

  protected final void launch (int url) {
    launch(getResourceString(url));
  }
}

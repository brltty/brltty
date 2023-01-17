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

package org.a11y.brltty.android.activities;
import org.a11y.brltty.android.*;

import android.util.Log;

import android.os.Bundle;

import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;

import android.view.View;
import android.widget.TextView;

public class AboutActivity extends InternalActivity {
  private final static String LOG_TAG = AboutActivity.class.getName();

  public void viewPrivacyPolicy (View view) {
    launch(R.string.privacy_policy_url);
  }

  public void viewGooglePlay (View view) {
    launch(R.string.google_play_url);
  }

  private final void setText (int view, CharSequence text) {
    ((TextView)findViewById(view)).setText(text);
  }

  @Override
  protected void onCreate (Bundle savedState) {
    super.onCreate(savedState);
    setContentView(R.layout.about_activity);

    String name = getPackageName();
    try {
      PackageInfo info = getPackageManager().getPackageInfo(name, 0);

      setText(R.id.app_version_name, info.versionName);
    } catch (PackageManager.NameNotFoundException exception) {
      Log.w(LOG_TAG, ("package information not found: " + name));
    }
  }

  public static void launch () {
    ApplicationUtilities.launch(AboutActivity.class);
  }
}

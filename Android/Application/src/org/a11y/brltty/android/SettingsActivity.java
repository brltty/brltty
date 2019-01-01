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

import java.util.List;

import android.os.Bundle;
import android.content.Context;
import android.preference.PreferenceActivity;

public class SettingsActivity extends PreferenceActivity {
  private static final String LOG_TAG = SettingsActivity.class.getName();

  @Override
  protected void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    ApplicationContext.set(this);
    setTitle(R.string.SETTINGS_SCREEN_MAIN);
  }

  @Override
  public void onBuildHeaders (List<Header> headers) {
    loadHeadersFromResource(R.xml.settings_headers, headers);
  }

  public static void launch () {
    ApplicationUtilities.launch(SettingsActivity.class);
  }
}

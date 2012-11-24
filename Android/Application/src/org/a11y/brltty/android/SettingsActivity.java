/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

import java.util.List;

import android.util.Log;

import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.os.Bundle;
import android.app.FragmentManager;
import android.app.FragmentTransaction;

import android.content.Intent;
import android.view.MenuItem;

public class SettingsActivity extends PreferenceActivity {
  private final String LOG_TAG = this.getClass().getName();

  @Override
  protected void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    getActionBar().setDisplayHomeAsUpEnabled(true);
  }

  @Override
  public void onBuildHeaders (List<Header> target) {
    loadHeadersFromResource(R.xml.settings_headers, target);
  }

  @Override
  public boolean onOptionsItemSelected (MenuItem item) {
    switch (item.getItemId()) {
      case android.R.id.home: {
        FragmentManager fm = getFragmentManager();

        if (fm.getBackStackEntryCount() > 0) {
          fm.popBackStack();
        } else {
          Intent intent = new Intent(this, MainActivity.class);
          intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
          startActivity(intent);
        }

        return true;
      }

      default: {
        return super.onOptionsItemSelected(item);
      }
    }
  }

  public static abstract class SettingsFragment extends PreferenceFragment {
    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);
    }
  }

  public static final class GeneralSettings extends SettingsFragment {
    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      addPreferencesFromResource(R.xml.settings_general);
    }
  }

  public static final class DeviceSettings extends SettingsFragment {
    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      addPreferencesFromResource(R.xml.settings_device);
    }
  }

  public static final class AdvancedSettings extends SettingsFragment {
    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      addPreferencesFromResource(R.xml.settings_advanced);
    }
  }
}

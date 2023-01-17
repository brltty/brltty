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

package org.a11y.brltty.android.settings;
import org.a11y.brltty.android.*;

import java.util.Map;
import java.util.LinkedHashMap;

import android.util.Log;
import android.widget.Toast;
import android.os.Bundle;

import android.content.SharedPreferences;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.preference.Preference;

public abstract class SettingsFragment extends PreferenceFragment {
  private final static String LOG_TAG = SettingsFragment.class.getName();

  @Override
  public void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    if (APITests.haveNougat) {
      getPreferenceManager().setStorageDeviceProtected();
    }
  }

  protected final void showProblem (int problem, String detail) {
    StringBuilder message = new StringBuilder();
    message.append(getString(problem));
    if (detail != null) message.append(": ").append(detail);
    Toast.makeText(getActivity(), message.toString(), Toast.LENGTH_LONG).show();
  }

  protected final void showProblem (int problem) {
    showProblem(problem, null);
  }

  private static String makePropertyKey (String name, String key) {
    return key + '-' + name;
  }

  protected static void putProperties (SharedPreferences.Editor editor, String name, Map<String, String> properties) {
    for (Map.Entry<String, String> property : properties.entrySet()) {
      editor.putString(makePropertyKey(name, property.getKey()), property.getValue());
    }
  }

  protected static Map<String, String> getProperties (SharedPreferences prefs, String name, String... keys) {
    Map<String, String> properties = new LinkedHashMap();

    for (String key : keys) {
      properties.put(key, prefs.getString(makePropertyKey(name, key), ""));
    }

    return properties;
  }

  protected static void removeProperties (SharedPreferences.Editor editor, String name, String... keys) {
    for (String key : keys) {
      editor.remove(makePropertyKey(name, key));
    }
  }

  protected final SharedPreferences getPreferences () {
    return getPreferenceManager().getSharedPreferences();
  }

  protected final Preference getPreference (int key) {
    return findPreference(getResources().getString(key));
  }

  protected final PreferenceScreen getPreferenceScreen (int key) {
    return (PreferenceScreen)getPreference(key);
  }
}

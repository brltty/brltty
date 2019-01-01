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
import org.a11y.brltty.core.*;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.CheckBoxPreference;

public final class MessageSettings extends SettingsFragment {
  private static final String LOG_TAG = MessageSettings.class.getName();

  private CheckBoxPreference showNotificationsCheckBox;
  private CheckBoxPreference showToastsCheckBox;
  private CheckBoxPreference showAnnouncementsCheckBox;

  @Override
  public void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.settings_message);

    showNotificationsCheckBox = getCheckBoxPreference(R.string.PREF_KEY_SHOW_NOTIFICATIONS);
    showToastsCheckBox = getCheckBoxPreference(R.string.PREF_KEY_SHOW_TOASTS);
    showAnnouncementsCheckBox = getCheckBoxPreference(R.string.PREF_KEY_SHOW_ANNOUNCEMENTS);

    showSelection(showNotificationsCheckBox);
    showSelection(showToastsCheckBox);
    showSelection(showAnnouncementsCheckBox);

    showNotificationsCheckBox.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final boolean newSetting = (Boolean)newValue;
          ApplicationSettings.SHOW_NOTIFICATIONS = newSetting;

          showSelection(showNotificationsCheckBox, newSetting);
          return true;
        }
      }
    );

    showToastsCheckBox.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final boolean newSetting = (Boolean)newValue;
          ApplicationSettings.SHOW_TOASTS = newSetting;

          showSelection(showToastsCheckBox, newSetting);
          return true;
        }
      }
    );

    showAnnouncementsCheckBox.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final boolean newSetting = (Boolean)newValue;
          ApplicationSettings.SHOW_ANNOUNCEMENTS = newSetting;

          showSelection(showAnnouncementsCheckBox, newSetting);
          return true;
        }
      }
    );
  }
}

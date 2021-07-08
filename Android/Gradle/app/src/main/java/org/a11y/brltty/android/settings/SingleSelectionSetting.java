/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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

import android.preference.Preference;
import android.preference.ListPreference;

import java.util.Arrays;

public abstract class SingleSelectionSetting extends PreferenceSetting<ListPreference> {
  protected abstract void onSelectionChanged (String newSelection);

  @Override
  public final void showSelection () {
    CharSequence label = settingPreference.getEntry();
    if (label == null) label = "";
    showSelection(label);
  }

  protected final void showSelection (int index) {
    showSelection(settingPreference.getEntries()[index]);
  }

  private final int toIndex (String value) {
    return Arrays.asList(settingPreference.getEntryValues()).indexOf(value);
  }

  protected SingleSelectionSetting (SettingsFragment fragment, int key) {
    super(fragment, key);

    settingPreference.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newSelection = (String)newValue;

          showSelection(toIndex(newSelection));
          onSelectionChanged(newSelection);

          return true;
        }
      }
    );
  }

  protected final void resetSelection () {
    settingPreference.setValueIndex(0);
    showSelection();
  }

  protected final void setElements (String[] values, String[] labels) {
    settingPreference.setEntryValues(values);
    settingPreference.setEntries(labels);
  }

  protected final void setElements (String[] values) {
    setElements(values, values);
  }

  protected final void sortElements (int fromIndex) {
    settingsFragment.sortList(settingPreference, fromIndex);
  }

  protected final void sortElements () {
    sortElements(0);
  }
}

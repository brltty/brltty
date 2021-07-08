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
import android.preference.MultiSelectListPreference;

import java.util.Set;

public abstract class MultipleSelectionSetting extends PreferenceSetting<MultiSelectListPreference> {
  protected abstract void onSelectionChanged (Set<String> newSelection);

  protected final void showSelection (Set<String> values) {
    StringBuilder text = new StringBuilder();

    {
      CharSequence[] labels = settingPreference.getEntries();

      for (String value : values) {
        if (value.length() > 0) {
          if (text.length() > 0) text.append('\n');
          text.append(labels[settingPreference.findIndexOfValue(value)]);
        }
      }
    }

    if (text.length() == 0) {
      text.append(getString(R.string.SET_SELECTION_NONE));
    }

    showSelection(text);
  }

  @Override
  public final void showSelection () {
    showSelection(settingPreference.getValues());
  }

  protected MultipleSelectionSetting (SettingsFragment fragment, int key) {
    super(fragment, key);

    settingPreference.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final Set<String> newSelection = (Set<String>)newValue;

          showSelection(newSelection);
          onSelectionChanged(newSelection);

          return true;
        }
      }
    );
  }
}

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

import android.preference.Preference;
import android.preference.MultiSelectListPreference;
import java.util.Set;

public abstract class MultipleSelectionSetting extends SelectionSetting<MultiSelectListPreference> {
  protected abstract void onSelectionChanged (Set<String> newSelection);

  @Override
  public final CharSequence[] getAllValues () {
    return preference.getEntryValues();
  }

  @Override
  public final CharSequence getValueAt (int index) {
    return getAllValues()[index];
  }

  @Override
  public final int indexOf (String value) {
    return preference.findIndexOfValue(value);
  }

  @Override
  public final CharSequence[] getAllLabels () {
    return preference.getEntries();
  }

  @Override
  public final CharSequence getLabelAt (int index) {
    return getAllLabels()[index];
  }

  public final Set<String> getSelectedValues () {
    return preference.getValues();
  }

  @Override
  protected final void setElements (String[] values, String[] labels) {
    preference.setEntryValues(values);
    preference.setEntries(labels);
  }

  protected final void setSummary (Set<String> values) {
    StringBuilder summary = new StringBuilder();

    for (String value : values) {
      if (value.length() > 0) {
        if (summary.length() > 0) summary.append('\n');
        summary.append(getLabelFor(value));
      }
    }

    if (summary.length() == 0) {
      summary.append(getString(R.string.SET_SELECTION_NONE));
    }

    setSummary(summary);
  }

  @Override
  public final void setSummary () {
    setSummary(getSelectedValues());
  }

  @Override
  public boolean onPreferenceChange (Preference preference, Object newValue) {
    final Set<String> newSelection = (Set<String>)newValue;

    setSummary(newSelection);
    onSelectionChanged(newSelection);

    return true;
  }

  protected MultipleSelectionSetting (SettingsFragment fragment, int key) {
    super(fragment, key);
  }
}

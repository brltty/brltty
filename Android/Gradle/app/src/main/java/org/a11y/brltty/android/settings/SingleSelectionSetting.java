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
import android.preference.ListPreference;

public abstract class SingleSelectionSetting extends SelectionSetting<ListPreference> {
  protected abstract void onSelectionChanged (String newSelection);

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

  public final String getSelectedValue () {
    return preference.getValue();
  }

  public final CharSequence getSelectedLabel () {
    return preference.getEntry();
  }

  @Override
  public final void setSummary () {
    CharSequence label = getSelectedLabel();
    if (label == null) label = "";
    setSummary(label);
  }

  protected final void setSummary (int index) {
    setSummary(getLabelAt(index));
  }

  @Override
  public boolean onPreferenceChange (Preference preference, Object newValue) {
    final String newSelection = (String)newValue;

    setSummary(indexOf(newSelection));
    onSelectionChanged(newSelection);

    return true;
  }

  protected SingleSelectionSetting (SettingsFragment fragment, int key) {
    super(fragment, key);
  }

  @Override
  protected final void setElements (String[] values, String[] labels) {
    preference.setEntryValues(values);
    preference.setEntries(labels);
  }

  protected final void selectFirstElement () {
    preference.setValueIndex(0);
    setSummary();
  }

  public final void selectValue (String newValue) {
    preference.setValue(newValue);
    setSummary();
  }
}

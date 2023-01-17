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
import android.preference.CheckBoxPreference;

public abstract class CheckBoxSetting extends PreferenceSetting<CheckBoxPreference> {
  protected abstract void onStateChanged (boolean newState);

  public final boolean isChecked () {
    return preference.isChecked();
  }

  protected final void setSummary (boolean checked) {
    setSummary(
      getString(
        checked? R.string.checkbox_state_checked: R.string.checkbox_state_unchecked
      )
    );
  }

  @Override
  public final void setSummary () {
    setSummary(isChecked());
  }

  @Override
  public boolean onPreferenceChange (Preference preference, Object newValue) {
    final boolean newState = (Boolean)newValue;

    setSummary(newState);
    onStateChanged(newState);

    return true;
  }

  protected CheckBoxSetting (SettingsFragment fragment, int key) {
    super(fragment, key);
  }
}

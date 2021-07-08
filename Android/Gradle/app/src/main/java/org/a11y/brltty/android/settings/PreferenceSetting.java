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

public abstract class PreferenceSetting<P extends Preference> {
  public abstract void showSelection ();

  protected final SettingsFragment settingsFragment;
  protected final P settingPreference;

  protected PreferenceSetting (SettingsFragment fragment, int key) {
    settingsFragment = fragment;
    settingPreference = (P)settingsFragment.getPreference(key);
    showSelection();
  }

  protected final String getString (int string) {
    return settingsFragment.getString(string);
  }

  protected final void showSelection (CharSequence text) {
    settingPreference.setSummary(text);
  }
}

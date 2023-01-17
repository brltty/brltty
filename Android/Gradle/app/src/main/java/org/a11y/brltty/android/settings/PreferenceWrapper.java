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

public abstract class PreferenceWrapper<P extends Preference> {
  protected final SettingsFragment settingsFragment;
  protected final P preference;

  protected PreferenceWrapper (SettingsFragment fragment, int key) {
    settingsFragment = fragment;
    preference = (P)settingsFragment.getPreference(key);
  }

  protected final String getString (int string) {
    return settingsFragment.getString(string);
  }

  public final CharSequence getSummary () {
    return preference.getSummary();
  }

  public final void setSummary (CharSequence summary) {
    preference.setSummary(summary);
  }

  public final boolean isEnabled () {
    return preference.isEnabled();
  }

  public final void setEnabled (boolean yes) {
    preference.setEnabled(yes);
  }

  public final boolean isSelectable () {
    return preference.isSelectable();
  }

  public final void setSelectable (boolean yes) {
    preference.setSelectable(yes);
  }
}

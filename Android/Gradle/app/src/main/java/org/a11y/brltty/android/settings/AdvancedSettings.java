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

import android.os.Bundle;

public final class AdvancedSettings extends SettingsFragment {
  private KeyboardTableSetting keyboardTableSetting = null;
  private AttributesTableSetting attributesTableSetting = null;

  private LogLevelSetting logLevelSetting = null;
  private LogCategoriesSetting logCategoriesSetting = null;

  private LogAccessibilityEventsSetting logAccessibilityEventsSetting = null;
  private LogRenderedScreenSetting logRenderedScreenSetting = null;
  private LogKeyboardEventsSetting logKeyboardEventsSetting = null;
  private LogUnhandledEventsSetting logUnhandledEventsSetting = null;

  @Override
  public void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.settings_advanced);

    keyboardTableSetting = new KeyboardTableSetting(this);
    attributesTableSetting = new AttributesTableSetting(this);

    logLevelSetting = new LogLevelSetting(this);
    logCategoriesSetting = new LogCategoriesSetting(this);

    logAccessibilityEventsSetting = new LogAccessibilityEventsSetting(this);
    logRenderedScreenSetting = new LogRenderedScreenSetting(this);
    logKeyboardEventsSetting = new LogKeyboardEventsSetting(this);
    logUnhandledEventsSetting = new LogUnhandledEventsSetting(this);
  }
}

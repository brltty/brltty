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
import org.a11y.brltty.core.CoreWrapper;

import java.util.Set;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.MultiSelectListPreference;

public final class AdvancedSettings extends SettingsFragment {
  private final static String LOG_TAG = AdvancedSettings.class.getName();

  private ListPreference keyboardTableList;
  private ListPreference attributesTableList;
  private ListPreference logLevelList;
  private MultiSelectListPreference logCategorySet;
  private CheckBoxPreference logAccessibilityEventsCheckBox;
  private CheckBoxPreference logRenderedScreenCheckBox;
  private CheckBoxPreference logKeyboardEventsCheckBox;
  private CheckBoxPreference logUnhandledEventsCheckBox;

  @Override
  public void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.settings_advanced);

    keyboardTableList = getListPreference(R.string.PREF_KEY_KEYBOARD_TABLE);
    attributesTableList = getListPreference(R.string.PREF_KEY_ATTRIBUTES_TABLE);
    logLevelList = getListPreference(R.string.PREF_KEY_LOG_LEVEL);
    logCategorySet = getMultiSelectListPreference(R.string.PREF_KEY_LOG_CATEGORIES);
    logAccessibilityEventsCheckBox = getCheckBoxPreference(R.string.PREF_KEY_LOG_ACCESSIBILITY_EVENTS);
    logRenderedScreenCheckBox = getCheckBoxPreference(R.string.PREF_KEY_LOG_RENDERED_SCREEN);
    logKeyboardEventsCheckBox = getCheckBoxPreference(R.string.PREF_KEY_LOG_KEYBOARD_EVENTS);
    logUnhandledEventsCheckBox = getCheckBoxPreference(R.string.PREF_KEY_LOG_UNHANDLED_EVENTS);

    sortList(keyboardTableList);
    sortList(attributesTableList);

    showSelection(keyboardTableList);
    showSelection(attributesTableList);
    showSelection(logLevelList);
    showSelection(logCategorySet);
    showSelection(logAccessibilityEventsCheckBox);
    showSelection(logRenderedScreenCheckBox);
    showSelection(logKeyboardEventsCheckBox);
    showSelection(logUnhandledEventsCheckBox);

    keyboardTableList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newTable = (String)newValue;

          CoreWrapper.runOnCoreThread(
            new Runnable() {
              @Override
              public void run () {
                CoreWrapper.changeKeyboardTable(newTable);
              }
            }
          );

          showSelection(keyboardTableList, newTable);
          return true;
        }
      }
    );

    attributesTableList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newTable = (String)newValue;

          CoreWrapper.runOnCoreThread(
            new Runnable() {
              @Override
              public void run () {
                CoreWrapper.changeAttributesTable(newTable);
              }
            }
          );

          showSelection(attributesTableList, newTable);
          return true;
        }
      }
    );

    logLevelList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newLevel = (String)newValue;

          CoreWrapper.runOnCoreThread(
            new Runnable() {
              @Override
              public void run () {
                CoreWrapper.changeLogLevel(newLevel);
              }
            }
          );

          showSelection(logLevelList, newLevel);
          return true;
        }
      }
    );

    logCategorySet.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final Set<String> newCategories = (Set<String>)newValue;

          CoreWrapper.runOnCoreThread(
            new Runnable() {
              @Override
              public void run () {
                CoreWrapper.changeLogCategories(newCategories);
              }
            }
          );

          showSelection(logCategorySet, newCategories);
          return true;
        }
      }
    );

    logAccessibilityEventsCheckBox.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final boolean newSetting = (Boolean)newValue;
          ApplicationSettings.LOG_ACCESSIBILITY_EVENTS = newSetting;

          showSelection(logAccessibilityEventsCheckBox, newSetting);
          return true;
        }
      }
    );

    logRenderedScreenCheckBox.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final boolean newSetting = (Boolean)newValue;
          ApplicationSettings.LOG_RENDERED_SCREEN = newSetting;

          showSelection(logRenderedScreenCheckBox, newSetting);
          return true;
        }
      }
    );

    logKeyboardEventsCheckBox.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final boolean newSetting = (Boolean)newValue;
          ApplicationSettings.LOG_KEYBOARD_EVENTS = newSetting;

          showSelection(logKeyboardEventsCheckBox, newSetting);
          return true;
        }
      }
    );

    logUnhandledEventsCheckBox.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final boolean newSetting = (Boolean)newValue;
          ApplicationSettings.LOG_UNHANDLED_EVENTS = newSetting;

          showSelection(logUnhandledEventsCheckBox, newSetting);
          return true;
        }
      }
    );
  }
}

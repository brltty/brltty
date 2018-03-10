/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brltty.android;
import org.a11y.brltty.core.*;

import java.util.Set;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.MultiSelectListPreference;

public final class AdvancedSettings extends SettingsFragment {
  private static final String LOG_TAG = AdvancedSettings.class.getName();

  private ListPreference keyboardTableList;
  private ListPreference attributesTableList;
  private ListPreference logLevelList;
  private MultiSelectListPreference logCategorySet;
  private CheckBoxPreference logAccessibilityEventsCheckBox;
  private CheckBoxPreference logRenderedScreenCheckBox;
  private CheckBoxPreference logKeyboardEventsCheckBox;

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

    sortList(keyboardTableList);
    sortList(attributesTableList);

    showListSelection(keyboardTableList);
    showListSelection(attributesTableList);
    showListSelection(logLevelList);
    showSetSelections(logCategorySet);

    keyboardTableList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newKeyboardTable = (String)newValue;

          CoreWrapper.runOnCoreThread(new Runnable() {
            @Override
            public void run () {
              CoreWrapper.changeKeyboardTable(newKeyboardTable);
            }
          });

          showListSelection(keyboardTableList, newKeyboardTable);
          return true;
        }
      }
    );

    attributesTableList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newAttributesTable = (String)newValue;

          CoreWrapper.runOnCoreThread(new Runnable() {
            @Override
            public void run () {
              CoreWrapper.changeAttributesTable(newAttributesTable);
            }
          });

          showListSelection(attributesTableList, newAttributesTable);
          return true;
        }
      }
    );

    logLevelList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newLogLevel = (String)newValue;

          CoreWrapper.runOnCoreThread(new Runnable() {
            @Override
            public void run () {
              CoreWrapper.changeLogLevel(newLogLevel);
            }
          });

          showListSelection(logLevelList, newLogLevel);
          return true;
        }
      }
    );

    logCategorySet.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final Set<String> newLogCategories = (Set<String>)newValue;

          CoreWrapper.runOnCoreThread(new Runnable() {
            @Override
            public void run () {
              CoreWrapper.changeLogCategories(newLogCategories);
            }
          });

          showSetSelections(logCategorySet, newLogCategories);
          return true;
        }
      }
    );

    logAccessibilityEventsCheckBox.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          ApplicationSettings.LOG_ACCESSIBILITY_EVENTS = (Boolean)newValue;
          return true;
        }
      }
    );

    logRenderedScreenCheckBox.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          ApplicationSettings.LOG_RENDERED_SCREEN = (Boolean)newValue;
          return true;
        }
      }
    );

    logKeyboardEventsCheckBox.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          ApplicationSettings.LOG_KEYBOARD_EVENTS = (Boolean)newValue;
          return true;
        }
      }
    );
  }
}

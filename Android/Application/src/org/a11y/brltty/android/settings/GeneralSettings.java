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

import android.os.Bundle;
import android.preference.Preference;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;

public final class GeneralSettings extends SettingsFragment {
  private final static String LOG_TAG = GeneralSettings.class.getName();

  private CheckBoxPreference releaseBrailleDeviceCheckBox;
  private ListPreference navigationModeList;
  private ListPreference textTableList;
  private ListPreference contractionTableList;
  private ListPreference speechSupportList;

  @Override
  public void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.settings_general);

    releaseBrailleDeviceCheckBox = getCheckBoxPreference(R.string.PREF_KEY_RELEASE_BRAILLE_DEVICE);
    navigationModeList = getListPreference(R.string.PREF_KEY_NAVIGATION_MODE);
    textTableList = getListPreference(R.string.PREF_KEY_TEXT_TABLE);
    contractionTableList = getListPreference(R.string.PREF_KEY_CONTRACTION_TABLE);
    speechSupportList = getListPreference(R.string.PREF_KEY_SPEECH_SUPPORT);

    sortList(textTableList, 1);
    sortList(contractionTableList);

    showSelection(releaseBrailleDeviceCheckBox);
    showSelection(navigationModeList);
    showSelection(textTableList);
    showSelection(contractionTableList);
    showSelection(speechSupportList);

    releaseBrailleDeviceCheckBox.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final boolean newSetting = (Boolean)newValue;

          ApplicationSettings.RELEASE_BRAILLE_DEVICE = newSetting;
          BrailleNotification.updateState();

          showSelection(releaseBrailleDeviceCheckBox, newSetting);
          return true;
        }
      }
    );

    navigationModeList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newMode = (String)newValue;

          showSelection(navigationModeList, newMode);
          BrailleRenderer.setBrailleRenderer(newMode);
          return true;
        }
      }
    );

    textTableList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newTable = (String)newValue;

          CoreWrapper.runOnCoreThread(
            new Runnable() {
              @Override
              public void run () {
                CoreWrapper.changeTextTable(newTable);
              }
            }
          );

          showSelection(textTableList, newTable);
          return true;
        }
      }
    );

    contractionTableList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newTable = (String)newValue;

          CoreWrapper.runOnCoreThread(
            new Runnable() {
              @Override
              public void run () {
                CoreWrapper.changeContractionTable(newTable);
              }
            }
          );

          showSelection(contractionTableList, newTable);
          return true;
        }
      }
    );

    speechSupportList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newDriver = (String)newValue;

          CoreWrapper.runOnCoreThread(
            new Runnable() {
              @Override
              public void run () {
                CoreWrapper.changeSpeechDriver(newDriver);
                CoreWrapper.restartSpeechDriver();
              }
            }
          );

          showSelection(speechSupportList, newDriver);
          return true;
        }
      }
    );
  }
}

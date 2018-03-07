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

import android.os.Bundle;
import android.preference.Preference;
import android.preference.ListPreference;

public final class GeneralSettings extends SettingsFragment {
  private static final String LOG_TAG = GeneralSettings.class.getName();

  protected ListPreference navigationModeList;
  protected ListPreference textTableList;
  protected ListPreference contractionTableList;
  protected ListPreference speechSupportList;

  @Override
  public void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    addPreferencesFromResource(R.xml.settings_general);

    navigationModeList = getListPreference(R.string.PREF_KEY_NAVIGATION_MODE);
    textTableList = getListPreference(R.string.PREF_KEY_TEXT_TABLE);
    contractionTableList = getListPreference(R.string.PREF_KEY_CONTRACTION_TABLE);
    speechSupportList = getListPreference(R.string.PREF_KEY_SPEECH_SUPPORT);

    sortList(textTableList, 1);
    sortList(contractionTableList);

    showListSelection(navigationModeList);
    showListSelection(textTableList);
    showListSelection(contractionTableList);
    showListSelection(speechSupportList);

    navigationModeList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newNavigationMode = (String)newValue;

          showListSelection(navigationModeList, newNavigationMode);
          BrailleRenderer.setBrailleRenderer(newNavigationMode);
          return true;
        }
      }
    );

    textTableList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newTextTable = (String)newValue;

          CoreWrapper.runOnCoreThread(new Runnable() {
            @Override
            public void run () {
              CoreWrapper.changeTextTable(newTextTable);
            }
          });

          showListSelection(textTableList, newTextTable);
          return true;
        }
      }
    );

    contractionTableList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newContractionTable = (String)newValue;

          CoreWrapper.runOnCoreThread(new Runnable() {
            @Override
            public void run () {
              CoreWrapper.changeContractionTable(newContractionTable);
            }
          });

          showListSelection(contractionTableList, newContractionTable);
          return true;
        }
      }
    );

    speechSupportList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newSpeechSupport = (String)newValue;

          CoreWrapper.runOnCoreThread(new Runnable() {
            @Override
            public void run () {
              CoreWrapper.changeSpeechDriver(newSpeechSupport);
              CoreWrapper.restartSpeechDriver();
            }
          });

          showListSelection(speechSupportList, newSpeechSupport);
          return true;
        }
      }
    );
  }
}

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

import java.util.Collections;

import java.util.Map;
import java.util.LinkedHashMap;

import java.util.Set;
import java.util.TreeSet;

import android.os.Bundle;
import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceScreen;
import android.preference.Preference;
import android.preference.EditTextPreference;
import android.preference.ListPreference;

public final class DeviceManager extends SettingsFragment {
  private static final String LOG_TAG = DeviceManager.class.getName();

  private Set<String> deviceNames;
  private DeviceCollection deviceCollection;

  private ListPreference selectedDeviceList;
  private PreferenceScreen addDeviceScreen;
  private PreferenceScreen removeDeviceScreen;

  private EditTextPreference deviceNameEditor;
  private ListPreference deviceMethodList;
  private ListPreference deviceIdentifierList;
  private ListPreference deviceDriverList;
  private Preference addDeviceButton;

  private Preference removeDeviceButton_ASK;
  private Preference removeDeviceButton_YES;
  private Preference removeDeviceButton_NO;

  private static final String PREF_KEY_DEVICE_NAMES = "device-names";

  public static final String PREF_KEY_DEVICE_QUALIFIER = "device-qualifier";
  public static final String PREF_KEY_DEVICE_REFERENCE = "device-reference";
  public static final String PREF_KEY_DEVICE_DRIVER = "device-driver";

  private static final String[] DEVICE_PROPERTY_KEYS = {
    PREF_KEY_DEVICE_QUALIFIER,
    PREF_KEY_DEVICE_REFERENCE,
    PREF_KEY_DEVICE_DRIVER
  };

  public static Map<String, String> getDeviceProperties (SharedPreferences prefs, String name) {
    return getProperties(prefs, name, DEVICE_PROPERTY_KEYS);
  }

  private static void removeDeviceProperties (SharedPreferences.Editor editor, String name) {
    removeProperties(editor, name, DEVICE_PROPERTY_KEYS);
  }

  private void updateRemoveDeviceScreen (String selectedDevice) {
    boolean on = false;

    if (selectedDeviceList.isEnabled()) {
      if (selectedDevice != null) {
        if (selectedDevice.length() > 0) {
          on = true;
          removeDeviceButton_ASK.setSummary(selectedDevice);
        }
      }
    }

    removeDeviceScreen.setSelectable(on);
  }

  private void updateRemoveDeviceScreen () {
    updateRemoveDeviceScreen(selectedDeviceList.getValue());
  }

  private void updateSelectedDeviceList () {
    boolean haveDevices = !deviceNames.isEmpty();
    selectedDeviceList.setEnabled(haveDevices);
    CharSequence summary;

    if (haveDevices) {
      {
        String[] names = new String[deviceNames.size()];
        deviceNames.toArray(names);
        setListElements(selectedDeviceList, names);
        sortList(selectedDeviceList);
      }

      summary = selectedDeviceList.getEntry();
      if ((summary == null) || (summary.length() == 0)) {
        summary = getString(R.string.SELECTED_DEVICE_UNSELECTED);
      }
    } else {
      summary = getString(R.string.SELECTED_DEVICE_NONE);
    }

    selectedDeviceList.setSummary(summary);
    updateRemoveDeviceScreen();
  }

  private String getDeviceMethod () {
    return deviceMethodList.getValue();
  }

  private DeviceCollection makeDeviceCollection (String deviceMethod) {
    String className = getClass().getPackage().getName() + "." + deviceMethod + "DeviceCollection";

    String[] argumentTypes = new String[] {
      "android.content.Context"
    };

    Object[] arguments = new Object[] {
      getActivity()
    };

    return (DeviceCollection)LanguageUtilities.newInstance(className, argumentTypes, arguments);
  }

  private void updateDeviceIdentifierList (String deviceMethod) {
    deviceCollection = makeDeviceCollection(deviceMethod);

    setListElements(
      deviceIdentifierList,
      deviceCollection.getIdentifierValues(), 
      deviceCollection.getIdentifierLabels()
    );

    sortList(deviceIdentifierList);

    {
      boolean haveIdentifiers = deviceIdentifierList.getEntryValues().length > 0;
      deviceIdentifierList.setEnabled(haveIdentifiers);

      if (haveIdentifiers) {
        resetList(deviceIdentifierList);
      } else {
        deviceIdentifierList.setSummary(getString(R.string.ADD_DEVICE_NO_DEVICES));
      }
    }

    resetList(deviceDriverList);
  }

  private void updateDeviceName (String name) {
    String problem;

    if (!deviceMethodList.isEnabled()) {
      problem = getString(R.string.ADD_DEVICE_UNSELECTED_METHOD);
    } else if (!deviceIdentifierList.isEnabled()) {
      problem = getString(R.string.ADD_DEVICE_UNSELECTED_DEVICE);
    } else if (!deviceDriverList.isEnabled()) {
      problem = getString(R.string.ADD_DEVICE_UNSELECTED_DRIVER);
    } else {
      if (name.length() == 0) {
        name = deviceDriverList.getSummary()
             + " " + deviceMethodList.getSummary()
             + " " + deviceIdentifierList.getSummary()
             ;
      }

      if (deviceNames.contains(name)) {
        problem = getString(R.string.ADD_DEVICE_DUPLICATE_NAME);
      } else {
        problem = "";
      }
    }

    addDeviceButton.setSummary(problem);
    addDeviceButton.setEnabled(problem.length() == 0);
    deviceNameEditor.setSummary(name);
  }

  private void updateDeviceName () {
    updateDeviceName(deviceNameEditor.getEditText().getText().toString());
  }

  @Override
  public void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    addPreferencesFromResource(R.xml.settings_devices);

    selectedDeviceList = getListPreference(R.string.PREF_KEY_SELECTED_DEVICE);
    addDeviceScreen = getPreferenceScreen(R.string.PREF_KEY_ADD_DEVICE);
    removeDeviceScreen = getPreferenceScreen(R.string.PREF_KEY_REMOVE_DEVICE);

    deviceNameEditor = getEditTextPreference(R.string.PREF_KEY_DEVICE_NAME);
    deviceMethodList = getListPreference(R.string.PREF_KEY_DEVICE_METHOD);
    deviceIdentifierList = getListPreference(R.string.PREF_KEY_DEVICE_IDENTIFIER);
    deviceDriverList = getListPreference(R.string.PREF_KEY_DEVICE_DRIVER);
    addDeviceButton = getPreference(R.string.PREF_KEY_DEVICE_ADD);

    removeDeviceButton_ASK = getPreference(R.string.PREF_KEY_REMOVE_DEVICE_ASK);
    removeDeviceButton_YES = getPreference(R.string.PREF_KEY_REMOVE_DEVICE_YES);
    removeDeviceButton_NO = getPreference(R.string.PREF_KEY_REMOVE_DEVICE_NO);

    sortList(deviceDriverList, 1);

    {
      SharedPreferences prefs = getSharedPreferences();
      deviceNames = new TreeSet<String>(prefs.getStringSet(PREF_KEY_DEVICE_NAMES, Collections.EMPTY_SET));
    }

    updateSelectedDeviceList();
    showSelection(deviceMethodList);
    updateDeviceIdentifierList(getDeviceMethod());
    updateDeviceName();

    selectedDeviceList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newSelectedDevice = (String)newValue;
          BrailleNotification.setDevice(newSelectedDevice);

          CoreWrapper.runOnCoreThread(new Runnable() {
            @Override
            public void run () {
              Map<String, String> properties = getDeviceProperties(
                ApplicationUtilities.getSharedPreferences(),
                newSelectedDevice
              );

              String qualifier = properties.get(PREF_KEY_DEVICE_QUALIFIER);
              String reference = properties.get(PREF_KEY_DEVICE_REFERENCE);
              String driver = properties.get(PREF_KEY_DEVICE_DRIVER);

              CoreWrapper.changeBrailleDevice(qualifier, reference);
              CoreWrapper.changeBrailleDriver(driver);
              CoreWrapper.restartBrailleDriver();
            }
          });

          selectedDeviceList.setSummary(newSelectedDevice);
          updateRemoveDeviceScreen(newSelectedDevice);
          return true;
        }
      }
    );

    deviceNameEditor.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newDeviceName = (String)newValue;

          updateDeviceName(newDeviceName);
          return true;
        }
      }
    );

    deviceMethodList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newDeviceMethod = (String)newValue;

          showSelection(deviceMethodList, newDeviceMethod);
          updateDeviceIdentifierList(newDeviceMethod);
          updateDeviceName();
          return true;
        }
      }
    );

    deviceIdentifierList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newDeviceIdentifier = (String)newValue;

          showSelection(deviceIdentifierList, newDeviceIdentifier);
          updateDeviceName();
          return true;
        }
      }
    );

    deviceDriverList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newDeviceDriver = (String)newValue;

          showSelection(deviceDriverList, newDeviceDriver);
          updateDeviceName();
          return true;
        }
      }
    );

    addDeviceButton.setOnPreferenceClickListener(
      new Preference.OnPreferenceClickListener() {
        @Override
        public boolean onPreferenceClick (Preference preference) {
          String name = deviceNameEditor.getSummary().toString();
          deviceNames.add(name);
          updateSelectedDeviceList();
          updateDeviceName();

          {
            SharedPreferences.Editor editor = preference.getEditor();

            Map<String, String> properties = new LinkedHashMap();
            properties.put(PREF_KEY_DEVICE_QUALIFIER, deviceCollection.getMethodQualifier());
            properties.put(PREF_KEY_DEVICE_REFERENCE, deviceCollection.makeDeviceReference(deviceIdentifierList.getValue()));
            properties.put(PREF_KEY_DEVICE_DRIVER, deviceDriverList.getValue());
            putProperties(editor, name, properties);

            editor.putStringSet(PREF_KEY_DEVICE_NAMES, deviceNames);
            editor.apply();
          }

          addDeviceScreen.getDialog().dismiss();
          return true;
        }
      }
    );

    removeDeviceButton_YES.setOnPreferenceClickListener(
      new Preference.OnPreferenceClickListener() {
        @Override
        public boolean onPreferenceClick (Preference preference) {
          String name = selectedDeviceList.getValue();

          if (name != null) {
            BrailleNotification.unsetDevice();
            deviceNames.remove(name);
            selectedDeviceList.setValue("");
            updateSelectedDeviceList();
            updateDeviceName();

            {
              SharedPreferences.Editor editor = preference.getEditor();
              editor.putStringSet(PREF_KEY_DEVICE_NAMES, deviceNames);
              removeDeviceProperties(editor, name);
              editor.apply();
            }
          }

          removeDeviceScreen.getDialog().dismiss();
          return true;
        }
      }
    );

    removeDeviceButton_NO.setOnPreferenceClickListener(
      new Preference.OnPreferenceClickListener() {
        @Override
        public boolean onPreferenceClick (Preference preference) {
          removeDeviceScreen.getDialog().dismiss();
          return true;
        }
      }
    );
  }
}

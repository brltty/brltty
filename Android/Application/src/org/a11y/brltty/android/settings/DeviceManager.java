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

import java.util.Collections;

import java.util.Map;
import java.util.LinkedHashMap;

import java.util.Set;
import java.util.TreeSet;

import android.util.Log;
import android.os.Bundle;
import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceScreen;
import android.preference.Preference;
import android.preference.EditTextPreference;
import android.preference.ListPreference;

public final class DeviceManager extends SettingsFragment {
  private final static String LOG_TAG = DeviceManager.class.getName();

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
  private Preference removeDeviceButton_NO;
  private Preference removeDeviceButton_YES;

  private final static String PREF_KEY_DEVICE_NAMES = "device-names";
  private final static String PREF_KEY_DEVICE_IDENTIFIER = "device-identifier";
  private final static String PREF_KEY_DEVICE_QUALIFIER = "device-qualifier";
  private final static String PREF_KEY_DEVICE_REFERENCE = "device-reference";
  private final static String PREF_KEY_DEVICE_DRIVER = "device-driver";

  private final static String[] DEVICE_PROPERTY_KEYS = {
    PREF_KEY_DEVICE_IDENTIFIER,
    PREF_KEY_DEVICE_QUALIFIER,
    PREF_KEY_DEVICE_REFERENCE,
    PREF_KEY_DEVICE_DRIVER
  };

  private static Map<String, String> getDeviceProperties (SharedPreferences prefs, String name) {
    return getProperties(prefs, name, DEVICE_PROPERTY_KEYS);
  }

  private static void removeDeviceProperties (SharedPreferences.Editor editor, String name) {
    removeProperties(editor, name, DEVICE_PROPERTY_KEYS);
  }

  private static String getSelectedDevice (SharedPreferences prefs) {
    Context context = BrailleApplication.get();
    String key = context.getResources().getString(R.string.PREF_KEY_SELECTED_DEVICE);
    return prefs.getString(key, "");
  }

  public static String getSelectedDevice () {
    return getSelectedDevice(DataType.getPreferences());
  }

  private static DeviceDescriptor getDeviceDescriptor (SharedPreferences prefs, String device) {
    String identifier = "";
    String driver = "";

    if (!device.isEmpty()) {
      Map<String, String> properties = getDeviceProperties(prefs, device);
      identifier = properties.get(PREF_KEY_DEVICE_IDENTIFIER);
      driver = properties.get(PREF_KEY_DEVICE_DRIVER);

      if (identifier.isEmpty()) {
        String qualifier = properties.get(PREF_KEY_DEVICE_QUALIFIER);

        if (!qualifier.isEmpty()) {
          StringBuilder sb = new StringBuilder();
          sb.append(qualifier);
          sb.append(DeviceCollection.QUALIFIER_DELIMITER);
          sb.append(properties.get(PREF_KEY_DEVICE_REFERENCE));
          identifier = sb.toString();
        }
      }
    }

    if (identifier.isEmpty()) {
      StringBuilder sb = new StringBuilder();
      sb.append(BluetoothDeviceCollection.DEVICE_QUALIFIER);
      sb.append(DeviceCollection.QUALIFIER_DELIMITER);
      sb.append(DeviceCollection.IDENTIFIER_SEPARATOR);
      sb.append(UsbDeviceCollection.DEVICE_QUALIFIER);
      sb.append(DeviceCollection.QUALIFIER_DELIMITER);
      identifier = sb.toString();
    }

    if (driver.isEmpty()) {
      driver = "auto";
    }

    return new DeviceDescriptor(identifier, driver);
  }

  private static DeviceDescriptor getDeviceDescriptor (SharedPreferences prefs) {
    return getDeviceDescriptor(prefs, getSelectedDevice(prefs));
  }

  public static DeviceDescriptor getDeviceDescriptor () {
    return getDeviceDescriptor(DataType.getPreferences());
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

    return (DeviceCollection)LanguageUtilities.newInstance(
      className, argumentTypes, arguments
    );
  }

  private void updateDeviceIdentifierList (String deviceMethod) {
    deviceCollection = makeDeviceCollection(deviceMethod);

    setListElements(
      deviceIdentifierList,
      deviceCollection.makeValues(), 
      deviceCollection.makeLabels()
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

  private static void restartBrailleDriver (final SharedPreferences prefs, final String device) {
    CoreWrapper.runOnCoreThread(
      new Runnable() {
        @Override
        public void run () {
          DeviceDescriptor descriptor = getDeviceDescriptor(prefs, device);
          CoreWrapper.changeBrailleDevice(descriptor.getIdentifier());
          CoreWrapper.changeBrailleDriver(descriptor.getDriver());
          CoreWrapper.restartBrailleDriver();
        }
      }
    );
  }

  @Override
  public void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.settings_devices);
    final SharedPreferences prefs = getPreferences();

    selectedDeviceList = getListPreference(R.string.PREF_KEY_SELECTED_DEVICE);
    addDeviceScreen = getPreferenceScreen(R.string.PREF_KEY_ADD_DEVICE);
    removeDeviceScreen = getPreferenceScreen(R.string.PREF_KEY_REMOVE_DEVICE);

    deviceNameEditor = getEditTextPreference(R.string.PREF_KEY_DEVICE_NAME);
    deviceMethodList = getListPreference(R.string.PREF_KEY_DEVICE_METHOD);
    deviceIdentifierList = getListPreference(R.string.PREF_KEY_DEVICE_IDENTIFIER);
    deviceDriverList = getListPreference(R.string.PREF_KEY_DEVICE_DRIVER);
    addDeviceButton = getPreference(R.string.PREF_KEY_DEVICE_ADD);

    removeDeviceButton_ASK = getPreference(R.string.PREF_KEY_REMOVE_DEVICE_ASK);
    removeDeviceButton_NO = getPreference(R.string.PREF_KEY_REMOVE_DEVICE_NO);
    removeDeviceButton_YES = getPreference(R.string.PREF_KEY_REMOVE_DEVICE_YES);

    sortList(deviceDriverList, 1);
    deviceNames = new TreeSet<String>(prefs.getStringSet(PREF_KEY_DEVICE_NAMES, Collections.EMPTY_SET));

    updateSelectedDeviceList();
    showSelection(deviceMethodList);
    updateDeviceIdentifierList(getDeviceMethod());
    updateDeviceName();

    selectedDeviceList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newDevice = (String)newValue;
          BrailleNotification.updateDevice(newDevice);
          selectedDeviceList.setSummary(newDevice);
          updateRemoveDeviceScreen(newDevice);
          restartBrailleDriver(prefs, newDevice);
          return true;
        }
      }
    );

    deviceNameEditor.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newName = (String)newValue;

          updateDeviceName(newName);
          return true;
        }
      }
    );

    deviceMethodList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newMethod = (String)newValue;

          showSelection(deviceMethodList, newMethod);
          updateDeviceIdentifierList(newMethod);
          updateDeviceName();
          return true;
        }
      }
    );

    deviceIdentifierList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newIdentifier = (String)newValue;

          showSelection(deviceIdentifierList, newIdentifier);
          updateDeviceName();
          return true;
        }
      }
    );

    deviceDriverList.setOnPreferenceChangeListener(
      new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          final String newDriver = (String)newValue;

          showSelection(deviceDriverList, newDriver);
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

            {
              Map<String, String> properties = new LinkedHashMap();

              properties.put(
                PREF_KEY_DEVICE_IDENTIFIER,
                deviceCollection.makeIdentifier(deviceIdentifierList.getValue())
              );

              properties.put(
                PREF_KEY_DEVICE_DRIVER,
                deviceDriverList.getValue()
              );

              putProperties(editor, name, properties);
            }

            editor.putStringSet(PREF_KEY_DEVICE_NAMES, deviceNames);
            editor.apply();
          }

          addDeviceScreen.getDialog().dismiss();
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

    removeDeviceButton_YES.setOnPreferenceClickListener(
      new Preference.OnPreferenceClickListener() {
        @Override
        public boolean onPreferenceClick (Preference preference) {
          String name = selectedDeviceList.getValue();

          if (name != null) {
            BrailleNotification.updateDevice(null);
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

            restartBrailleDriver(prefs, "");
          }

          removeDeviceScreen.getDialog().dismiss();
          return true;
        }
      }
    );
  }
}

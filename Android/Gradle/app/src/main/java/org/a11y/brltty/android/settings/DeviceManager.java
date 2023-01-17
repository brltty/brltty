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
import org.a11y.brltty.core.CoreWrapper;

import java.util.Collections;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.Set;
import java.util.TreeSet;

import android.util.Log;
import android.os.Bundle;
import android.os.AsyncTask;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceScreen;
import android.preference.Preference;

public final class DeviceManager extends SettingsFragment {
  private final static String LOG_TAG = DeviceManager.class.getName();

  private Set<String> deviceNames;
  private DeviceCollection deviceCollection;

  private SingleSelectionSetting selectedDeviceSetting;
  private PreferenceScreen addDeviceScreen;
  private PreferenceScreen removeDeviceScreen;

  private TextSetting deviceNameSetting;
  private SingleSelectionSetting deviceMethodSetting;
  private SingleSelectionSetting deviceIdentifierSetting;
  private SingleSelectionSetting deviceDriverSetting;
  private PreferenceButton addDeviceButton;

  private Preference removeDevicePrompt;
  private PreferenceButton removeDeviceCancellationButton;
  private PreferenceButton removeDeviceConfirmationButton;

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

    if (selectedDeviceSetting.isEnabled()) {
      if (selectedDevice != null) {
        if (selectedDevice.length() > 0) {
          on = true;
          removeDevicePrompt.setSummary(selectedDevice);
        }
      }
    }

    removeDeviceScreen.setSelectable(on);
  }

  private void updateRemoveDeviceScreen () {
    updateRemoveDeviceScreen(selectedDeviceSetting.getSelectedValue());
  }

  private void updateSelectedDeviceSetting () {
    boolean haveDevices = !deviceNames.isEmpty();
    selectedDeviceSetting.setEnabled(haveDevices);
    CharSequence summary;

    if (haveDevices) {
      {
        String[] names = new String[deviceNames.size()];
        deviceNames.toArray(names);
        selectedDeviceSetting.setElements(names);
        selectedDeviceSetting.sortElementsByLabel();
      }

      summary = selectedDeviceSetting.getSelectedLabel();
      if ((summary == null) || (summary.length() == 0)) {
        summary = getString(R.string.SELECTED_DEVICE_UNSELECTED);
      }
    } else {
      summary = getString(R.string.SELECTED_DEVICE_NONE);
    }

    selectedDeviceSetting.setSummary(summary);
    updateRemoveDeviceScreen();
  }

  private String getDeviceMethod () {
    return deviceMethodSetting.getSelectedValue();
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

  private void updateDeviceIdentifierSetting (String deviceMethod) {
    deviceCollection = makeDeviceCollection(deviceMethod);

    deviceIdentifierSetting.setElements(
      deviceCollection.makeValues(), 
      deviceCollection.makeLabels()
    );

    deviceIdentifierSetting.sortElementsByLabel();

    {
      boolean haveIdentifiers = deviceIdentifierSetting.getElementCount() > 0;
      deviceIdentifierSetting.setEnabled(haveIdentifiers);

      if (haveIdentifiers) {
        deviceIdentifierSetting.selectFirstElement();
      } else {
        deviceIdentifierSetting.setSummary(getString(R.string.ADD_DEVICE_NO_DEVICES));
      }
    }

    deviceDriverSetting.selectFirstElement();
  }

  private void updateDeviceName (String name) {
    String problem;

    if (!deviceMethodSetting.isEnabled()) {
      problem = getString(R.string.ADD_DEVICE_UNSELECTED_METHOD);
    } else if (!deviceIdentifierSetting.isEnabled()) {
      problem = getString(R.string.ADD_DEVICE_UNSELECTED_DEVICE);
    } else if (!deviceDriverSetting.isEnabled()) {
      problem = getString(R.string.ADD_DEVICE_UNSELECTED_DRIVER);
    } else {
      if (name.length() == 0) {
        name = new StringBuilder()
              .append(deviceDriverSetting.getSummary())
              .append(' ')
              .append(deviceMethodSetting.getSummary())
              .append(' ')
              .append(deviceIdentifierSetting.getSummary())
              .toString();
      }

      if (deviceNames.contains(name)) {
        problem = getString(R.string.ADD_DEVICE_DUPLICATE_NAME);
      } else {
        problem = "";
      }
    }

    addDeviceButton.setSummary(problem);
    addDeviceButton.setEnabled(problem.isEmpty());
    deviceNameSetting.setSummary(name);
  }

  private void updateDeviceName () {
    updateDeviceName(deviceNameSetting.getText().toString());
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

    addDeviceScreen = getPreferenceScreen(R.string.PREF_KEY_ADD_DEVICE);
    removeDeviceScreen = getPreferenceScreen(R.string.PREF_KEY_REMOVE_DEVICE);
    removeDevicePrompt = getPreference(R.string.PREF_KEY_REMOVE_DEVICE_PROMPT);

    selectedDeviceSetting = new SingleSelectionSetting(this, R.string.PREF_KEY_SELECTED_DEVICE) {
      @Override
      public void onSelectionChanged (String newDevice) {
        BrailleNotification.updateDevice(newDevice);
        updateRemoveDeviceScreen(newDevice);
        restartBrailleDriver(prefs, newDevice);
      }
    };

    deviceNameSetting = new TextSetting(this, R.string.PREF_KEY_DEVICE_NAME) {
      @Override
      public void onTextChanged (String newName) {
        updateDeviceName(newName);
      }
    };

    deviceMethodSetting = new SingleSelectionSetting(this, R.string.PREF_KEY_DEVICE_METHOD) {
      @Override
      public void onSelectionChanged (String newMethod) {
        updateDeviceIdentifierSetting(newMethod);
        updateDeviceName();
      }
    };

    deviceIdentifierSetting = new SingleSelectionSetting(this, R.string.PREF_KEY_DEVICE_IDENTIFIER) {
      @Override
      public void onSelectionChanged (String newIdentifier) {
        updateDeviceName();
      }
    };

    deviceDriverSetting = new SingleSelectionSetting(this, R.string.PREF_KEY_DEVICE_DRIVER) {
      @Override
      public void onSelectionChanged (String newDriver) {
        updateDeviceName();
      }
    };

    addDeviceButton = new PreferenceButton(this, R.string.PREF_KEY_DEVICE_ADD) {
      @Override
      public void onButtonClicked () {
        new AsyncTask<Object, Object, String>() {
          String name;

          @Override
          protected void onPreExecute () {
            name = deviceNameSetting.getSummary().toString();
          }

          @Override
          protected String doInBackground (Object... arguments) {
            try {
              return deviceCollection.makeIdentifier(deviceIdentifierSetting.getSelectedValue());
            } catch (SecurityException exception) {
              return null;
            }
          }

          @Override
          protected void onPostExecute (String identifier) {
            if (identifier == null) {
              showProblem(R.string.ADD_DEVICE_NO_PERMISSION, name);
            } else {
              deviceNames.add(name);
              updateSelectedDeviceSetting();
              updateDeviceName();

              {
                final SharedPreferences.Editor editor = preference.getEditor();

                {
                  Map<String, String> properties = new LinkedHashMap();
                  properties.put(PREF_KEY_DEVICE_IDENTIFIER, identifier);

                  properties.put(
                    PREF_KEY_DEVICE_DRIVER,
                    deviceDriverSetting.getSelectedValue()
                  );

                  putProperties(editor, name, properties);
                }

                editor.putStringSet(PREF_KEY_DEVICE_NAMES, deviceNames);
                editor.apply();
              }

              dismissScreen();
            }
          }
        }.execute();
      }
    };

    removeDeviceCancellationButton = new PreferenceButton (this, R.string.PREF_KEY_REMOVE_DEVICE_CANCEL) {
      @Override
      public void onButtonClicked () {
        dismissScreen();
      }
    };

    removeDeviceConfirmationButton = new PreferenceButton(this, R.string.PREF_KEY_REMOVE_DEVICE_CONFIRM) {
      @Override
      public void onButtonClicked () {
        String name = selectedDeviceSetting.getSelectedValue();

        if (name != null) {
          BrailleNotification.updateDevice(null);
          deviceNames.remove(name);
          selectedDeviceSetting.selectValue("");
          updateSelectedDeviceSetting();
          updateDeviceName();

          {
            SharedPreferences.Editor editor = preference.getEditor();
            editor.putStringSet(PREF_KEY_DEVICE_NAMES, deviceNames);
            removeDeviceProperties(editor, name);
            editor.apply();
          }

          restartBrailleDriver(prefs, "");
        }

        dismissScreen();
      }
    };

    deviceDriverSetting.sortElementsByLabel(1);
    deviceNames = new TreeSet<String>(prefs.getStringSet(PREF_KEY_DEVICE_NAMES, Collections.EMPTY_SET));

    updateSelectedDeviceSetting();
    updateDeviceIdentifierSetting(getDeviceMethod());
    updateDeviceName();
  }
}

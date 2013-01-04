/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brltty.android;

import java.util.Arrays;
import java.util.Collections;
import java.util.Collection;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.Set;
import java.util.TreeSet;

import android.util.Log;
import android.os.Bundle;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.preference.Preference;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.ListPreference;
import android.preference.MultiSelectListPreference;

import android.bluetooth.*;
import android.hardware.usb.*;

public class SettingsActivity extends PreferenceActivity {
  private static final String LOG_TAG = SettingsActivity.class.getName();

  public static <T> String[] makeStringArray (Collection<T> collection, StringMaker<T> stringMaker) {
    List<String> strings = new ArrayList<String>(collection.size());

    for (T element : collection) {
      strings.add(stringMaker.makeString(element));
    }

    return strings.toArray(new String[strings.size()]);
  }

  public static final String PREF_KEY_DEVICE_QUALIFIER = "device-qualifier-%1$s";
  public static final String PREF_KEY_DEVICE_REFERENCE = "device-reference-%1$s";
  public static final String PREF_KEY_DEVICE_DRIVER = "device-driver-%1$s";

  public static final String[] devicePropertyKeys = {
    PREF_KEY_DEVICE_QUALIFIER,
    PREF_KEY_DEVICE_REFERENCE,
    PREF_KEY_DEVICE_DRIVER
  };

  public static Map<String, String> getProperties (String owner, String[] keys, SharedPreferences prefs) {
    Map<String, String> properties = new LinkedHashMap();

    for (String key : keys) {
      properties.put(key, prefs.getString(String.format(key, owner), ""));
    }

    return properties;
  }

  public static void putProperties (String owner, Map<String, String> properties, SharedPreferences.Editor editor) {
    for (Map.Entry<String, String> property : properties.entrySet()) {
      editor.putString(String.format(property.getKey(), owner), property.getValue());
    }
  }

  public static void removeProperties (String owner, String[] keys, SharedPreferences.Editor editor) {
    for (String key : keys) {
      editor.remove(String.format(key, owner));
    }
  }

  @Override
  protected void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
  }

  @Override
  public void onBuildHeaders (List<Header> headers) {
    loadHeadersFromResource(R.xml.settings_headers, headers);
  }

  public static abstract class SettingsFragment extends PreferenceFragment {
    protected final String LOG_TAG = this.getClass().getName();

    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);
    }

    protected Preference getPreference (int key) {
      return findPreference(getResources().getString(key));
    }

    protected PreferenceScreen getPreferenceScreen (int key) {
      return (PreferenceScreen)getPreference(key);
    }

    protected CheckBoxPreference getCheckBoxPreference (int key) {
      return (CheckBoxPreference)getPreference(key);
    }

    protected EditTextPreference getEditTextPreference (int key) {
      return (EditTextPreference)getPreference(key);
    }

    protected ListPreference getListPreference (int key) {
      return (ListPreference)getPreference(key);
    }

    protected MultiSelectListPreference getMultiSelectListPreference (int key) {
      return (MultiSelectListPreference)getPreference(key);
    }

    protected void showListSelection (ListPreference list) {
      CharSequence label = list.getEntry();

      if (label == null) {
        label = "";
      }

      list.setSummary(label);
    }

    protected void showListSelection (ListPreference list, int index) {
      list.setSummary(list.getEntries()[index]);
    }

    protected int getListIndex (ListPreference list, String value) {
      return Arrays.asList(list.getEntryValues()).indexOf(value);
    }

    protected void showListSelection (ListPreference list, String value) {
      showListSelection(list, getListIndex(list, value));
    }

    protected void showSetSelections (MultiSelectListPreference set, Set<String> values) {
      StringBuilder label = new StringBuilder();

      if (values.size() > 0) {
        CharSequence[] labels = set.getEntries();

        for (String value : values) {
          if (value.length() > 0) {
            if (label.length() > 0) {
              label.append('\n');
            }

            label.append(labels[set.findIndexOfValue(value)]);
          }
        }
      } else {
        label.append("none selected");
      }

      set.setSummary(label.toString());
    }

    protected void showSetSelections (MultiSelectListPreference set) {
      showSetSelections(set, set.getValues());
    }

    protected void resetList (ListPreference list) {
      list.setValueIndex(0);
      showListSelection(list);
    }

    protected void setListElements (ListPreference list, String[] values, String[] labels) {
      list.setEntryValues(values);
      list.setEntries(labels);
    }

    protected void setListElements (ListPreference list, String[] values) {
      setListElements(list, values, values);
    }

    protected SharedPreferences getSharedPreferences () {
      return getPreferenceManager().getDefaultSharedPreferences(getActivity());
    }
  }

  public static final class GeneralSettings extends SettingsFragment {
    protected ListPreference textTableList;
    protected ListPreference contractionTableList;

    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      addPreferencesFromResource(R.xml.settings_general);

      textTableList = getListPreference(R.string.PREF_KEY_TEXT_TABLE);
      contractionTableList = getListPreference(R.string.PREF_KEY_CONTRACTION_TABLE);

      showListSelection(textTableList);
      showListSelection(contractionTableList);

      textTableList.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            showListSelection(textTableList, (String)newValue);
            return true;
          }
        }
      );

      contractionTableList.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            showListSelection(contractionTableList, (String)newValue);
            return true;
          }
        }
      );
    }
  }

  public static final class DeviceManager extends SettingsFragment {
    protected Set<String> deviceNames;
    protected DeviceCollection deviceCollection;

    protected ListPreference selectedDeviceList;
    protected PreferenceScreen addDeviceScreen;
    protected PreferenceScreen removeDeviceScreen;

    protected EditTextPreference deviceNameEditor;
    protected ListPreference deviceMethodList;
    protected ListPreference deviceIdentifierList;
    protected ListPreference deviceDriverList;
    protected Preference addDeviceButton;

    protected Preference removeDeviceButton_ASK;
    protected Preference removeDeviceButton_YES;
    protected Preference removeDeviceButton_NO;

    protected static final String PREF_KEY_DEVICE_NAMES = "device-names";

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
        }

        summary = selectedDeviceList.getEntry();
        if ((summary == null) || (summary.length() == 0)) {
          summary = "not selected";
        }
      } else {
        summary = "no devices";
      }

      selectedDeviceList.setSummary(summary);
      updateRemoveDeviceScreen();
    }

    private String getDeviceMethod () {
      return deviceMethodList.getValue();
    }

    public interface DeviceCollection {
      public String[] getIdentifierValues ();
      public String[] getIdentifierLabels ();
      public String getMethodQualifier ();
      public String getDeviceReference (String identifier);
    }

    public static final class BluetoothDeviceCollection implements DeviceCollection {
      private final Collection<BluetoothDevice> devices;

      public BluetoothDeviceCollection (Context context) {
        devices = BluetoothAdapter.getDefaultAdapter().getBondedDevices();
      }

      @Override
      public String[] getIdentifierValues () {
        StringMaker<BluetoothDevice> stringMaker = new StringMaker<BluetoothDevice>() {
          @Override
          public String makeString (BluetoothDevice device) {
            return device.getAddress();
          }
        };

        return makeStringArray(devices, stringMaker);
      }

      @Override
      public String[] getIdentifierLabels () {
        StringMaker<BluetoothDevice> stringMaker = new StringMaker<BluetoothDevice>() {
          @Override
          public String makeString (BluetoothDevice device) {
            return device.getName();
          }
        };

        return makeStringArray(devices, stringMaker);
      }

      public String getMethodQualifier () {
        return "bluetooth";
      }

      public String getDeviceReference (String identifier) {
        return identifier;
      }
    }

    public static final class UsbDeviceCollection implements DeviceCollection {
      private final UsbManager manager;
      private final Map<String, UsbDevice> map;
      private final Collection<UsbDevice> devices;

      public UsbDeviceCollection (Context context) {
        manager = (UsbManager)context.getSystemService(Context.USB_SERVICE);
        map = manager.getDeviceList();
        devices = map.values();
      }

      @Override
      public String[] getIdentifierValues () {
        StringMaker<UsbDevice> stringMaker = new StringMaker<UsbDevice>() {
          @Override
          public String makeString (UsbDevice device) {
            return device.getDeviceName();
          }
        };

        return makeStringArray(devices, stringMaker);
      }

      @Override
      public String[] getIdentifierLabels () {
        StringMaker<UsbDevice> stringMaker = new StringMaker<UsbDevice>() {
          @Override
          public String makeString (UsbDevice device) {
            return String.format("%04X:%04X", device.getVendorId(), device.getProductId());
          }
        };

        return makeStringArray(devices, stringMaker);
      }

      public String getMethodQualifier () {
        return "usb";
      }

      public String getDeviceReference (String identifier) {
        UsbDevice device = map.get(identifier);
        UsbDeviceConnection connection = manager.openDevice(device);

        if (connection != null) {
          String serialNumber = connection.getSerial();
          connection.close();
          return serialNumber;
        }

        return null;
      }
    }

    public static final class SerialDeviceCollection implements DeviceCollection {
      @Override
      public String[] getIdentifierValues () {
        return new String[0];
      }

      @Override
      public String[] getIdentifierLabels () {
        return new String[0];
      }

      public String getMethodQualifier () {
        return "serial";
      }

      public String getDeviceReference (String identifier) {
        return identifier;
      }
    }

    private DeviceCollection makeDeviceCollection (String deviceMethod) {
      String className = getClass().getName() + "$" + deviceMethod + "DeviceCollection";

      String[] argumentTypes = new String[] {
        "android.content.Context"
      };

      Object[] arguments = new Object[] {
        getActivity()
      };

      return (DeviceCollection)ReflectionHelper.newInstance(className, argumentTypes, arguments);
    }

    private void updateDeviceIdentifierList (String deviceMethod) {
      deviceCollection = makeDeviceCollection(deviceMethod);

      setListElements(
        deviceIdentifierList,
        deviceCollection.getIdentifierValues(), 
        deviceCollection.getIdentifierLabels()
      );

      {
        boolean haveIdentifiers = deviceIdentifierList.getEntryValues().length > 0;
        deviceIdentifierList.setEnabled(haveIdentifiers);

        if (haveIdentifiers) {
          resetList(deviceIdentifierList);
        } else {
          deviceIdentifierList.setSummary("no devices");
        }
      }

      resetList(deviceDriverList);
    }

    private void updateDeviceName (String name) {
      String problem;

      if (!deviceMethodList.isEnabled()) {
        problem = "communication method not selected";
      } else if (!deviceIdentifierList.isEnabled()) {
        problem = "device not selected";
      } else if (!deviceDriverList.isEnabled()) {
        problem = "braille driver not selected";
      } else {
        if (name.length() == 0) {
          name = deviceDriverList.getSummary()
               + " " + deviceMethodList.getSummary()
               + " " + deviceIdentifierList.getSummary()
               ;
        }

        if (deviceNames.contains(name)) {
          problem = "device name already in use";
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

      addPreferencesFromResource(R.xml.settings_device);

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

      {
        SharedPreferences prefs = getSharedPreferences();
        deviceNames = new TreeSet<String>(prefs.getStringSet(PREF_KEY_DEVICE_NAMES, Collections.EMPTY_SET));
      }

      updateSelectedDeviceList();
      showListSelection(deviceMethodList);
      updateDeviceIdentifierList(getDeviceMethod());
      updateDeviceName();

      selectedDeviceList.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            String newName = (String)newValue;
            selectedDeviceList.setSummary(newName);
            updateRemoveDeviceScreen(newName);
            return true;
          }
        }
      );

      deviceNameEditor.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            updateDeviceName((String)newValue);
            return true;
          }
        }
      );

      deviceMethodList.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            String newMethod = (String)newValue;
            showListSelection(deviceMethodList, newMethod);
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
            showListSelection(deviceIdentifierList, (String)newValue);
            updateDeviceName();
            return true;
          }
        }
      );

      deviceDriverList.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            showListSelection(deviceDriverList, (String)newValue);
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
              properties.put(PREF_KEY_DEVICE_REFERENCE, deviceCollection.getDeviceReference(deviceIdentifierList.getValue()));
              properties.put(PREF_KEY_DEVICE_DRIVER, deviceDriverList.getValue());
              putProperties(name, properties, editor);

              editor.putStringSet(PREF_KEY_DEVICE_NAMES, deviceNames);
              editor.commit();
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
              deviceNames.remove(name);
              selectedDeviceList.setValue("");
              updateSelectedDeviceList();
              updateDeviceName();

              {
                SharedPreferences.Editor editor = preference.getEditor();
                editor.putStringSet(PREF_KEY_DEVICE_NAMES, deviceNames);
                removeProperties(name, devicePropertyKeys, editor);
                editor.commit();
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

  public static final class AdvancedSettings extends SettingsFragment {
    protected ListPreference attributesTableList;
    protected ListPreference logLevelList;
    protected MultiSelectListPreference logCategorySet;

    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      addPreferencesFromResource(R.xml.settings_advanced);

      attributesTableList = getListPreference(R.string.PREF_KEY_ATTRIBUTES_TABLE);
      logLevelList = getListPreference(R.string.PREF_KEY_LOG_LEVEL);
      logCategorySet = getMultiSelectListPreference(R.string.PREF_KEY_LOG_CATEGORIES);

      showListSelection(attributesTableList);
      showListSelection(logLevelList);
      showSetSelections(logCategorySet);

      attributesTableList.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            showListSelection(attributesTableList, (String)newValue);
            return true;
          }
        }
      );

      logLevelList.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            showListSelection(logLevelList, (String)newValue);
            return true;
          }
        }
      );

      logCategorySet.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            Set<String> newValues = (Set<String>)newValue;
            showSetSelections(logCategorySet, newValues);
            return true;
          }
        }
      );
    }
  }
}

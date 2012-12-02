/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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
import java.util.Set;
import java.util.TreeSet;

import android.util.Log;
import android.os.Bundle;

import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;

import android.preference.PreferenceScreen;
import android.preference.Preference;
import android.preference.EditTextPreference;
import android.preference.ListPreference;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

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

    protected PreferenceScreen getScreenPreference (int key) {
      return (PreferenceScreen)getPreference(key);
    }

    protected EditTextPreference getEditorPreference (int key) {
      return (EditTextPreference)getPreference(key);
    }

    protected ListPreference getListPreference (int key) {
      return (ListPreference)getPreference(key);
    }

    protected void showListSelection (ListPreference list) {
      list.setSummary(list.getEntry());
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

    protected SharedPreferences getSharedPreferences () {
      return getPreferenceManager().getDefaultSharedPreferences(getActivity());
    }

    protected String makeListSelectionKey (ListPreference list, String owner) {
      return list.getKey() + "-" + owner;
    }

    protected void putListSelection (SharedPreferences.Editor editor, ListPreference list, String owner) {
      editor.putString(makeListSelectionKey(list, owner), list.getValue());
    }
  }

  public static final class GeneralSettings extends SettingsFragment {
    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      addPreferencesFromResource(R.xml.settings_general);
    }
  }

  public static final class DeviceManager extends SettingsFragment {
    protected Set<String> deviceNames;

    protected ListPreference changeDeviceList;
    protected PreferenceScreen addDeviceScreen;
    protected Preference removeDeviceButton;

    protected EditTextPreference deviceNameEditor;
    protected ListPreference deviceMethodList;
    protected ListPreference deviceIdentifierList;
    protected ListPreference deviceDriverList;
    protected Preference addDeviceButton;

    protected void setListElements (ListPreference list, String[] values, String[] labels) {
      list.setEntryValues(values);
      list.setEntries(labels);
    }

    protected void setListElements (ListPreference list, String[] values) {
      setListElements(list, values, values);
    }

    protected void updateRemoveDeviceButton () {
      boolean on = false;

      if (changeDeviceList.isEnabled()) {
        CharSequence value = changeDeviceList.getSummary();

        if (value != null) {
          if (value.length() > 0) {
            on = true;
          }
        }
      }

      removeDeviceButton.setSelectable(on);
    }

    private void updateChangeDeviceList () {
      boolean haveDevices = !deviceNames.isEmpty();
      changeDeviceList.setEnabled(haveDevices);

      if (haveDevices) {
        {
          String[] names = new String[deviceNames.size()];
          deviceNames.toArray(names);
          setListElements(changeDeviceList, names);
        }

        changeDeviceList.setSummary(changeDeviceList.getEntry());
      } else {
        changeDeviceList.setSummary("no devices");
      }

      updateRemoveDeviceButton();
    }

    private String getDeviceMethod () {
      return deviceMethodList.getValue();
    }

    public interface DeviceCollection {
      public String[] getIdentifierValues ();
      public String[] getIdentifierLabels ();
    }

    public static final class BluetoothDeviceCollection implements DeviceCollection {
      private final Collection<BluetoothDevice> devices;

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

      public BluetoothDeviceCollection (Context context) {
        devices = BluetoothAdapter.getDefaultAdapter().getBondedDevices();
      }
    }

    public static final class UsbDeviceCollection implements DeviceCollection {
      private final Collection<UsbDevice> devices;

      protected UsbManager getManager (Context context) {
        return (UsbManager)context.getSystemService(Context.USB_SERVICE);
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

      public UsbDeviceCollection (Context context) {
        devices = getManager(context).getDeviceList().values();
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

    protected void resetDeviceDriverList () {
      deviceDriverList.setValueIndex(0);
      showListSelection(deviceDriverList);
    }

    private void updateDeviceIdentifiers (String deviceMethod) {
      DeviceCollection devices = makeDeviceCollection(deviceMethod);

      setListElements(
        deviceIdentifierList,
        devices.getIdentifierValues(), 
        devices.getIdentifierLabels()
      );

      {
        int count = deviceIdentifierList.getEntryValues().length;
        deviceIdentifierList.setEnabled(count > 0);

        if (count == 0) {
          deviceIdentifierList.setSummary("no devices");
        } else {
          deviceIdentifierList.setValueIndex(0);
          showListSelection(deviceIdentifierList);
        }
      }

      resetDeviceDriverList();
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

      changeDeviceList = getListPreference(R.string.PREF_KEY_CHANGE_DEVICE);
      addDeviceScreen = getScreenPreference(R.string.PREF_KEY_ADD_DEVICE);
      removeDeviceButton = getPreference(R.string.PREF_KEY_REMOVE_DEVICE);

      deviceNameEditor = getEditorPreference(R.string.PREF_KEY_DEVICE_NAME);
      deviceMethodList = getListPreference(R.string.PREF_KEY_DEVICE_METHOD);
      deviceIdentifierList = getListPreference(R.string.PREF_KEY_DEVICE_IDENTIFIER);
      deviceDriverList = getListPreference(R.string.PREF_KEY_DEVICE_DRIVER);
      addDeviceButton = getPreference(R.string.PREF_KEY_DEVICE_ADD);

      {
        SharedPreferences prefs = getSharedPreferences();
        deviceNames = new TreeSet<String>(prefs.getStringSet("device-names", Collections.EMPTY_SET));
      }

      updateChangeDeviceList();
      showListSelection(deviceMethodList);
      updateDeviceIdentifiers(getDeviceMethod());
      updateDeviceName();

      changeDeviceList.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            changeDeviceList.setSummary((String)newValue);
            updateRemoveDeviceButton();
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
            updateDeviceIdentifiers(newMethod);
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
            updateChangeDeviceList();
            updateDeviceName();

            {
              SharedPreferences.Editor editor = preference.getEditor();
              putListSelection(editor, deviceMethodList, name);
              putListSelection(editor, deviceIdentifierList, name);
              putListSelection(editor, deviceDriverList, name);
              editor.putStringSet("device-names", deviceNames);
              editor.commit();
            }

            addDeviceScreen.getDialog().dismiss();
            return true;
          }
        }
      );

      removeDeviceButton.setOnPreferenceClickListener(
        new Preference.OnPreferenceClickListener() {
          @Override
          public boolean onPreferenceClick (Preference preference) {
            deviceNames.remove(changeDeviceList.getValue());
            changeDeviceList.setValue("");
            updateChangeDeviceList();
            return true;
          }
        }
      );
    }
  }

  public static final class AdvancedSettings extends SettingsFragment {
    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      addPreferencesFromResource(R.xml.settings_advanced);
    }
  }
}

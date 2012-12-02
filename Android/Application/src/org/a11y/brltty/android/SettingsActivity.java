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

    protected PreferenceScreen findScreen (String key) {
      return (PreferenceScreen)findPreference(key);
    }

    protected EditTextPreference findEditor (String key) {
      return (EditTextPreference)findPreference(key);
    }

    protected ListPreference findList (String key) {
      return (ListPreference)findPreference(key);
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

    protected ListPreference deviceNameList;
    protected PreferenceScreen newDeviceScreen;
    protected Preference removeDeviceButton;

    protected EditTextPreference deviceNameEditor;
    protected ListPreference communicationMethodList;
    protected ListPreference deviceIdentifierList;
    protected ListPreference brailleDriverList;
    protected Preference addDeviceButton;

    protected void setListElements (ListPreference list, String[] values, String[] labels) {
      list.setEntryValues(values);
      list.setEntries(labels);
    }

    protected void setListElements (ListPreference list, String[] values) {
      setListElements(list, values, values);
    }

    private void updateRemoveDeviceButton () {
      removeDeviceButton.setSelectable(deviceNameList.isEnabled());
    }

    private void updateDeviceNameList () {
      int count = deviceNames.size();

      if (count == 0) {
        deviceNameList.setEnabled(false);
        deviceNameList.setSummary("no devices");
      } else {
        {
          String[] names = new String[count];
          deviceNames.toArray(names);
          setListElements(deviceNameList, names);
        }

        deviceNameList.setEnabled(true);
        deviceNameList.setSummary(deviceNameList.getEntry());
      }

      updateRemoveDeviceButton();
    }

    private String getCommunicationMethod () {
      return communicationMethodList.getValue();
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

    private DeviceCollection makeDeviceCollection (String communicationMethod) {
      String className = getClass().getName() + "$" + communicationMethod + "DeviceCollection";

      String[] argumentTypes = new String[] {
        "android.content.Context"
      };

      Object[] arguments = new Object[] {
        getActivity()
      };

      return (DeviceCollection)ReflectionHelper.newInstance(className, argumentTypes, arguments);
    }

    protected void resetBrailleDriverList () {
      brailleDriverList.setValueIndex(0);
      showListSelection(brailleDriverList);
    }

    private void updateDeviceIdentifiers (String communicationMethod) {
      DeviceCollection devices = makeDeviceCollection(communicationMethod);

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

      resetBrailleDriverList();
    }

    private void updateDeviceName (String name) {
      String problem;

      if (!communicationMethodList.isEnabled()) {
        problem = "communication method not selected";
      } else if (!deviceIdentifierList.isEnabled()) {
        problem = "device not selected";
      } else if (!brailleDriverList.isEnabled()) {
        problem = "braille driver not selected";
      } else {
        if (name.length() == 0) {
          name = brailleDriverList.getSummary()
               + " " + communicationMethodList.getSummary()
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

      deviceNameList = findList("current-device");
      newDeviceScreen = findScreen("new-device");
      removeDeviceButton = findPreference("device-remove");

      deviceNameEditor = findEditor("device-name");
      communicationMethodList = findList("device-method");
      deviceIdentifierList = findList("device-identifier");
      brailleDriverList = findList("device-driver");
      addDeviceButton = findPreference("device-add");

      {
        SharedPreferences prefs = getSharedPreferences();
        deviceNames = new TreeSet<String>(prefs.getStringSet("device-names", Collections.EMPTY_SET));
      }

      updateDeviceNameList();
      showListSelection(communicationMethodList);
      updateDeviceIdentifiers(getCommunicationMethod());
      updateDeviceName();

      deviceNameList.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            deviceNameList.setSummary((String)newValue);
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

      communicationMethodList.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            String newMethod = (String)newValue;
            showListSelection(communicationMethodList, newMethod);
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

      brailleDriverList.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            showListSelection(brailleDriverList, (String)newValue);
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
            updateDeviceNameList();
            updateDeviceName();

            {
              SharedPreferences.Editor editor = preference.getEditor();
              putListSelection(editor, communicationMethodList, name);
              putListSelection(editor, deviceIdentifierList, name);
              putListSelection(editor, brailleDriverList, name);
              editor.putStringSet("device-names", deviceNames);
              editor.commit();
            }

            newDeviceScreen.getDialog().dismiss();
            return true;
          }
        }
      );

      removeDeviceButton.setOnPreferenceClickListener(
        new Preference.OnPreferenceClickListener() {
          @Override
          public boolean onPreferenceClick (Preference preference) {
            deviceNames.remove(deviceNameList.getValue());
            deviceNameList.setValue("");
            updateDeviceNameList();
            updateRemoveDeviceButton();
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

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

import java.util.Collections;
import java.util.Collection;
import java.util.List;
import java.util.ArrayList;

import android.util.Log;
import android.os.Bundle;

import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;

import android.preference.PreferenceScreen;
import android.preference.Preference;
import android.preference.ListPreference;

import android.content.Context;
import android.content.Intent;

import android.bluetooth.*;
import android.hardware.usb.*;

public class SettingsActivity extends PreferenceActivity {
  private static final String LOG_TAG = SettingsActivity.class.getName();

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

    protected ListPreference findList (String key) {
      return (ListPreference)findPreference(key);
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
    PreferenceScreen newDeviceScreen;
    ListPreference communicationMethodList;
    ListPreference deviceIdentifierList;
    ListPreference brailleDriverList;
    Preference addDeviceButton;

    interface StringArrayElementMaker<T> {
      String makeStringArrayElement (T object);
    }

    private static <T> String[] makeStringArray (Collection<T> collection, StringArrayElementMaker<T> stringArrayElementMaker) {
      List<String> strings = new ArrayList<String>(collection.size());

      for (T element : collection) {
        strings.add(stringArrayElementMaker.makeStringArrayElement(element));
      }

      return strings.toArray(new String[strings.size()]);
    }

    public interface DeviceCollection {
      public String[] getIdentifierValues ();
      public String[] getIdentifierLabels ();
    }

    public static final class BluetoothDeviceCollection implements DeviceCollection {
      private final Collection<BluetoothDevice> collection;

      @Override
      public String[] getIdentifierValues () {
        StringArrayElementMaker<BluetoothDevice> stringArrayElementMaker = new StringArrayElementMaker<BluetoothDevice>() {
          @Override
          public String makeStringArrayElement (BluetoothDevice device) {
            return device.getAddress();
          }
        };

        return makeStringArray(collection, stringArrayElementMaker);
      }

      @Override
      public String[] getIdentifierLabels () {
        StringArrayElementMaker<BluetoothDevice> stringArrayElementMaker = new StringArrayElementMaker<BluetoothDevice>() {
          @Override
          public String makeStringArrayElement (BluetoothDevice device) {
            return device.getName();
          }
        };

        return makeStringArray(collection, stringArrayElementMaker);
      }

      public BluetoothDeviceCollection (Context context) {
        collection = BluetoothAdapter.getDefaultAdapter().getBondedDevices();
      }
    }

    public static final class UsbDeviceCollection implements DeviceCollection {
      private final Collection<UsbDevice> collection;

      protected UsbManager getManager (Context context) {
        return (UsbManager)context.getSystemService(Context.USB_SERVICE);
      }

      @Override
      public String[] getIdentifierValues () {
        StringArrayElementMaker<UsbDevice> stringArrayElementMaker = new StringArrayElementMaker<UsbDevice>() {
          @Override
          public String makeStringArrayElement (UsbDevice device) {
            return device.getDeviceName();
          }
        };

        return makeStringArray(collection, stringArrayElementMaker);
      }

      @Override
      public String[] getIdentifierLabels () {
        StringArrayElementMaker<UsbDevice> stringArrayElementMaker = new StringArrayElementMaker<UsbDevice>() {
          @Override
          public String makeStringArrayElement (UsbDevice device) {
            return device.getDeviceName();
          }
        };

        return makeStringArray(collection, stringArrayElementMaker);
      }

      public UsbDeviceCollection (Context context) {
        collection = getManager(context).getDeviceList().values();
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

    private DeviceCollection getDeviceCollection (String communicationMethod) {
      String className = getClass().getName() + "$" + communicationMethod + "DeviceCollection";

      String[] argumentTypes = new String[] {
        "android.content.Context"
      };

      Object[] arguments = new Object[] {
        getActivity()
      };

      return (DeviceCollection)ReflectionHelper.newInstance(className, argumentTypes, arguments);
    }

    private String getCommunicationMethod () {
      return communicationMethodList.getValue();
    }

    private void refreshDeviceIdentifiers (String communicationMethod) {
      DeviceCollection devices = getDeviceCollection(communicationMethod);
      ListPreference list = deviceIdentifierList;

      list.setEntryValues(devices.getIdentifierValues());
      list.setEntries(devices.getIdentifierLabels());
    }

    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      addPreferencesFromResource(R.xml.settings_device);

      newDeviceScreen = findScreen("new-device");
      communicationMethodList = findList("device-communication");
      deviceIdentifierList = findList("device-identifier");
      brailleDriverList = findList("device-driver");
      addDeviceButton = findPreference("device-add");

      communicationMethodList.setOnPreferenceChangeListener(
        new Preference.OnPreferenceChangeListener() {
          @Override
          public boolean onPreferenceChange (Preference preference, Object newValue) {
            refreshDeviceIdentifiers((String)newValue);
            return true;
          }
        }
      );

      addDeviceButton.setOnPreferenceClickListener(
        new Preference.OnPreferenceClickListener() {
          @Override
          public boolean onPreferenceClick (Preference preference) {
            newDeviceScreen.getDialog().dismiss();
            return true;
          }
        }
      );
    }

    @Override
    public void onResume () {
      super.onResume();

      refreshDeviceIdentifiers(getCommunicationMethod());
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

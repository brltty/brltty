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

import java.lang.reflect.*;

import java.util.Collections;
import java.util.Collection;
import java.util.List;
import java.util.ArrayList;

import android.util.Log;

import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.os.Bundle;

import android.preference.PreferenceScreen;
import android.preference.Preference;
import android.preference.ListPreference;

import android.content.Context;
import android.content.Intent;

import android.bluetooth.*;
import android.hardware.usb.*;

public class SettingsActivity extends PreferenceActivity {
  private final String LOG_TAG = this.getClass().getName();

  @Override
  protected void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
  }

  @Override
  public void onBuildHeaders (List<Header> target) {
    loadHeadersFromResource(R.xml.settings_headers, target);
  }

  public static abstract class SettingsFragment extends PreferenceFragment {
    protected final String LOG_TAG = this.getClass().getName();

    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);
    }

    protected ListPreference findListPreference (String key) {
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
    ListPreference deviceCommunicationPreference;
    ListPreference deviceNamePreference;

    interface GetString<T> {
      String getString (T object);
    }

    private static <T> String[] makeStringArray (Collection<T> collection, GetString<T> getString) {
      List<String> strings = new ArrayList<String>(collection.size());

      for (T element : collection) {
        strings.add(getString.getString(element));
      }

      return strings.toArray(new String[strings.size()]);
    }

    public interface DeviceCollection {
      public String[] getNameValues ();
      public String[] getNameLabels ();
    }

    public static final class BluetoothDeviceCollection implements DeviceCollection {
      private final Collection<BluetoothDevice> collection;

      @Override
      public String[] getNameValues () {
        GetString<BluetoothDevice> getString = new GetString<BluetoothDevice>() {
          @Override
          public String getString (BluetoothDevice device) {
            return device.getAddress();
          }
        };

        return makeStringArray(collection, getString);
      }

      @Override
      public String[] getNameLabels () {
        GetString<BluetoothDevice> getString = new GetString<BluetoothDevice>() {
          @Override
          public String getString (BluetoothDevice device) {
            return device.getName();
          }
        };

        return makeStringArray(collection, getString);
      }

      public BluetoothDeviceCollection (Context context) {
        collection = BluetoothAdapter.getDefaultAdapter().getBondedDevices();
      }
    }

    public static final class UsbDeviceCollection implements DeviceCollection {
      private final Collection<UsbDevice> collection;

      @Override
      public String[] getNameValues () {
        GetString<UsbDevice> getString = new GetString<UsbDevice>() {
          @Override
          public String getString (UsbDevice device) {
            return device.getDeviceName();
          }
        };

        return makeStringArray(collection, getString);
      }

      @Override
      public String[] getNameLabels () {
        GetString<UsbDevice> getString = new GetString<UsbDevice>() {
          @Override
          public String getString (UsbDevice device) {
            return device.getDeviceName();
          }
        };

        return makeStringArray(collection, getString);
      }

      public UsbDeviceCollection (Context context) {
        UsbManager manager = (UsbManager)context.getSystemService(Context.USB_SERVICE);
        collection = manager.getDeviceList().values();
      }
    }

    public static final class SerialDeviceCollection implements DeviceCollection {
      @Override
      public String[] getNameValues () {
        return new String[0];
      }

      @Override
      public String[] getNameLabels () {
        return new String[0];
      }
    }

    private DeviceCollection getDeviceCollection (String communicationMethod) {
      String className = getClass().getName() + "$" + communicationMethod + "DeviceCollection";

      Constructor constructor;
      try {
        constructor = Class.forName(className).getConstructor(
          Class.forName("android.content.Context")
        );
      } catch (ClassNotFoundException exception) {
        Log.w(LOG_TAG, "class not found: " + className);
        return null;
      } catch (NoSuchMethodException exception) {
        Log.w(LOG_TAG, "constructor not found: " + className);
        return null;
      }

      DeviceCollection collection;
      try {
        collection = (DeviceCollection)constructor.newInstance(new Object[] {
          getActivity()
        });
      } catch (java.lang.InstantiationException exception) {
        Log.w(LOG_TAG, "uninstantiatable class: " + className);
        return null;
      } catch (IllegalAccessException exception) {
        Log.w(LOG_TAG, "inaccessible constructor: " + className);
        return null;
      } catch (InvocationTargetException exception) {
        Log.w(LOG_TAG, "construction failed: " + className);
        return null;
      }

      return collection;
    }

    private ListPreference getDeviceList () {
      return findListPreference("device-name");
    }

    private String getCommunicationMethod () {
      return deviceCommunicationPreference.getValue();
    }

    private void refreshDeviceNames () {
      DeviceCollection devices = getDeviceCollection(getCommunicationMethod());
      ListPreference list = deviceNamePreference;

      list.setEntryValues(devices.getNameValues());
      list.setEntries(devices.getNameLabels());
    }

    private void setCommunicatinMethodChangeListener () {
      Preference.OnPreferenceChangeListener listener = new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange (Preference preference, Object newValue) {
          refreshDeviceNames();
          return true;
        }
      };

      deviceCommunicationPreference.setOnPreferenceChangeListener(listener);
    }

    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      addPreferencesFromResource(R.xml.settings_device);
      deviceCommunicationPreference = findListPreference("device-communication");
      deviceNamePreference = findListPreference("device-name");

      setCommunicatinMethodChangeListener();
    }

    @Override
    public void onResume () {
      super.onResume();
      refreshDeviceNames();
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

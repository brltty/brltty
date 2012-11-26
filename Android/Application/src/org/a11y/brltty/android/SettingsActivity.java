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
    private Preference addDevicePreference;

    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      addPreferencesFromResource(R.xml.settings_device);
      addDevicePreference = findPreference("add-device");
    }

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
      public String[] getValues ();
      public String[] getLabels ();
    }

    public static final class BluetoothDeviceCollection implements DeviceCollection {
      private final Collection<BluetoothDevice> collection;

      @Override
      public String[] getValues () {
        GetString<BluetoothDevice> getString = new GetString<BluetoothDevice>() {
          @Override
          public String getString (BluetoothDevice device) {
            return device.getAddress();
          }
        };

        return makeStringArray(collection, getString);
      }

      @Override
      public String[] getLabels () {
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
      public String[] getValues () {
        GetString<UsbDevice> getString = new GetString<UsbDevice>() {
          @Override
          public String getString (UsbDevice device) {
            return device.getDeviceName();
          }
        };

        return makeStringArray(collection, getString);
      }

      @Override
      public String[] getLabels () {
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
      public String[] getValues () {
        return new String[0];
      }

      @Override
      public String[] getLabels () {
        return new String[0];
      }
    }

    private DeviceCollection getDeviceCollection (String deviceType) {
      String className = getClass().getName() + "$" + deviceType + "DeviceCollection";

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
      return findListPreference("device-list");
    }

    private String getDeviceType () {
      return findListPreference("device-type").getValue();
    }

    private void refreshDeviceList () {
      DeviceCollection devices = getDeviceCollection(getDeviceType());
      ListPreference list = getDeviceList();
      list.setEntryValues(devices.getValues());
      list.setEntries(devices.getLabels());
    }

    @Override
    public boolean onPreferenceTreeClick (PreferenceScreen screen, Preference preference) {
      if (preference == addDevicePreference) {
        refreshDeviceList();
      }

      return super.onPreferenceTreeClick(screen, preference);
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

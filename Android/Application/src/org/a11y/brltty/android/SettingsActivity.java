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
    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      addPreferencesFromResource(R.xml.settings_device);
    }
  }

  public static final class DeviceFinder extends SettingsFragment {
    private final String LOG_TAG = this.getClass().getName();

    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);
    }

    public interface DeviceCollection {
      public String[] getValues ();
      public String[] getLabels ();
    }

    public static final class BluetoothDeviceCollection implements DeviceCollection {
      private final Collection<BluetoothDevice> collection;

      @Override
      public String[] getValues () {
        List<String> values = new ArrayList<String>(collection.size());

        for (BluetoothDevice device : collection) {
          values.add(device.getAddress());
        }

        return (String[])values.toArray();
      }

      @Override
      public String[] getLabels () {
        List<String> labels = new ArrayList<String>(collection.size());

        for (BluetoothDevice device : collection) {
          labels.add(device.getName());
        }

        return (String[])labels.toArray();
      }

      public BluetoothDeviceCollection (Context context) {
        collection = BluetoothAdapter.getDefaultAdapter().getBondedDevices();
      }
    }

    public static final class UsbDeviceCollection implements DeviceCollection {
      private final Collection<UsbDevice> collection;

      @Override
      public String[] getValues () {
        return new String[0];
      }

      @Override
      public String[] getLabels () {
        return new String[0];
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

    private DeviceCollection getDevices () {
      ListPreference preference = findListPreference("device-type");
      String type = preference.getValue();
      String name = getClass().getName() + "$" + type + "DeviceCollection";

      try {
        Constructor constructor = Class.forName(name).getConstructor(
          Class.forName("android.content.Context")
        );

        DeviceCollection devices = (DeviceCollection)constructor.newInstance(new Object[] {
          getActivity()
        });

        return devices;
      } catch (Exception exception) {
        Log.w(LOG_TAG, exception.getMessage());
      }

      return null;
    }

    private void refreshDeviceList () {
      DeviceCollection devices = getDevices();
      ListPreference preference = findListPreference("device-list");
      preference.setEntryValues(devices.getValues());
      preference.setEntries(devices.getLabels());
    }

    @Override
    public void onResume () {
      super.onResume();

      refreshDeviceList();
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

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

import java.util.List;

import android.util.Log;

import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.os.Bundle;

import android.preference.Preference;
import android.preference.ListPreference;

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

    protected PreferenceActivity getPreferenceActivity () {
      return (PreferenceActivity)getActivity();
    }

    protected Preference getPreference (String key) {
      return getPreferenceActivity().findPreference(key);
    }

    protected ListPreference getListPreference (String key) {
      return (ListPreference)getPreference(key);
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
    @Override
    public void onCreate (Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);
    }

    public static abstract class DeviceList {
      private final List deviceList;

      protected abstract List makeDeviceList ();

      protected DeviceList () {
        deviceList = makeDeviceList();
      }
    }

    public static class BluetoothDeviceList extends DeviceList {
      @Override
      protected List makeDeviceList () {
        return null;
      }
    }

    public static class UsbDeviceList extends DeviceList {
      @Override
      protected List makeDeviceList () {
        return null;
      }
    }

    public static class SerialDeviceList extends DeviceList {
      @Override
      protected List makeDeviceList () {
        return null;
      }
    }

    private DeviceList deviceList = null;

    private void refreshDeviceList () {
      ListPreference preference = getListPreference("device-type");
      Log.i("REFDEV", "pref: " + preference);
      String value = preference.getValue();
      Log.i("REFDEV", "val: " + value);
      String className = getClass() + "." + value + "DeviceList";
    }

    @Override
    public void onResume () {
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

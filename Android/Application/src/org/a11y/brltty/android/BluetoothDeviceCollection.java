/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2019 by The BRLTTY Developers.
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

package org.a11y.brltty.android;

import java.util.Collections;
import java.util.Collection;
import java.util.Map;

import android.content.Context;
import android.bluetooth.*;

public final class BluetoothDeviceCollection extends DeviceCollection {
  public static final String DEVICE_QUALIFIER = "bluetooth";

  @Override
  public final String getQualifier () {
    return DEVICE_QUALIFIER;
  }

  private final Collection<BluetoothDevice> devices;

  public BluetoothDeviceCollection (Context context) {
    super();
    BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
    devices = (adapter != null)? adapter.getBondedDevices(): Collections.EMPTY_SET;
  }

  @Override
  public final String[] makeValues () {
    StringMaker<BluetoothDevice> stringMaker = new StringMaker<BluetoothDevice>() {
      @Override
      public String makeString (BluetoothDevice device) {
        return device.getAddress();
      }
    };

    return makeStringArray(devices, stringMaker);
  }

  @Override
  public final String[] makeLabels () {
    StringMaker<BluetoothDevice> stringMaker = new StringMaker<BluetoothDevice>() {
      @Override
      public String makeString (BluetoothDevice device) {
        return device.getName();
      }
    };

    return makeStringArray(devices, stringMaker);
  }

  @Override
  protected final void putParameters (Map<String, String> parameters, String value) {
    parameters.put("address", value);
  }
}

/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brltty.android;

import java.util.Collection;
import java.util.Map;

import android.content.Context;
import android.hardware.usb.*;

public final class UsbDeviceCollection extends DeviceCollection {
  public static final String DEVICE_QUALIFIER = "usb";

  @Override
  public String getQualifier () {
    return DEVICE_QUALIFIER;
  }

  private final UsbManager manager;
  private final Map<String, UsbDevice> map;
  private final Collection<UsbDevice> devices;

  public UsbDeviceCollection (Context context) {
    super();
    manager = (UsbManager)context.getSystemService(Context.USB_SERVICE);
    map = manager.getDeviceList();
    devices = map.values();
  }

  @Override
  public String[] getValues () {
    StringMaker<UsbDevice> stringMaker = new StringMaker<UsbDevice>() {
      @Override
      public String makeString (UsbDevice device) {
        return device.getDeviceName();
      }
    };

    return makeStringArray(devices, stringMaker);
  }

  @Override
  public String[] getLabels () {
    StringMaker<UsbDevice> stringMaker = new StringMaker<UsbDevice>() {
      @Override
      public String makeString (UsbDevice device) {
        return String.format("%04X:%04X", device.getVendorId(), device.getProductId());
      }
    };

    return makeStringArray(devices, stringMaker);
  }

  @Override
  public String makeDeviceReference (String identifier) {
    UsbDevice device = map.get(identifier);
    UsbDeviceConnection connection = manager.openDevice(device);

    if (connection != null) {
      String serialNumber = connection.getSerial();
      connection.close();

      return String.format("vendorIdentifier=0X%04X+productIdentifier=0X%04X+serialNumber=%s",
                           device.getVendorId(), device.getProductId(), serialNumber);
    }

    return null;
  }
}

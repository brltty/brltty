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

import java.util.Collection;
import java.util.Map;

import android.content.Context;
import android.hardware.usb.*;

public final class UsbDeviceCollection extends DeviceCollection {
  public static final String DEVICE_QUALIFIER = "usb";

  @Override
  public final String getQualifier () {
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

  private final String getManufacturerName (UsbDevice device) {
    if (ApplicationUtilities.haveLollipop) {
      return device.getManufacturerName();
    } else {
      return null;
    }
  }

  private final String getProductName (UsbDevice device) {
    if (ApplicationUtilities.haveLollipop) {
      return device.getProductName();
    } else {
      return null;
    }
  }

  private final String getSerialNumber (UsbDevice device) {
    if (ApplicationUtilities.haveLollipop) {
      return device.getSerialNumber();
    } else {
      UsbDeviceConnection connection = manager.openDevice(device);
      if (connection == null) return null;

      try {
        return connection.getSerial();
      } finally {
        connection.close();
      }
    }
  }

  @Override
  public final String[] makeValues () {
    StringMaker<UsbDevice> stringMaker = new StringMaker<UsbDevice>() {
      @Override
      public String makeString (UsbDevice device) {
        return device.getDeviceName();
      }
    };

    return makeStringArray(devices, stringMaker);
  }

  @Override
  public final String[] makeLabels () {
    StringMaker<UsbDevice> stringMaker = new StringMaker<UsbDevice>() {
      @Override
      public String makeString (UsbDevice device) {
        StringBuilder label = new StringBuilder();

        {
          boolean first = true;

          String[] components = new String[] {
            getManufacturerName(device),
            getProductName(device),
            getSerialNumber(device)
          };

          for (String component : components) {
            if (component == null) continue;
            component = component.trim();
            if (component.isEmpty()) continue;

            if (first) {
              first = false;
            } else {
              label.append(',');
            }

            if (label.length() > 0) label.append(' ');
            label.append(component);
          }
        }

        if (label.length() == 0) {
          label.append(
            String.format(
              "[%04X:%04X]",
              device.getVendorId(),
              device.getProductId()
            )
          );
        }

        return label.toString();
      }
    };

    return makeStringArray(devices, stringMaker);
  }

  @Override
  protected final void putParameters (Map<String, String> parameters, String value) {
    UsbDevice device = map.get(value);
    parameters.put("vendorIdentifier", Integer.toString(device.getVendorId()));
    parameters.put("productIdentifier", Integer.toString(device.getProductId()));
    parameters.put("serialNumber", getSerialNumber(device));
  }
}

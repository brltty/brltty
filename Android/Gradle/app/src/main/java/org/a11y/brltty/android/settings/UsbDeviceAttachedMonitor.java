/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
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

package org.a11y.brltty.android.settings;
import org.a11y.brltty.android.*;

import android.util.Log;
import android.app.Activity;
import android.os.Bundle;

import android.content.Intent;
import android.hardware.usb.*;
import java.util.Map;
import java.util.LinkedHashMap;

public class UsbDeviceAttachedMonitor extends Activity {
  private final static String LOG_TAG = UsbDeviceAttachedMonitor.class.getName();

  private static void addProperty (Map<String, String> properties, String name, int value) {
    if (value != 0) {
      properties.put(name, String.format("%04X", value));
    }
  }

  private static void addProperty (Map<String, String> properties, String name, String value) {
    if (value != null) {
      if (!value.isEmpty()) {
        char quote = '"';
        properties.put(name, String.format("%c%s%c", quote, value, quote));
      }
    }
  }

  private static Map<String, String> getProperties (UsbDevice device) {
    Map<String, String> properties = new LinkedHashMap<>();

    addProperty(properties, "VID", device.getVendorId());
    addProperty(properties, "PID", device.getProductId());
    addProperty(properties, "Serial", device.getSerialNumber());
    addProperty(properties, "Manufacturer", device.getManufacturerName());
    addProperty(properties, "Product", device.getProductName());

    return properties;
  }

  private static String makeDescription (UsbDevice device) {
    Map<String, String> properties = getProperties(device);
    StringBuilder description = new StringBuilder();

    for (String name : properties.keySet()) {
      String value = properties.get(name);
      if (description.length() > 0) description.append(' ');
      description.append(name).append(':').append(value);
    }

    return description.toString();
  }

  @Override
  protected void onCreate (Bundle savedState) {
    super.onCreate(savedState);

    Intent intent = getIntent();
    UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
    Log.i(LOG_TAG, ("device attached: " + makeDescription(device)));
  }

  @Override
  protected void onResume () {
    super.onResume();
    finish();
  }
}

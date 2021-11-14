/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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

public class UsbDeviceAttachedMonitor extends Activity {
  private final static String LOG_TAG = UsbDeviceAttachedMonitor.class.getName();

  private static void addParameter (Map<String, String> parameters, String name, String value) {
    if ((value != null) && !value.isEmpty()) {
      parameters.put(name, value);
    }
  }

  private static String makeDescription (UsbDevice device) {
    Map<String, String> parameters = DeviceCollection.newParameters();
    UsbDeviceCollection.putParameters(parameters, device);

    addParameter(parameters, "manufacturerName", device.getManufacturerName());
    addParameter(parameters, "productName", device.getProductName());

    return UsbDeviceCollection.makeReference(parameters);
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

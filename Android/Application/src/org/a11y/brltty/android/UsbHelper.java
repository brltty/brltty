/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

import java.nio.ByteBuffer;

import java.util.HashMap;
import java.util.Iterator;
import java.util.concurrent.LinkedBlockingDeque;

import android.util.Log;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;

import android.app.PendingIntent;

import android.hardware.usb.*;

public class UsbHelper {
  private static final String LOG_TAG = UsbHelper.class.getName();

  private static Context usbContext;
  private static UsbManager usbManager;

  private static BroadcastReceiver permissionReceiver;
  private static PendingIntent permissionIntent;

  private static final String ACTION_USB_PERMISSION =
    "org.a11y.brltty.android.USB_PERMISSION";

  private static void makePermissionReceiver () {
    permissionReceiver = new BroadcastReceiver () {
      @Override
      public void onReceive (Context context, Intent intent) {
        String action = intent.getAction();

        if (action.equals(ACTION_USB_PERMISSION)) {
          synchronized (this) {
            UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

            if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
              Log.i(LOG_TAG, "permission granted for USB device: " + device);
            } else {
              Log.w(LOG_TAG, "permission denied for USB device: " + device);
            }

            notify();
          }
        }
      }
    };

    {
      IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
      usbContext.registerReceiver(permissionReceiver, filter);
    }

    permissionIntent = PendingIntent.getBroadcast(usbContext, 0, new Intent(ACTION_USB_PERMISSION), 0);
  }

  public static void begin (Context context) {
    usbContext = context;

    usbManager = (UsbManager)usbContext.getSystemService(Context.USB_SERVICE);
    makePermissionReceiver();
  }

  public static void end () {
    usbContext.unregisterReceiver(permissionReceiver);
  }

  public static Iterator<UsbDevice> getDeviceIterator () {
    HashMap<String, UsbDevice> devices = usbManager.getDeviceList();
    return devices.values().iterator();
  }

  public static UsbDevice getNextDevice (Iterator<UsbDevice> iterator) {
    return iterator.hasNext()? iterator.next(): null;
  }

  public static UsbInterface getDeviceInterface (UsbDevice device, int identifier) {
    int count = device.getInterfaceCount();

    for (int index=0; index<count; index+=1) {
      UsbInterface intf = device.getInterface(index);

      if (identifier == intf.getId()) return intf;
    }

    return null;
  }

  public static UsbEndpoint getInterfaceEndpoint (UsbInterface intf, int address) {
    int count = intf.getEndpointCount();

    for (int index=0; index<count; index+=1) {
      UsbEndpoint endpoint = intf.getEndpoint(index);

      if (address == endpoint.getAddress()) return endpoint;
    }

    return null;
  }

  private static boolean obtainPermission (UsbDevice device) {
    if (usbManager.hasPermission(device)) {
      Log.d(LOG_TAG, "permission already granted for USB device: " + device);
      return true;
    }

    Log.d(LOG_TAG, "requesting permission for USB device: " + device);
    synchronized (permissionReceiver) {
      usbManager.requestPermission(device, permissionIntent);

      try {
        permissionReceiver.wait();
      } catch (InterruptedException exception) {
      }
    }

    return usbManager.hasPermission(device);
  }

  public static UsbDeviceConnection openDeviceConnection (UsbDevice device) {
    if (obtainPermission(device)) {
      return usbManager.openDevice(device);
    }

    return null;
  }

  public static UsbRequest enqueueRequest (UsbDeviceConnection connection, UsbEndpoint endpoint, byte[] bytes) {
    UsbRequest request = new UsbRequest();

    if (request.initialize(connection, endpoint)) {
      ByteBuffer buffer = ByteBuffer.wrap(bytes);
      request.setClientData(bytes);
      if (request.queue(buffer, bytes.length)) return request;

      request.close();
    }

    return null;
  }

  public static void cancelRequest (UsbRequest request) {
    request.cancel();
    request.close();
  }
}

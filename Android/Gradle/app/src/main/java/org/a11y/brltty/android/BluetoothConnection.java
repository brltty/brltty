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

package org.a11y.brltty.android;

import java.lang.reflect.*;
import java.util.Set;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.File;
import java.io.FileOutputStream;

import java.util.UUID;
import android.os.ParcelUuid;

import android.util.Log;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;

public class BluetoothConnection {
  private final static String LOG_TAG = BluetoothConnection.class.getName();

  protected final static UUID SERIAL_PROFILE_UUID = UUID.fromString(
    "00001101-0000-1000-8000-00805F9B34FB"
  );

  protected final static BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
  protected final long remoteAddressValue;
  protected final byte[] remoteAddressBytes = new byte[6];
  protected final String remoteAddressString;

  protected BluetoothSocket bluetoothSocket = null;
  protected InputStream inputStream = null;
  protected OutputStream outputStream = null;
  protected OutputStream pipeStream = null;
  protected InputThread inputThread = null;

  private class InputThread extends Thread {
    volatile boolean stop = false;

    InputThread () {
      super(("bluetooth-input-" + remoteAddressString));
    }

    @Override
    public void run () {
      try {
        byte[] buffer = new byte[0X80];

        while (!stop) {
          int byteCount;

          try {
            byteCount = inputStream.read(buffer);
          } catch (IOException exception) {
            Log.d(LOG_TAG, ("Bluetooth input exception: " + remoteAddressString + ": " + exception.getMessage()));
            break;
          }

          if (byteCount > 0) {
            pipeStream.write(buffer, 0, byteCount);
            pipeStream.flush();
          } else if (byteCount < 0) {
            Log.d(LOG_TAG, ("Bluetooth input end: " + remoteAddressString));
            break;
          }
        }
      } catch (Throwable cause) {
        Log.e(LOG_TAG, ("Bluetooth input failed: " + remoteAddressString), cause);
      }

      closePipeStream();
    }
  }

  public static boolean isUp () {
    if (bluetoothAdapter == null) return false;
    if (!bluetoothAdapter.isEnabled()) return false;
    return true;
  }

  public final BluetoothDevice getDevice () {
    if (!isUp()) return null;

    if (APITests.haveJellyBean) {
      return bluetoothAdapter.getRemoteDevice(remoteAddressBytes);
    } else {
      return bluetoothAdapter.getRemoteDevice(remoteAddressString);
    }
  }

  public final String getName () {
    BluetoothDevice device = getDevice();
    if (device == null) return null;
    return device.getName();
  }

  public static String getName (long address) {
    return new BluetoothConnection(address).getName();
  }

  public BluetoothConnection (long address) {
    remoteAddressValue = address;

    {
      long value = remoteAddressValue;
      int i = remoteAddressBytes.length;

      while (i > 0) {
        remoteAddressBytes[--i] = (byte)(value & 0XFF);
        value >>= 8;
      }
    }

    {
      StringBuilder sb = new StringBuilder();

      for (byte octet : remoteAddressBytes) {
        if (sb.length() > 0) sb.append(':');
        sb.append(String.format("%02X", octet));
      }

      remoteAddressString = sb.toString();
    }
  }

  private void closeBluetoothSocket () {
    if (bluetoothSocket != null) {
      try {
        bluetoothSocket.close();
      } catch (IOException exception) {
        Log.w(LOG_TAG, ("Bluetooth socket close failure: " + remoteAddressString), exception);
      }

      bluetoothSocket = null;
    }
  }

  private void closeInputStream () {
    if (inputStream != null) {
      try {
        inputStream.close();
      } catch (IOException exception) {
        Log.w(LOG_TAG, ("Bluetooth input stream close failure: " + remoteAddressString), exception);
      }

      inputStream = null;
    }
  }

  private void closeOutputStream () {
    if (outputStream != null) {
      try {
        outputStream.close();
      } catch (IOException exception) {
        Log.w(LOG_TAG, ("Bluetooth output stream close failure: " + remoteAddressString), exception);
      }

      outputStream = null;
    }
  }

  private void closePipeStream () {
    if (pipeStream != null) {
      try {
        pipeStream.close();
      } catch (IOException exception) {
        Log.w(LOG_TAG, ("Bluetooth pipe stream close failure: " + remoteAddressString), exception);
      }

      pipeStream = null;
    }
  }

  private void stopInputThread () {
    if (inputThread != null) {
      inputThread.stop = true;
      inputThread = null;
    }
  }

  public void close () {
    stopInputThread();
    closePipeStream();
    closeInputStream();
    closeOutputStream();
    closeBluetoothSocket();
  }

  public boolean canDiscover () {
    BluetoothDevice device = getDevice();

    for (ParcelUuid uuid : device.getUuids()) {
      if (uuid.getUuid().equals(SERIAL_PROFILE_UUID)) {
        return true;
      }
    }

    return false;
  }

  public static BluetoothSocket createRfcommSocket (BluetoothDevice device, int channel) {
    String className = device.getClass().getName();
    String methodName = "createRfcommSocket";
    String reference = className + "." + methodName;

    try {
      Class[] arguments = new Class[] {int.class};
      Method method = device.getClass().getMethod(methodName, arguments);
      return (BluetoothSocket)method.invoke(device, channel);
    } catch (NoSuchMethodException exception) {
      Log.w(LOG_TAG, "cannot find method: " + reference);
    } catch (IllegalAccessException exception) {
      Log.w(LOG_TAG, "cannot access method: " + reference);
    } catch (InvocationTargetException exception) {
      Log.w(LOG_TAG, reference + " failed", exception.getCause());
    }

    return null;
  }

  public boolean open (int inputPipe, int channel, boolean secure) {
    BluetoothDevice device = getDevice();
    if (device == null) return false;
    if (bluetoothAdapter.isDiscovering()) return false;

    if (channel == 0) {
      try {
        bluetoothSocket = secure? device.createRfcommSocketToServiceRecord(SERIAL_PROFILE_UUID):
                                  device.createInsecureRfcommSocketToServiceRecord(SERIAL_PROFILE_UUID);
      } catch (IOException exception) {
        Log.e(LOG_TAG, ("Bluetooth UUID resolution failed: " + remoteAddressString + ": " + exception.getMessage()));
        bluetoothSocket = null;
      }
    } else {
      bluetoothSocket = createRfcommSocket(device, channel);
    }

    if (bluetoothSocket != null) {
      try {
        bluetoothSocket.connect();

        inputStream = bluetoothSocket.getInputStream();
        outputStream = bluetoothSocket.getOutputStream();

        {
          File pipeFile = new File("/proc/self/fd/" + inputPipe);
          pipeStream = new FileOutputStream(pipeFile);
        }

        inputThread = new InputThread();
        inputThread.start();

        return true;
      } catch (IOException openException) {
        Log.d(LOG_TAG, ("Bluetooth connect failed: " + remoteAddressString + ": " + openException.getMessage()));
      }
    }

    close();
    return false;
  }

  public boolean write (byte[] bytes) {
    try {
      outputStream.write(bytes);
      outputStream.flush();
      return true;
    } catch (IOException exception) {
      Log.e(LOG_TAG, ("Bluetooth write failed: " + remoteAddressString), exception);
    }

    return false;
  }

  private static BluetoothDevice[] pairedDevices = null;

  public static int getPairedDeviceCount () {
    if (isUp()) {
      Set<BluetoothDevice> devices = bluetoothAdapter.getBondedDevices();

      if (devices != null) {
        pairedDevices = devices.toArray(new BluetoothDevice[devices.size()]);
        return pairedDevices.length;
      }
    }

    pairedDevices = null;
    return 0;
  }

  private static BluetoothDevice getPairedDevice (int index) {
    if (index >= 0) {
      if (pairedDevices != null) {
        if (index < pairedDevices.length) {
          return pairedDevices[index];
        }
      }
    }

    return null;
  }

  public static String getPairedDeviceAddress (int index) {
    BluetoothDevice device = getPairedDevice(index);
    if (device == null) return null;
    return device.getAddress();
  }

  public static String getPairedDeviceName (int index) {
    BluetoothDevice device = getPairedDevice(index);
    if (device == null) return null;
    return device.getName();
  }
}

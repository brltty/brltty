/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.util.UUID;

import android.os.Build;
import android.util.Log;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;

public class BluetoothConnection {
  private static final String LOG_TAG = BluetoothConnection.class.getName();

  protected static final UUID SERIAL_PROFILE_UUID = UUID.fromString(
    "00001101-0000-1000-8000-00805F9B34FB"
  );

  protected static final BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
  protected final BluetoothDevice bluetoothDevice;
  protected final String bluetoothAddress;

  protected BluetoothSocket bluetoothSocket = null;
  protected InputStream inputStream = null;
  protected OutputStream outputStream = null;
  protected OutputStream pipeStream = null;
  protected InputThread inputThread = null;

  private class InputThread extends Thread {
    volatile boolean stop = false;

    InputThread () {
      super("bluetooth-input-" + bluetoothAddress);
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
            Log.d(LOG_TAG, "Bluetooth input exception: " + bluetoothAddress + ": " + exception.getMessage());
            break;
          }

          if (byteCount > 0) {
            pipeStream.write(buffer, 0, byteCount);
            pipeStream.flush();
          } else if (byteCount < 0) {
            Log.d(LOG_TAG, "Bluetooth input end: " + bluetoothAddress);
            break;
          }
        }
      } catch (Throwable cause) {
        Log.e(LOG_TAG, "Bluetooth input failed: " + bluetoothAddress, cause);
      }

      closePipeStream();
    }
  }

  public static BluetoothDevice getDevice (long deviceAddress) {
    byte[] hardwareAddress = new byte[6];

    {
      int i = hardwareAddress.length;

      while (i > 0) {
        hardwareAddress[--i] = (byte)(deviceAddress & 0XFF);
        deviceAddress >>= 8;
      }
    }

    if (!ApplicationUtilities.haveSdkVersion(Build.VERSION_CODES.JELLY_BEAN)) {
      StringBuilder sb = new StringBuilder();

      for (byte octet : hardwareAddress) {
        if (sb.length() > 0) sb.append(':');
        sb.append(String.format("%02X", octet));
      }

      return bluetoothAdapter.getRemoteDevice(sb.toString());
    }

    return bluetoothAdapter.getRemoteDevice(hardwareAddress);
  }

  public static String getName (long deviceAddress) {
    return getDevice(deviceAddress).getName();
  }

  public BluetoothConnection (long deviceAddress) {
    bluetoothDevice = getDevice(deviceAddress);
    bluetoothAddress = bluetoothDevice.getAddress();
  }

  private void closeBluetoothSocket () {
    if (bluetoothSocket != null) {
      try {
        bluetoothSocket.close();
      } catch (IOException exception) {
        Log.w(LOG_TAG, "Bluetooth socket close failure: " + bluetoothAddress, exception);
      }

      bluetoothSocket = null;
    }
  }

  private void closeInputStream () {
    if (inputStream != null) {
      try {
        inputStream.close();
      } catch (IOException exception) {
        Log.w(LOG_TAG, "Bluetooth input stream close failure: " + bluetoothAddress, exception);
      }

      inputStream = null;
    }
  }

  private void closeOutputStream () {
    if (outputStream != null) {
      try {
        outputStream.close();
      } catch (IOException exception) {
        Log.w(LOG_TAG, "Bluetooth output stream close failure: " + bluetoothAddress, exception);
      }

      outputStream = null;
    }
  }

  private void closePipeStream () {
    if (pipeStream != null) {
      try {
        pipeStream.close();
      } catch (IOException exception) {
        Log.w(LOG_TAG, "Bluetooth pipe stream close failure: " + bluetoothAddress, exception);
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
    if (channel == 0) {
      try {
        bluetoothSocket = secure? bluetoothDevice.createRfcommSocketToServiceRecord(SERIAL_PROFILE_UUID):
                                  bluetoothDevice.createInsecureRfcommSocketToServiceRecord(SERIAL_PROFILE_UUID);
      } catch (IOException exception) {
        Log.e(LOG_TAG, "Bluetooth UUID resolution failed: " + bluetoothAddress + ": " + exception.getMessage());
        bluetoothSocket = null;
      }
    } else {
      bluetoothSocket = createRfcommSocket(bluetoothDevice, channel);
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
        Log.e(LOG_TAG, "Bluetooth connect failed: " + bluetoothAddress + ": " + openException.getMessage());
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
      Log.e(LOG_TAG, "Bluetooth write failed: " + bluetoothAddress, exception);
    }

    return false;
  }
}

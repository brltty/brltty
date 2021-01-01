/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2021 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
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

package org.a11y.brlapi;

import java.util.Map;
import java.util.HashMap;

import java.io.InterruptedIOException;
import java.util.concurrent.TimeoutException;

public class ConnectionBase extends NativeComponent implements AutoCloseable {
  private long connectionHandle;
  private final static Map<Long, ConnectionBase> connections = new HashMap<>();
  private boolean hasBecomeUnusable = false;

  private native int openConnection (
    ConnectionSettings desiredSettings, ConnectionSettings actualSettings
  ) throws ConnectException;

  private final ConnectionSettings connectionSettings;
  private final int fileDescriptor;

  public ConnectionBase (ConnectionSettings settings) throws ConnectException {
    super();
    connectionSettings = new ConnectionSettings();
    fileDescriptor = openConnection(settings, connectionSettings);

    synchronized (connections) {
      if (connections.putIfAbsent(connectionHandle, this) != null) {
        throw new IllegalStateException("connection handle already mapped");
      }
    }
  }

  public final String getServerHost () {
    return connectionSettings.getServerHost();
  }

  public final String getAuthenticationScheme () {
    return connectionSettings.getAuthenticationScheme();
  }

  public final int getFileDescriptor () {
    return fileDescriptor;
  }

  private native void closeConnection ();
  private boolean hasBeenClosed = false;

  @Override
  public void close () {
    synchronized (this) {
      if (!hasBeenClosed) {
        synchronized (connections) {
          connections.remove(connectionHandle);
        }

        closeConnection();
        hasBeenClosed = true;
      }
    }
  }

  public static ConnectionBase getConnection (long handle) {
    synchronized (connections) {
      return connections.get(handle);
    }
  }

  public static void setUnusable (long handle) {
    // this needs to be public but it mustn't be easy
    ConnectionBase connection = getConnection(handle);
    connection.hasBecomeUnusable = true;
  }

  public final boolean isUnusable () {
    return hasBecomeUnusable;
  }

  public native String getDriverName ();
  public native String getModelIdentifier ();
  public native DisplaySize getDisplaySize ();
  
  public native void pause (int milliseconds)
         throws InterruptedIOException;

  public native int enterTtyMode (int tty, String driver);
  public native void enterTtyModeWithPath (String driver, int... ttys);
  public native void leaveTtyMode ();
  public native void setFocus (int tty);

  protected native void writeText (int cursor, String text);
  protected native void writeDots (byte[] dots);
  public native void write (WriteArguments arguments);

  public native Long readKey (boolean wait) throws InterruptedIOException;

  public native long readKeyWithTimeout (int milliseconds)
         throws InterruptedIOException, TimeoutException;

  public native void ignoreKeys (long type, long[] keys);
  public native void acceptKeys (long type, long[] keys);

  public native void ignoreAllKeys ();
  public native void acceptAllKeys ();

  public native void ignoreKeyRanges (long[][] ranges);
  public native void acceptKeyRanges (long[][] ranges);

  public native void enterRawMode (String driver);
  public native void leaveRawMode ();
  public native int sendRaw (byte[] buffer);

  public native int recvRaw (byte[] buffer)
         throws InterruptedIOException;

  public native Object getParameter (int parameter, long subparam, boolean global);
  public native void setParameter (int parameter, long subparam, boolean global, Object value);
  public native long watchParameter (int parameter, long subparam, boolean global, ParameterWatcher watcher);
  public native static void unwatchParameter (long identifier);
}

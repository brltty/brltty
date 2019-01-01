/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2019 by
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

public class BasicConnection extends NativeLibrary {
  protected long connectionHandle;

  protected native int openConnection (
    ConnectionSettings desiredSettings,
    ConnectionSettings actualSettings
  );

  public native void closeConnection ();

  public native String getDriverName ();
  public native String getModelIdentifier ();
  public native DisplaySize getDisplaySize ();
  
  public native int enterTtyMode (int tty, String driver);
  public native void enterTtyModeWithPath (int[] ttys, String driver);
  public native void leaveTtyMode ();
  public native void setFocus (int tty);

  protected native void writeText (int cursor, String text);
  public native void writeDots (byte[] dots);
  public native void write (WriteArguments arguments);

  public native long readKey (boolean wait);
  public native long readKeyWithTimeout (int milliseconds);

  public native void ignoreKeys (long type, long[] keys);
  public native void acceptKeys (long type, long[] keys);

  public native void ignoreAllKeys ();
  public native void acceptAllKeys ();

  public native void ignoreKeyRanges (long[][] ranges);
  public native void acceptKeyRanges (long[][] ranges);

  public native void enterRawMode (String driver);
  public native void leaveRawMode ();
  public native int sendRaw (byte[] buffer);
  public native int recvRaw (byte[] buffer);

  protected final ConnectionSettings connectionSettings;
  protected final int fileDescriptor;

  public BasicConnection (ConnectionSettings settings) {
    super();
    connectionSettings = new ConnectionSettings();
    fileDescriptor = openConnection(settings, connectionSettings);
  }

  public String getServerHost () {
    return connectionSettings.getServerHost();
  }

  public String getAuthorizationSchemes () {
    return connectionSettings.getAuthorizationSchemes();
  }

  public int getFileDescriptor () {
    return fileDescriptor;
  }
}

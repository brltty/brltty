/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2010 by
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
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.BrlAPI;

public class Native {
  static {
    System.loadLibrary("brlapi_java");
  }

  protected long handle;

  protected native int openConnection (
    ConnectionSettings desiredSettings,
    ConnectionSettings actualSettings)
    throws Error;
  public native void closeConnection ();

  public native String getDriverName () throws Error;
  public native DisplaySize getDisplaySize () throws Error;
  
  public native int enterTtyMode (int tty, String driver) throws Error;
  public native void enterTtyModeWithPath (int ttys[], String driver) throws Error;
  public native void leaveTtyMode () throws Error;
  public native void setFocus (int tty) throws Error;

  protected native void writeTextNative (int cursor, String text) throws Error;
  public native void writeDots (byte dots[]) throws Error;
  public native void write (WriteArguments arguments) throws Error;

  public native long readKey (boolean wait) throws Error;
  public native void ignoreKeys (long type, long keys[]) throws Error;
  public native void acceptKeys (long type, long keys[]) throws Error;

  public native void ignoreAllKeys () throws Error;
  public native void acceptAllKeys () throws Error;

  public native void ignoreKeyRanges (long ranges[][]) throws Error;
  public native void acceptKeyRanges (long ranges[][]) throws Error;

  public native void enterRawMode (String driver) throws Error;
  public native void leaveRawMode () throws Error;
  public native int sendRaw (byte buffer[]) throws Error;
  public native int recvRaw (byte buffer[]) throws Error;

  public static native String getPacketTypeName (long type);
}

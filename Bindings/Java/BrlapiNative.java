/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2008 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 * Please see the file COPYING-API for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

public class BrlapiNative {
  protected long handle;

  protected native int openConnection (
    BrlapiSettings desiredSettings,
    BrlapiSettings actualSettings)
    throws BrlapiError;
  public native void closeConnection ();

  public native String getDriverName () throws BrlapiError;
  public native BrlapiSize getDisplaySize () throws BrlapiError;
  
  public native int enterTtyMode (int tty, String driver) throws BrlapiError;
  public native void enterTtyModeWithPath (int ttys[], String driver) throws BrlapiError;
  public native void leaveTtyMode () throws BrlapiError;
  public native void setFocus (int tty) throws BrlapiError;

  protected native void writeTextNative (int cursor, String text) throws BrlapiError;
  public native void writeDots (byte dots[]) throws BrlapiError;
  public native void write (BrlapiWriteArguments arguments) throws BrlapiError;

  public native long readKey (boolean wait) throws BrlapiError;
  public native void ignoreKeys (long type, long keys[]) throws BrlapiError;
  public native void acceptKeys (long type, long keys[]) throws BrlapiError;

  public native void ignoreAllKeys () throws BrlapiError;
  public native void acceptAllKeys () throws BrlapiError;

  public native void ignoreKeyRanges (long ranges[][]) throws BrlapiError;
  public native void acceptKeyRanges (long ranges[][]) throws BrlapiError;

  public native void enterRawMode (String driver) throws BrlapiError;
  public native void leaveRawMode () throws BrlapiError;
  public native int sendRaw (byte buffer[]) throws BrlapiError;
  public native int recvRaw (byte buffer[]) throws BrlapiError;

  public static native String getPacketTypeName (long type);
}

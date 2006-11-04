/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2006 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 * All rights reserved.
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

  protected native int initializeConnection (
    BrlapiSettings clientSettings,
    BrlapiSettings usedSettings)
    throws BrlapiError;
  public native void closeConnection ();
  public static native byte[] loadAuthKey (String path);

  public native String getDriverId () throws BrlapiError;
  public native String getDriverName () throws BrlapiError;
  public native BrlapiSize getDisplaySize () throws BrlapiError;
  
  public native int enterTtyMode (int tty, String driver) throws BrlapiError;
  public native void enterTtyModeWithPath (int ttys[], String driver) throws BrlapiError;
  public native void leaveTtyMode () throws BrlapiError;
  public native void setFocus (int tty) throws BrlapiError;

  public native void writeText (int cursor, String str) throws BrlapiError;
  public native void writeDots (byte str[]) throws BrlapiError;
  public native void write (BrlapiWriteStruct s) throws BrlapiError;

  public native long readKey (boolean block) throws BrlapiError;
  public native void ignoreKeyRange (long x, long y) throws BrlapiError;
  public native void acceptKeyRange (long x, long y) throws BrlapiError;
  public native void ignoreKeySet (long s[]) throws BrlapiError;
  public native void acceptKeySet (long s[]) throws BrlapiError;

  public native void enterRawMode (String driver) throws BrlapiError;
  public native void leaveRawMode () throws BrlapiError;
  public native int sendRaw (byte buf[]) throws BrlapiError;
  public native int recvRaw (byte buf[]) throws BrlapiError;

  public static native String packetType (long type);
}

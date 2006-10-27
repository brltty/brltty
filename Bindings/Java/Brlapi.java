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

public class Brlapi implements BrlapiConstants {
  private final BrlapiSettings settings;
  private long handle;
  private final int fileDescriptor;;

  public Brlapi (BrlapiSettings settings) throws BrlapiError {
    this.settings = new BrlapiSettings();
    fileDescriptor = initializeConnection(settings, this.settings);
  }

  protected void finalize () {
    closeConnection();
  }

  private final native int initializeConnection (
    BrlapiSettings clientSettings,
    BrlapiSettings usedSettings)
    throws BrlapiError;
  public final native void closeConnection ();
  public final static native byte[] loadAuthKey (String path);

  public String getHostName () {
    return settings.hostName;
  }

  public String getAuthKey () {
    return settings.authKey;
  }

  public int getFileDescriptor () {
    return fileDescriptor;
  }

  public final native String getDriverId () throws BrlapiError;
  public final native String getDriverName () throws BrlapiError;
  public final native BrlapiSize getDisplaySize () throws BrlapiError;
  /*public final native byte[] getDriverInfo () throws BrlapiError;*/
  
  public final native int enterTtyMode (int tty, String driver) throws BrlapiError;
  public final native int getTtyPath (int ttys[], String driver) throws BrlapiError;
  public final native void leaveTtyMode () throws BrlapiError;
  public final native void setFocus (int tty) throws BrlapiError;

  public final native void writeText (int cursor, String str) throws BrlapiError;
  public final native int writeDots (byte str[]) throws BrlapiError;
  public final native void write (BrlapiWriteStruct s) throws BrlapiError;

  public final native long readKey (boolean block) throws BrlapiError;
  public final native void ignoreKeyRange (long x, long y) throws BrlapiError;
  public final native void unignoreKeyRange (long x, long y) throws BrlapiError;
  public final native void ignoreKeySet (long s[]) throws BrlapiError;
  public final native void unignoreKeySet (long s[]) throws BrlapiError;

  public final native void enterRawMode (String driver) throws BrlapiError;
  public final native void leaveRawMode () throws BrlapiError;
  public final native int sendRaw (byte buf[]) throws BrlapiError;
  public final native int recvRaw (byte buf[]) throws BrlapiError;

  public final static native String packetType (long type);
}

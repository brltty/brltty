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

public class Brlapi {
  public static class settings {
    public String authKey;
    public String hostName;
    public settings(String authKey, String hostName) {
      this.authKey = authKey;
      this.hostName = hostName;
    }
    public settings() { this (null, null); }
    public settings(String hostName) { this ((String) null, hostName); }

  };
  public final native int initializeConnection(settings clientSettings,
      settings usedSettings) throws BrlapiError;
  public final native void closeConnection();

  public final static native byte[] loadAuthKey(String jpath);

  public final native String getDriverId() throws BrlapiError;
  public final native String getDriverName() throws BrlapiError;
  public final native BrlapiSize getDisplaySize() throws BrlapiError;
  /*public final native byte[] getDriverInfo() throws BrlapiError;*/
  
  public final native int enterTtyMode(int tty, String how) throws BrlapiError;
  public final native int getTtyPath(int ttys[], String how) throws BrlapiError;
  public final native void leaveTtyMode() throws BrlapiError;
  public final native void setFocus(int tty) throws BrlapiError;

  public final native void writeText(int cursor, String str) throws BrlapiError;
  public final native int writeDots(byte str[]) throws BrlapiError;
  public static class writeStruct {
    public int displayNumber;
    public int regionBegin, regionSize;
    public String text;
    public byte attrAnd[];
    public byte attrOr[];
    public int cursor;
    public writeStruct(int displayNumber, int regionBegin, int regionSize,
		    String text, byte attrAnd[], byte attrOr[], int cursor) {
      this.displayNumber = displayNumber;
      this.regionBegin = regionBegin;
      this.regionSize = regionSize;
      this.text = text;
      this.attrAnd = attrAnd;
      this.attrOr = attrOr;
      this.cursor = cursor;
    }
    public writeStruct() { this (-1, 0, 0, null, null, null, -1); }
  }
  public final native void write(writeStruct s) throws BrlapiError;

  public final native long readKey(boolean block) throws BrlapiError;
  public final native void ignoreKeyRange(long x, long y) throws BrlapiError;
  public final native void ignoreKeySet(long s[]) throws BrlapiError;
  public final native void unignoreKeyRange(long x, long y) throws BrlapiError;
  public final native void unignoreKeySet(long s[]) throws BrlapiError;

  public final native void enterRawMode(String driver) throws BrlapiError;
  public final native void leaveRawMode() throws BrlapiError;
  public final native int sendRaw(byte buf[]) throws BrlapiError;
  public final native int recvRaw(byte buf[]) throws BrlapiError;

  public final static native String packetType(long type);

  public long handle;
}

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

public class Brlapi extends BrlapiNative implements BrlapiConstants {
  protected final BrlapiSettings settings;
  protected final int fileDescriptor;;
  protected int ttyNumber = -1;

  public Brlapi (BrlapiSettings settings) throws BrlapiError {
    this.settings = new BrlapiSettings();
    fileDescriptor = initializeConnection(settings, this.settings);
  }

  protected void finalize () {
    closeConnection();
  }

  public String getHostName () {
    return settings.hostName;
  }

  public String getAuthKey () {
    return settings.authKey;
  }

  public int getFileDescriptor () {
    return fileDescriptor;
  }

  public int enterTtyMode (int tty, String driver) throws BrlapiError {
    return ttyNumber = super.enterTtyMode(tty, driver);
  }

  public int getTtyPath (int ttys[], String driver) throws BrlapiError {
    return ttyNumber = super.getTtyPath(ttys, driver);
  }

  public void leaveTtyMode () throws BrlapiError {
    ttyNumber = -1;
    super.leaveTtyMode();
  }

  public int getTtyNumber () {
    return ttyNumber;
  }
}

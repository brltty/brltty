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

package org.a11y.BrlAPI;

public class Brlapi extends BrlapiNative implements BrlapiConstants {
  protected final BrlapiSettings settings;
  protected final int fileDescriptor;

  public Brlapi (BrlapiSettings settings) throws BrlapiError {
    this.settings = new BrlapiSettings();
    fileDescriptor = openConnection(settings, this.settings);
  }

  protected void finalize () {
    closeConnection();
  }

  public String getHost () {
    return settings.host;
  }

  public String getAuth () {
    return settings.auth;
  }

  public int getFileDescriptor () {
    return fileDescriptor;
  }

  public int enterTtyMode (int tty) throws BrlapiError {
    return enterTtyMode(tty, null);
  }

  public int enterTtyMode (String driver) throws BrlapiError {
    return enterTtyMode(TTY_DEFAULT, driver);
  }

  public int enterTtyMode () throws BrlapiError {
    return enterTtyMode(null);
  }

  public void enterTtyModeWithPath (int ttys[]) throws BrlapiError {
    enterTtyModeWithPath(ttys, null);
  }

  public void writeText (int cursor) throws BrlapiError {
    writeText(cursor, null);
  }

  public void writeText (String text) throws BrlapiError {
    writeText(CURSOR_OFF, text);
  }

  public void writeText (String text, int cursor) throws BrlapiError {
    writeText(cursor, text);
  }

  public void writeText (int cursor, String text) throws BrlapiError {
    if (text != null) {
      BrlapiSize size = getDisplaySize();
      int count = size.getWidth() * size.getHeight();

      {
        StringBuffer sb = new StringBuffer(text);
        while (sb.length() < count) sb.append(' ');
        text = sb.toString();
      }

      text = text.substring(0, count);
    }
    writeTextNative(cursor, text);
  }
}

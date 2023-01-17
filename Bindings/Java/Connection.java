/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2023 by
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

import java.io.InterruptedIOException;

public class Connection extends ConnectionBase {
  public Connection (ConnectionSettings settings) throws ConnectException {
    super(settings);
  }

  public int getCellCount () {
    DisplaySize size = getDisplaySize();
    return size.getWidth() * size.getHeight();
  }

  public int enterTtyMode (int tty) {
    return enterTtyMode(tty, null);
  }

  public int enterTtyMode (String driver) {
    return enterTtyMode(Constants.TTY_DEFAULT, driver);
  }

  public int enterTtyMode () {
    return enterTtyMode(null);
  }

  public void enterTtyModeWithPath (int[] ttys, String driver) {
    enterTtyModeWithPath(driver, ttys);
  }

  public void enterTtyModeWithPath (int... ttys) {
    enterTtyModeWithPath(null, ttys);
  }

  public final long readKey () throws InterruptedIOException {
    return readKey(true);
  }

  public void write (byte[] dots) {
    int count = getCellCount();

    if (dots.length != count) {
      byte[] newDots = new byte[count];
      while (count > dots.length) newDots[--count] = 0;
      System.arraycopy(dots, 0, newDots, 0, count);
      dots = newDots;
    }

    writeDots(dots);
  }

  public void write (int cursor, String text) {
    if (text != null) {
      int count = getCellCount();

      {
        StringBuilder newText = new StringBuilder(text);
        while (newText.length() < count) newText.append(' ');
        text = newText.toString();
      }

      if (text.length() > count) text = text.substring(0, count);
    }

    writeText(cursor, text);
  }

  public void write (String text, int cursor) {
    write(cursor, text);
  }

  public void write (int cursor) {
    write(cursor, null);
  }

  public void write (String text) {
    write(Constants.CURSOR_LEAVE, text);
  }

  private Parameters connectionParameters = null;
  public Parameters getParameters () {
    synchronized (this) {
      if (connectionParameters == null) {
        connectionParameters = new Parameters(this);
      }
    }

    return connectionParameters;
  }
}

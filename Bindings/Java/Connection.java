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

public class Connection extends BasicConnection {
  public Connection (ConnectionSettings settings) {
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

  public void enterTtyModeWithPath (int[] ttys) {
    enterTtyModeWithPath(ttys, null);
  }

  public void writeDots (byte[] dots) {
    int count = getCellCount();

    if (dots.length != count) {
      byte[] d = new byte[count];
      while (count > dots.length) d[--count] = 0;
      System.arraycopy(dots, 0, d, 0, count);
      dots = d;
    }

    super.writeDots(dots);
  }

  public void writeText (int cursor, String text) {
    if (text != null) {
      int count = getCellCount();

      {
        StringBuilder sb = new StringBuilder(text);
        while (sb.length() < count) sb.append(' ');
        text = sb.toString();
      }

      text = text.substring(0, count);
    }

    super.writeText(cursor, text);
  }

  public void writeText (String text, int cursor) {
    writeText(cursor, text);
  }

  public void writeText (int cursor) {
    writeText(cursor, null);
  }

  public void writeText (String text) {
    writeText(Constants.CURSOR_OFF, text);
  }
}

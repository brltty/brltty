/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2020 by
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

public class ComputerBrailleRowCells {
  private final byte[] cells = new byte[0X100];
  private final byte[] mask = new byte[0X20];

  private final int copyBytes (byte[] to, byte[] from, int index) {
    int count = to.length;
    System.arraycopy(from, index, to, 0, count);
    return index + count;
  }

  public ComputerBrailleRowCells (byte[] bytes) {
    int index = 0;
    index = copyBytes(cells, bytes, index);
    index = copyBytes(mask, bytes, index);
  }

  public byte[] getCells () {
    return cells;
  }

  public byte[] getMask () {
    return mask;
  }
}

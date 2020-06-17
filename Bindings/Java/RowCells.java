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

public class RowCells {
  private final static int BYTE_SIZE = Byte.SIZE;

  private final byte[] cellArray;
  private final BitMask cellMask;

  private final int copyBytes (byte[] to, byte[] from, int index) {
    int count = to.length;
    System.arraycopy(from, index, to, 0, count);
    return index + count;
  }

  public RowCells (byte[] bytes) {
    cellArray = new byte[0X100];
    int index = copyBytes(cellArray, bytes, 0);

    {
      byte[] mask = new byte[cellArray.length / BYTE_SIZE];
      index = copyBytes(mask, bytes, index);
      cellMask = new BitMask(mask);
    }
  }

  public int getSize () {
    return cellArray.length;
  }

  public boolean isDefined (int index) {
    return cellMask.isSet(index);
  }

  public byte getCell (int index) {
    return isDefined(index)? cellArray[index]: 0;
  }

  @Override
  public String toString () {
    StringBuilder sb = new StringBuilder();
    sb.append('{');
    boolean first = true;

    for (int index : cellMask.getBitNumbers()) {
      if (first) {
        first = false;
      } else {
        sb.append(", ");
      }

      sb.append(String.format("%02X:", index));
      int cell = cellArray[index];

      if (cell != 0) {
        cell &= 0XFF;
        int dot = 1;

        while (true) {
          if ((cell & 1) != 0) sb.append(dot);
          if ((cell >>= 1) == 0) break;
          dot += 1;
        }
      } else {
        sb.append(0);
      }
    }

    sb.append('}');
    return sb.toString();
  }
}

/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2021 by
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

import java.util.Arrays;

public abstract class ParameterComponent extends Component {
  protected ParameterComponent () {
    super();
  }

  public static String asString (Object value) {
    return (String)value;
  }

  public static boolean[] asBooleanArray (Object value) {
    return (boolean[])value;
  }

  public static byte[] asByteArray (Object value) {
    return (byte[])value;
  }

  public static short[] asShortArray (Object value) {
    return (short[])value;
  }

  public static int[] asIntArray (Object value) {
    return (int[])value;
  }

  public static long[] asLongArray (Object value) {
    return (long[])value;
  }

  public static boolean asBoolean (Object value) {
    return asBooleanArray(value)[0];
  }

  public static byte asByte (Object value) {
    return asByteArray(value)[0];
  }

  public static short asShort (Object value) {
    return asShortArray(value)[0];
  }

  public static int asInt (Object value) {
    return asIntArray(value)[0];
  }

  public static long asLong (Object value) {
    return asLongArray(value)[0];
  }

  public static DisplaySize asDisplaySize (Object value) {
    return new DisplaySize(asIntArray(value));
  }

  public static BitMask asBitMask (Object value) {
    return new BitMask(asByteArray(value));
  }

  public static RowCells asRowCells (Object value) {
    return new RowCells(asByteArray(value));
  }

  public static String asDots (byte cell) {
    if (cell == 0) return "0";

    StringBuilder numbers = new StringBuilder();
    int dots = cell & BYTE_MASK;
    int dot = 1;

    while (true) {
      if ((dots & 1) != 0) numbers.append(dot);
      if ((dots >>= 1) == 0) break;
      dot += 1;
    }

    return numbers.toString();
  }

  public static String asDots (byte[] cells) {
    int count = cells.length;
    String[] dots = new String[count];

    for (int i=0; i<count; i+=1) {
      dots[i] = asDots(cells[i]);
    }

    return Arrays.toString(dots);
  }

  public static String toString (Object value) {
    if (value == null) return null;

    if (value.getClass().isArray()) {
      if (value instanceof boolean[]) {
        return Arrays.toString((boolean[])value);
      }

      if (value instanceof byte[]) {
        return Arrays.toString((byte[])value);
      }

      if (value instanceof short[]) {
        return Arrays.toString((short[])value);
      }

      if (value instanceof int[]) {
        return Arrays.toString((int[])value);
      }

      if (value instanceof long[]) {
        return Arrays.toString((long[])value);
      }
    }

    return value.toString();
  }
}

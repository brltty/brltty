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

import java.util.Arrays;

public class BitMask extends Component {
  private final static int BYTE_SIZE = Byte.SIZE;

  private final byte[] maskBytes;
  private final int maskSize;

  public BitMask (byte[] bytes) {
    maskBytes = bytes;
    maskSize = maskBytes.length * BYTE_SIZE;
  }

  public int getSize () {
    return maskSize;
  }

  public boolean isSet (int index) {
    if (index < 0) return false;
    if (index >= maskSize) return false;

    int bit = 1 << (index % BYTE_SIZE);
    index /= BYTE_SIZE;
    return (maskBytes[index] & bit) != 0;
  }

  private int[] bitNumbers = null;

  private final int[] newBitNumbers () {
    int size = getSize();
    int[] buffer = new int[size];
    int count = 0;
    int start = 0;

    for (int bits : maskBytes) {
      if ((bits &= BYTE_MASK) != 0) {
        int bit = start;

        while (true) {
          if ((bits & 1) != 0) buffer[count++] = bit;
          if ((bits >>= 1) == 0) break;
          bit += 1;
        }
      }

      start += BYTE_SIZE;
    }

    int[] result = new int[count];
    System.arraycopy(buffer, 0, result, 0, count);
    return result;
  }

  public final int[] getBitNumbers () {
    synchronized (this) {
      if (bitNumbers == null) bitNumbers = newBitNumbers();
    }

    int count = bitNumbers.length;
    int[] result = new int[count];
    System.arraycopy(bitNumbers, 0, result, 0, count);
    return result;
  }

  @Override
  public String toString () {
    return Arrays.toString(getBitNumbers());
  }
}

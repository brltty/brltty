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

public abstract class Component {
  protected Component () {
  }

  public final static int BYTE_MASK = (1 << Byte.SIZE) - 1;
  public final static char UNICODE_BRAILLE_ROW = 0X2800;

  public static char toUnicodeBraille (byte dots) {
    char character = UNICODE_BRAILLE_ROW;
    character |= dots & BYTE_MASK;
    return character;
  }

  public static String toUnicodeBraille (byte[] dots) {
    int count = dots.length;
    char[] characters = new char[count];
    for (int i=0; i<count; i+=1) characters[i] = toUnicodeBraille(dots[i]);
    return new String(characters);
  }

  public static void printf (String format, Object... arguments) {
    System.out.printf(format, arguments);
  }

  public static String toOperandName (String string) {
    return string.replace(' ', '-').toLowerCase();
  }
}

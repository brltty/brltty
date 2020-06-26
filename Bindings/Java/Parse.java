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

public abstract class Parse {
  private Parse () {
    super();
  }

  public static Boolean asBoolean (String description, String operand)
         throws OperandException {
    if (operand.equals("false")) return false;
    if (operand.equals("true")) return true;

    if (operand.equals("off")) return false;
    if (operand.equals("on")) return true;

    if (operand.equals("no")) return false;
    if (operand.equals("yes")) return true;

    if (operand.equals("0")) return false;
    if (operand.equals("1")) return true;

    throw new OperandException("not a boolean: %s: %s", description, operand);
  }

  public static Long asLong (String description, String operand, long minimum, long maximum)
         throws OperandException {
    long value;

    try {
      value = Long.valueOf(operand);

      if (value < minimum) {
        throw new OperandException(
          "less than %d: %s: %s", minimum, description, operand
        );
      }

      if (value > maximum) {
        throw new OperandException(
          "greater than %d: %s: %s", maximum, description, operand
        );
      }

      return value;
    } catch (NumberFormatException exception) {
      throw new OperandException(
        "not an integer: %s: %s", description, operand
      );
    }
  }

  public static Long asLong (String description, String operand, long maximum)
         throws OperandException {
    return asLong(description, operand, 0, maximum);
  }

  public static Long asLong (String description, String operand)
         throws OperandException {
    return asLong(description, operand, Long.MAX_VALUE);
  }

  public static Integer asInt (String description, String operand, int minimum, int maximum)
         throws OperandException {
    return asLong(description, operand, minimum, maximum).intValue();
  }

  public static Integer asInt (String description, String operand, int maximum)
         throws OperandException {
    return asInt(description, operand, 0, maximum);
  }

  public static Integer asInt (String description, String operand)
         throws OperandException {
    return asInt(description, operand, Integer.MAX_VALUE);
  }

  public static Short asShort (String description, String operand, short minimum, short maximum)
         throws OperandException {
    return asLong(description, operand, minimum, maximum).shortValue();
  }

  public static Short asShort (String description, String operand, short maximum)
         throws OperandException {
    return asShort(description, operand, (short)0, maximum);
  }

  public static Short asShort (String description, String operand)
         throws OperandException {
    return asShort(description, operand, Short.MAX_VALUE);
  }

  public static Byte asByte (String description, String operand, byte minimum, byte maximum)
         throws OperandException {
    return asLong(description, operand, minimum, maximum).byteValue();
  }

  public static Byte asByte (String description, String operand, byte maximum)
         throws OperandException {
    return asByte(description, operand, (byte)0, maximum);
  }

  public static Byte asByte (String description, String operand)
         throws OperandException {
    return asByte(description, operand, Byte.MAX_VALUE);
  }
}

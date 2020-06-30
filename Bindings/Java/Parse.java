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

  private final static KeywordMap<Boolean> booleanKeywords = new KeywordMap<>();
  static {
    Boolean FALSE = false;
    Boolean TRUE = true;

    booleanKeywords.put("false", FALSE);
    booleanKeywords.put("true", TRUE);

    booleanKeywords.put("off", FALSE);
    booleanKeywords.put("on", TRUE);

    booleanKeywords.put("no", FALSE);
    booleanKeywords.put("yes", TRUE);

    booleanKeywords.put("0", FALSE);
    booleanKeywords.put("1", TRUE);
  }

  public static Boolean asBoolean (String description, String operand)
         throws SyntaxException
  {
    Boolean value = booleanKeywords.get(operand);
    if (value != null) return value;
    throw new SyntaxException("not a boolean: %s: %s", description, operand);
  }

  private static Number asNumber (String description, String operand, long minimum, long maximum)
         throws SyntaxException
  {
    long value;

    try {
      value = Long.valueOf(operand);

      if (value < minimum) {
        throw new SyntaxException(
          "less than %d: %s: %s", minimum, description, operand
        );
      }

      if (value > maximum) {
        throw new SyntaxException(
          "greater than %d: %s: %s", maximum, description, operand
        );
      }

      return new Long(value);
    } catch (NumberFormatException exception) {
      throw new SyntaxException(
        "not an integer: %s: %s", description, operand
      );
    }
  }

  public static long asLong (String description, String operand, long minimum, long maximum)
         throws SyntaxException
  {
    return asNumber(description, operand, minimum, maximum).longValue();
  }

  public static long asLong (String description, String operand, long maximum)
         throws SyntaxException
  {
    return asLong(description, operand, 0, maximum);
  }

  public static long asLong (String description, String operand)
         throws SyntaxException
  {
    return asLong(description, operand, Long.MAX_VALUE);
  }

  public static int asInt (String description, String operand, int minimum, int maximum)
         throws SyntaxException
  {
    return asNumber(description, operand, minimum, maximum).intValue();
  }

  public static int asInt (String description, String operand, int maximum)
         throws SyntaxException
  {
    return asInt(description, operand, 0, maximum);
  }

  public static int asInt (String description, String operand)
         throws SyntaxException
  {
    return asInt(description, operand, Integer.MAX_VALUE);
  }

  public static short asShort (String description, String operand, short minimum, short maximum)
         throws SyntaxException
  {
    return asNumber(description, operand, minimum, maximum).shortValue();
  }

  public static short asShort (String description, String operand, short maximum)
         throws SyntaxException
  {
    return asShort(description, operand, (short)0, maximum);
  }

  public static short asShort (String description, String operand)
         throws SyntaxException
  {
    return asShort(description, operand, Short.MAX_VALUE);
  }

  public static byte asByte (String description, String operand, byte minimum, byte maximum)
         throws SyntaxException
  {
    return asNumber(description, operand, minimum, maximum).byteValue();
  }

  public static byte asByte (String description, String operand, byte maximum)
         throws SyntaxException
  {
    return asByte(description, operand, (byte)0, maximum);
  }

  public static byte asByte (String description, String operand)
         throws SyntaxException
  {
    return asByte(description, operand, Byte.MAX_VALUE);
  }

  public static byte asDots (String description, String operand)
         throws SyntaxException
  {
    if (operand.equals("0")) return 0;

    byte dots = 0;
    String numbers = "12345678";

    for (char number : operand.toCharArray()) {
      int index = numbers.indexOf(number);
      if (index < 0) {
        throw new SyntaxException(
          "not a dot number: %s: %s (%c)", description, operand, number
        );
      }

      int dot = 1 << index;
      if ((dots & dot) != 0) {
        throw new SyntaxException(
          "duplicate dot number: %s: %s (%c)", description, operand, number
        );
      }

      dots |= dot;
    }

    return dots;
  }
}

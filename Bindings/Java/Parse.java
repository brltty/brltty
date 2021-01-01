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
    throw new SyntaxException("%s is not a boolean: %s", description, operand);
  }

  public static void checkMinimum (String description, long value, long minimum)
         throws SyntaxException
  {
    if (value < minimum) {
      throw new SyntaxException(
        "%s is less than %d: %d", description, minimum, value
      );
    }
  }

  public static void checkMaximum (String description, long value, long maximum)
         throws SyntaxException
  {
    if (value > maximum) {
      throw new SyntaxException(
        "%s is greater than %d: %d", description, maximum, value
      );
    }
  }

  public static void checkRange (String description, long value, long minimum, long maximum)
         throws SyntaxException
  {
    checkMinimum(description, value, minimum);
    checkMaximum(description, value, maximum);
  }

  private static Number asNumber (String description, String operand, long minimum, long maximum)
          throws SyntaxException
  {
    long value;

    try {
      value = Long.valueOf(operand);
      checkRange(description, value, minimum, maximum);
      return Long.valueOf(value);
    } catch (NumberFormatException exception) {
      throw new SyntaxException(
        "%s is not an integer: %s", description, operand
      );
    }
  }

  public final static byte DEFAULT_RANGE_MINIMUM = 0;

  public static long asLong (String description, String operand, long minimum, long maximum)
         throws SyntaxException
  {
    return asNumber(description, operand, minimum, maximum).longValue();
  }

  public static long asLong (String description, String operand, long minimum)
         throws SyntaxException
  {
    return asLong(description, operand, minimum, Long.MAX_VALUE);
  }

  public static long asLong (String description, String operand)
         throws SyntaxException
  {
    return asLong(description, operand, DEFAULT_RANGE_MINIMUM);
  }

  public static int asInt (String description, String operand, int minimum, int maximum)
         throws SyntaxException
  {
    return asNumber(description, operand, minimum, maximum).intValue();
  }

  public static int asInt (String description, String operand, int minimum)
         throws SyntaxException
  {
    return asInt(description, operand, minimum, Integer.MAX_VALUE);
  }

  public static int asInt (String description, String operand)
         throws SyntaxException
  {
    return asInt(description, operand, DEFAULT_RANGE_MINIMUM);
  }

  public static short asShort (String description, String operand, short minimum, short maximum)
         throws SyntaxException
  {
    return asNumber(description, operand, minimum, maximum).shortValue();
  }

  public static short asShort (String description, String operand, short minimum)
         throws SyntaxException
  {
    return asShort(description, operand, minimum, Short.MAX_VALUE);
  }

  public static short asShort (String description, String operand)
         throws SyntaxException
  {
    return asShort(description, operand, DEFAULT_RANGE_MINIMUM);
  }

  public static byte asByte (String description, String operand, byte minimum, byte maximum)
         throws SyntaxException
  {
    return asNumber(description, operand, minimum, maximum).byteValue();
  }

  public static byte asByte (String description, String operand, byte minimum)
         throws SyntaxException
  {
    return asByte(description, operand, minimum, Byte.MAX_VALUE);
  }

  public static byte asByte (String description, String operand)
         throws SyntaxException
  {
    return asByte(description, operand, DEFAULT_RANGE_MINIMUM);
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
          "%s contains an invalid dot number: %s (%c)", description, operand, number
        );
      }

      int dot = 1 << index;
      if ((dots & dot) != 0) {
        throw new SyntaxException(
          "%s contains a duplicate dot number: %s (%c)", description, operand, number
        );
      }

      dots |= dot;
    }

    return dots;
  }

  public final static int MINIMUM_CURSOR_POSITION =  1;
  public final static String NO_CURSOR = "no";
  public final static String LEAVE_CURSOR = "leave";

  public static int asCursorPosition (String operand) throws SyntaxException {
    if (Strings.isAbbreviation(NO_CURSOR, operand)) {
      return Constants.CURSOR_OFF;
    }

    if (Strings.isAbbreviation(LEAVE_CURSOR, operand)) {
      return Constants.CURSOR_LEAVE;
    }

    return asInt(
      WriteArguments.CURSOR_POSITION,
      operand, MINIMUM_CURSOR_POSITION
    );
  }

  public final static int MINIMUM_DISPLAY_NUMBER = 1;
  public final static String DEFAULT_DISPLAY = "default";

  public static int asDisplayNumber (String operand) throws SyntaxException {
    if (Strings.isAbbreviation(DEFAULT_DISPLAY, operand)) {
      return Constants.DISPLAY_DEFAULT;
    }

    return asInt(
      WriteArguments.DISPLAY_NUMBER,
      operand, MINIMUM_DISPLAY_NUMBER
    );
  }
}

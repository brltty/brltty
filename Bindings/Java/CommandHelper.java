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

public abstract class CommandHelper {
  protected CommandHelper () {
  }

  public static void writeCommandMessage (String format, Object... arguments) {
    System.err.println(String.format(format, arguments));
  }

  public static void syntaxError (String format, Object... arguments) {
    writeCommandMessage(format, arguments);
    System.exit(2);
  }

  public static void semanticError (String format, Object... arguments) {
    writeCommandMessage(format, arguments);
    System.exit(3);
  }

  public static void internalError (String format, Object... arguments) {
    writeCommandMessage(format, arguments);
    System.exit(4);
  }

  protected final void tooManyParameters () {
    syntaxError("too many parameters");
  }

  public static Boolean parseBoolean (String description, String operand) {
    if (operand.equals("false")) return false;
    if (operand.equals("true")) return true;

    if (operand.equals("off")) return false;
    if (operand.equals("on")) return true;

    if (operand.equals("no")) return false;
    if (operand.equals("yes")) return true;

    if (operand.equals("0")) return false;
    if (operand.equals("1")) return true;

    syntaxError("not a boolean: %s: %s", description, operand);
    return null;
  }

  public static Long parseLong (String description, String operand, long minimum, long maximum) {
    long value;

    try {
      value = Long.valueOf(operand);

      if (value < minimum) {
        syntaxError(
          "less than %d: %s: %s", minimum, description, operand
        );
      }

      if (value > maximum) {
        syntaxError(
          "greater than %d: %s: %s", maximum, description, operand
        );
      }

      return value;
    } catch (NumberFormatException exception) {
      syntaxError(
        "not an integer: %s: %s", description, operand
      );
    }

    return null;
  }

  public static Long parseLong (String description, String operand, long maximum) {
    return parseLong(description, operand, 0, maximum);
  }

  public static Long parseLong (String description, String operand) {
    return parseLong(description, operand, Long.MAX_VALUE);
  }

  public static Integer parseInt (String description, String operand, int minimum, int maximum) {
    return parseLong(description, operand, minimum, maximum).intValue();
  }

  public static Integer parseInt (String description, String operand, int maximum) {
    return parseInt(description, operand, 0, maximum);
  }

  public static Integer parseInt (String description, String operand) {
    return parseInt(description, operand, Integer.MAX_VALUE);
  }

  public static Short parseShort (String description, String operand, short minimum, short maximum) {
    return parseLong(description, operand, minimum, maximum).shortValue();
  }

  public static Short parseShort (String description, String operand, short maximum) {
    return parseShort(description, operand, (short)0, maximum);
  }

  public static Short parseShort (String description, String operand) {
    return parseShort(description, operand, Short.MAX_VALUE);
  }

  public static Byte parseByte (String description, String operand, byte minimum, byte maximum) {
    return parseLong(description, operand, minimum, maximum).byteValue();
  }

  public static Byte parseByte (String description, String operand, byte maximum) {
    return parseByte(description, operand, (byte)0, maximum);
  }

  public static Byte parseByte (String description, String operand) {
    return parseByte(description, operand, Byte.MAX_VALUE);
  }

  public static void show (String line) {
    System.out.println(line);
  }

  public static void show (String label, String value) {
    show((label + ": " + value));
  }
}

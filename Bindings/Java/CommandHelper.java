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

  public static Long parseLong (String description, String operand, Long minimum, Long maximum) {
    long value;

    try {
      value = Long.valueOf(operand);

      if (minimum != null) {
        if (value < minimum) {
          syntaxError(
            "%s less than %d: %s", description, minimum, operand
          );
        }
      }

      if (maximum != null) {
        if (value > maximum) {
          syntaxError(
            "%s greater than %d: %s", description, maximum, operand
          );
        }
      }

      return value;
    } catch (NumberFormatException exception) {
      syntaxError(
        String.format(
          "not an integer: %s: %s", description, operand
        )
      );
    }

    return null;
  }

  public static Long parseLong (String description, String operand, Long maximum) {
    return parseLong(description, operand, 0L, maximum);
  }

  public static Long parseLong (String description, String operand) {
    return parseLong(description, operand, Long.MAX_VALUE);
  }

  public static void show (String line) {
    System.out.println(line);
  }

  public static void show (String label, String value) {
    show((label + ": " + value));
  }
}

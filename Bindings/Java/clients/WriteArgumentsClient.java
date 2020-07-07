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

package org.a11y.brlapi.clients;
import org.a11y.brlapi.*;

public class WriteArgumentsClient extends PauseClient {
  public final static String NO_CURSOR = "no";
  public final static int MINIMUM_CURSOR_POSITION =  1;
  public final static int MAXIMUM_CURSOR_POSITION = 99;

  public final static String DEFAULT_DISPLAY = "default";
  public final static int MINIMUM_DISPLAY_NUMBER = 1;
  public final static int MAXIMUM_DISPLAY_NUMBER = 9;

  public WriteArgumentsClient (String... arguments) {
    super(arguments);
    addRepeatingParameter("text");

    addOption("cursor",
      (operands) -> {
        String operand = operands[0];
        int position;

        if (Strings.isAbbreviation(NO_CURSOR, operand)) {
          position = Constants.CURSOR_OFF;
        } else {
          position = Parse.asInt(
            "cursor position", operand,
            MINIMUM_CURSOR_POSITION, MAXIMUM_CURSOR_POSITION
          );
        }

        writeArguments.setCursorPosition(position);
      },
      "position"
    );

    addOption("display",
      (operands) -> {
        String operand = operands[0];
        int number;

        if (Strings.isAbbreviation(DEFAULT_DISPLAY, operand)) {
          number = Constants.DISPLAY_DEFAULT;
        } else {
          number = Parse.asInt(
            "display number", operand,
            MINIMUM_DISPLAY_NUMBER, MAXIMUM_DISPLAY_NUMBER
          );
        }

        writeArguments.setDisplayNumber(number);
      },
      "number"
    );
  }

  @Override
  protected final void extendUsageSummary (StringBuilder usage) {
    super.extendUsageSummary(usage);

    usage.append('\n')
         .append("The cursor position must be an integer")
         .append(" within the range ").append(MINIMUM_CURSOR_POSITION)
         .append(" through ").append(MAXIMUM_CURSOR_POSITION)
         .append(" or the word ").append(NO_CURSOR)
         .append(". ")

         .append("If not specified, ")
         .append(NO_CURSOR)
         .append(" is assumed. ")
         ;

    usage.append('\n')
         .append("The display number must be an integer")
         .append(" within the range ").append(MINIMUM_DISPLAY_NUMBER)
         .append(" through ").append(MAXIMUM_DISPLAY_NUMBER)
         .append(" or the word ").append(DEFAULT_DISPLAY)
         .append(". ")

         .append("If not specified, ")
         .append(DEFAULT_DISPLAY)
         .append(" is assumed. ")
         ;
  }

  private final WriteArguments writeArguments = new WriteArguments()
    .setCursorPosition(Constants.CURSOR_OFF)
    .setDisplayNumber(Constants.DISPLAY_DEFAULT)
    ;

  @Override
  protected final void processParameters (String[] parameters) {
    StringBuilder builder = new StringBuilder();

    for (String parameter : parameters) {
      if (builder.length() > 0) builder.append(' ');
      builder.append(parameter);
    }

    writeArguments.setText(builder.toString());
  }

  @Override
  protected final void runClient (Connection connection) 
            throws ProgramException
  {
    if (writeArguments.getRegionBegin() == 0) {
      writeArguments.setRegionBegin(1);
    }

    if (writeArguments.getRegionSize() == 0) {
      writeArguments.setRegionSize(writeArguments.getText().length());
    }

    ttyMode(
      connection, false,
      (tty) -> {
        printf("%s\n", writeArguments.getText());
        tty.write(writeArguments);
        pause(tty, getPauseTimeout());
      }
    );
  }
}

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
  private final WriteArguments writeArguments = new WriteArguments()
    .setCursorPosition(Constants.CURSOR_OFF)
    .setDisplayNumber(Constants.DISPLAY_DEFAULT)
    ;

  private final OperandUsage cursorPositionUsage =
     new CursorPositionUsage(writeArguments.getCursorPosition());

  private final OperandUsage displayNumberUsage =
      new DisplayNumberUsage(writeArguments.getDisplayNumber());

  public WriteArgumentsClient (String... arguments) {
    super(arguments);
    addRepeatingParameter("text");

    addOption("cursor",
      (operands) -> {
        writeArguments.setCursorPosition(Parse.asCursorPosition(operands[0]));
      },
      "position"
    );

    addOption("display",
      (operands) -> {
        writeArguments.setDisplayNumber(Parse.asDisplayNumber(operands[0]));
      },
      "number"
    );
  }

  @Override
  protected final void extendUsageSummary (StringBuilder usage) {
    super.extendUsageSummary(usage);

    cursorPositionUsage.appendTo(usage);
    displayNumberUsage.appendTo(usage);
  }

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
      (con) -> {
        printf("%s\n", writeArguments.getText());
        con.write(writeArguments);
        pause(con);
      }
    );
  }
}

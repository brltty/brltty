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

  public final static int MINIMUM_REGION_BEGIN =  1;
  public final static int MAXIMUM_REGION_BEGIN = 99;

  public final static int MINIMUM_REGION_SIZE =  1;
  public final static int MAXIMUM_REGION_SIZE = 99;

  public WriteArgumentsClient (String... arguments) {
    super(arguments);

    addOption("text",
      (operands) -> {
        writeArguments.setText(operands[0]);
      },
      "text"
    );

    addOption("region",
      (operands) -> {
        writeArguments.setRegionBegin(
          Parse.asInt(
            "region begin", operands[0],
            MINIMUM_REGION_BEGIN, MAXIMUM_REGION_BEGIN
          )
        );

        writeArguments.setRegionSize(
          Parse.asInt(
            "region Size", operands[1],
            MINIMUM_REGION_SIZE, MAXIMUM_REGION_SIZE
          )
        );
      },
      "begin", "size"
    );

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

    new OperandUsage("beginning of the region")
      .setRange(MINIMUM_REGION_BEGIN, MAXIMUM_REGION_BEGIN)
      .appendTo(usage);

    new OperandUsage("region size")
      .setRange(MINIMUM_REGION_SIZE, MAXIMUM_REGION_SIZE)
      .appendTo(usage);

    cursorPositionUsage.appendTo(usage);
    displayNumberUsage.appendTo(usage);
  }

  @Override
  protected final void runClient (Connection connection) 
            throws ProgramException
  {
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

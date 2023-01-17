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

package org.a11y.brlapi.clients;
import org.a11y.brlapi.*;

import java.util.List;
import java.util.LinkedList;

public class WriteArgumentsClient extends PauseClient {
  private final List<WriteArguments> writeArgumentsList = new LinkedList<>();
  private WriteArguments writeArguments = new WriteArguments();
  private boolean fixWriteArguments = true;

  private final OperandUsage cursorPositionUsage =
     new CursorPositionUsage(writeArguments.getCursorPosition());

  private final OperandUsage displayNumberUsage =
      new DisplayNumberUsage(writeArguments.getDisplayNumber());

  public final static int MINIMUM_REGION_BEGIN = 1;
  public final static int MINIMUM_REGION_SIZE = 1;

  public WriteArgumentsClient (String... arguments) {
    super(arguments);
    writeArgumentsList.add(writeArguments);

    addOption("text",
      (operands) -> {
        writeArguments.setText(operands[0]);
      },
      "text"
    );

    addOption("begin",
      (operands) -> {
        writeArguments.setRegionBegin(
          Parse.asInt(
            WriteArguments.REGION_BEGIN,
            operands[0], MINIMUM_REGION_BEGIN
          )
        );
      },
      "region begin"
    );

    addOption("length",
      (operands) -> {
        writeArguments.setRegionSize(
          Parse.asInt(
            WriteArguments.REGION_SIZE,
            operands[0], MINIMUM_REGION_SIZE
          )
        );
      },
      "region size"
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

    addOption("fix",
      (operands) -> {
        fixWriteArguments = true;
      }
    );

    addOption("nofix",
      (operands) -> {
        fixWriteArguments = false;
      }
    );

    addOption("render",
      (operands) -> {
        writeArguments = new WriteArguments();
        writeArgumentsList.add(writeArguments);
      }
    );
  }

  @Override
  protected final void extendUsageSummary (StringBuilder usage) {
    super.extendUsageSummary(usage);

    new OperandUsage(WriteArguments.REGION_BEGIN)
      .setDefault(MINIMUM_REGION_BEGIN)
      .setRangeMinimum(MINIMUM_REGION_BEGIN)
      .appendTo(usage);

    new OperandUsage(WriteArguments.REGION_SIZE)
      .setDefault(
        String.format(
          "the size of the largest content property (%s, %s, %s)",
          WriteArguments.TEXT, WriteArguments.AND_MASK, WriteArguments.OR_MASK
        )
       )
      .setRangeMinimum(MINIMUM_REGION_SIZE)
      .setRangeUnits("cells")
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
        int cellCount = con.getCellCount();

        for (WriteArguments arguments : writeArgumentsList) {
          try {
            arguments.check(cellCount, fixWriteArguments);
          } catch (IllegalStateException exception) {
            throw new SyntaxException(exception.getMessage());
          }

          con.write(arguments);
        }

        byte[] dots = con.getParameters().renderedCells.get();
        printf("%s\n", toUnicodeBraille(dots));

        pause(con);
      }
    );
  }
}

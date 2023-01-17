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

public class PauseClient extends Client {
  public final static byte MINIMUM_WAIT_TIME = 1;
  public final static byte DEFAULT_WAIT_TIME = 5;

  private byte waitTime = DEFAULT_WAIT_TIME;
  private final OperandUsage waitTimeUsage = new OperandUsage("wait time")
    .setDefault(waitTime)
    .setRangeMinimum(MINIMUM_WAIT_TIME)
    .setRangeUnits("seconds")
    ;

  protected final int getWaitTime () {
    return waitTime * 1000;
  }

  protected final boolean pause (Connection connection) {
    return pause(connection, getWaitTime());
  }

  public PauseClient (String... arguments) {
    super(arguments);
    // don't add parameters to this client because some other clients extend it

    addOption("wait",
      (operands) -> {
        waitTime = Parse.asByte(
          waitTimeUsage.getOperandDescription(),
          operands[0], MINIMUM_WAIT_TIME
        );
      },
      "seconds"
    );
  }

  @Override
  protected void extendUsageSummary (StringBuilder usage) {
    super.extendUsageSummary(usage);

    waitTimeUsage.appendTo(usage);
  }

  @Override
  protected void runClient (Connection connection) 
            throws ProgramException
  {
    printf("Pausing for %d seconds...", waitTime);
    String result;

    if (pause(connection)) {
      result = "pause completed";
    } else {
      result = "pause interrupted";
    }

    printf(" %s\n", result);
  }
}

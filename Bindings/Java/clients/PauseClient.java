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

public class PauseClient extends Client {
  public final static byte MINIMUM_PAUSE_TIMEOUT =  1;
  public final static byte DEFAULT_PAUSE_TIMEOUT = 10;
  public final static byte MAXIMUM_PAUSE_TIMEOUT = 30;

  private byte pauseTimeout = DEFAULT_PAUSE_TIMEOUT;

  protected final int getPauseTimeout () {
    return pauseTimeout * 1000;
  }

  public PauseClient (String... arguments) {
    super(arguments);
    // don't add parameters to this client because some other clients extend it

    addOption("timeout",
      (operands) -> {
        pauseTimeout = Parse.asByte(
          "pause timeout", operands[0],
          MINIMUM_PAUSE_TIMEOUT, MAXIMUM_PAUSE_TIMEOUT
        );
      },
      "number of seconds"
    );
  }

  @Override
  protected final void extendUsageSummary (StringBuilder usage) {
    super.extendUsageSummary(usage);

    usage.append('\n')
         .append("The pause timeout must be an integer number of seconds")
         .append(" within the range ").append(MINIMUM_PAUSE_TIMEOUT)
         .append(" through ").append(MAXIMUM_PAUSE_TIMEOUT)
         .append(". ")

         .append("If not specified, ")
         .append(DEFAULT_PAUSE_TIMEOUT)
         .append(" is assumed. ")
         ;
  }

  @Override
  protected void runClient (Connection connection) 
            throws ProgramException
  {
    printf("Pausing for %d seconds...", pauseTimeout);
    String result;

    if (pause(connection, getPauseTimeout())) {
      result = "timed out";
    } else {
      result = "event occurred";
    }

    printf(" %s\n", result);
  }
}

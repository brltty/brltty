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

import java.io.InterruptedIOException;

public class PauseClient extends Client {
  public final static byte MINIMUM_PAUSE_TIMEOUT =  1;
  public final static byte DEFAULT_PAUSE_TIMEOUT = 10;
  public final static byte MAXIMUM_PAUSE_TIMEOUT = 30;

  private byte pauseTimeout = DEFAULT_PAUSE_TIMEOUT;

  public PauseClient (String... arguments) {
    super(arguments);

    addOption("timeout",
      (operands) -> {
        pauseTimeout = Parse.asByte(
          "pause timeout", operands[0],
          MINIMUM_PAUSE_TIMEOUT, MAXIMUM_PAUSE_TIMEOUT
        );
      },
      "sconds"
    );
  }

  @Override
  protected final void extendUsageSummary (StringBuilder usage) {
    usage.append("The timeout defaults to ");
    usage.append(DEFAULT_PAUSE_TIMEOUT);
    usage.append(" and must be within the range ");
    usage.append(MINIMUM_PAUSE_TIMEOUT);
    usage.append(" through ");
    usage.append(MAXIMUM_PAUSE_TIMEOUT);
    usage.append('.');
  }

  @Override
  protected final void runClient (Connection connection) {
    printf("Pausing for %d seconds...", pauseTimeout);
    String result;

    try {
      connection.pause(pauseTimeout * 1000);
      result = "timed out";
    } catch (InterruptedIOException exception) {
      result = "event occurred";
    }

    printf(" %s\n", result);
  }
}

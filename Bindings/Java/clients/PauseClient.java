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
  private int pauseTimeout = 10;

  public PauseClient (String... arguments) {
    super(arguments);

    addOption("timeout",
      (operands) -> {
        pauseTimeout = Parse.asInt("pause timeout", operands[0], 1, 30);
      },
      "sconds"
    );
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

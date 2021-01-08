/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2021 by
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

public class ApiErrorClient extends Client {
  public ApiErrorClient (String... arguments) {
    super(arguments);
  }

  @Override
  protected final void runClient (Connection connection) {
    String result = null;

    try {
      connection.setParameter(
        Constants.PARAM_SERVER_VERSION, 0, true, new int[] {0}
      );

      result = "not thrown";
    } catch (APIError error) {
      result = "test succeeded";
    }

    printf("API Error: %s\n", result);
  }
}

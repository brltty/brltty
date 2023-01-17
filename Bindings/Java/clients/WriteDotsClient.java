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

public class WriteDotsClient extends PauseClient {
  public WriteDotsClient (String... arguments) {
    super(arguments);
    addRepeatingParameter("dots");
  }

  private byte[] dots = null;

  @Override
  protected final void processParameters (String[] parameters)
            throws SyntaxException
  {
    int count = parameters.length;
    dots = new byte[count];

    for (int index=0; index<count; index+=1) {
      dots[index] = Parse.asDots("dots parameter", parameters[index]);
    }
  }

  @Override
  protected final void runClient (Connection connection) 
            throws ProgramException
  {
    ttyMode(
      connection, false,
      (con) -> {
        con.write(dots);

        byte[] dots = con.getParameters().renderedCells.get();
        printf("%s\n", toUnicodeBraille(dots));

        pause(con);
      }
    );
  }
}

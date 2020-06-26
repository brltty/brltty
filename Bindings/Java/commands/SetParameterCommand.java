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

package org.a11y.brlapi.commands;
import org.a11y.brlapi.*;

public class SetParameterCommand extends Command {
  public SetParameterCommand (String[] arguments) {
    super(arguments);
  }

  private String parameterName;
  private String parameterValue;

  @Override
  protected void processParameters (String[] parameters) {
    switch (parameters.length) {
      case 0:
        syntaxError("missing parameter name");
        return;

      case 1:
        syntaxError ("missing larameter value");
        return;

      case 2:
        parameterName = parameters[0];
        parameterValue = parameters[1];
        return;
    }

    tooManyParameters();
  }

  @Override
  public final void run () {
    connect(
      new Client() {
        @Override
        public void run (Connection connection) {
          Parameter parameter = connection.getParameters().get(parameterName);

          if (parameter == null) {
            syntaxError("unknown parameter: %s", parameterName);
          }

          try {
            parameter.set(parameterValue);
          } catch (OperandException exception) {
            syntaxError(exception.getMessage());
          }
        }
      }
    );
  }
}

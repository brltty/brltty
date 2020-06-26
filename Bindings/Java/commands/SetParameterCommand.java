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

          if (!(parameter instanceof Parameter.Settable)) {
            syntaxError("parameter not settable: %s", parameterName);
          }

          if (parameter instanceof Parameter.StringSettable) {
            Parameter.StringSettable settable = (Parameter.StringSettable)parameter;
            settable.set(parameterValue);
            return;
          }

          if (parameter instanceof Parameter.BooleanSettable) {
            Parameter.BooleanSettable settable = (Parameter.BooleanSettable)parameter;
            settable.set(parseBoolean(parameterName, parameterValue));
            return;
          }

          if (parameter instanceof Parameter.ByteSettable) {
            Parameter.ByteSettable settable = (Parameter.ByteSettable)parameter;
            settable.set(parseByte(parameterName, parameterValue, settable.getMinimum(), settable.getMaximum()));
            return;
          }

          if (parameter instanceof Parameter.ShortSettable) {
            Parameter.ShortSettable settable = (Parameter.ShortSettable)parameter;
            settable.set(parseShort(parameterName, parameterValue, settable.getMinimum(), settable.getMaximum()));
            return;
          }

          if (parameter instanceof Parameter.IntSettable) {
            Parameter.IntSettable settable = (Parameter.IntSettable)parameter;
            settable.set(parseInt(parameterName, parameterValue, settable.getMinimum(), settable.getMaximum()));
            return;
          }

          if (parameter instanceof Parameter.LongSettable) {
            Parameter.LongSettable settable = (Parameter.LongSettable)parameter;
            settable.set(parseLong(parameterName, parameterValue, settable.getMinimum(), settable.getMaximum()));
            return;
          }
        }
      }
    );
  }
}

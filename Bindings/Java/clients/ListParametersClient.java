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

public class ListParametersClient extends Client {
  public ListParametersClient (String[] arguments) {
    super(arguments);
  }

  private String parameterName;
  private Long subparamValue;

  @Override
  protected void processParameters (String[] parameters)
            throws OperandException
  {
    parameterName = null;
    subparamValue = null;

    switch (parameters.length) {
      case 2:
        subparamValue = Parse.asLong("subparam", parameters[1]);
        /* fall through */

      case 1:
        parameterName = parameters[0];
        /* fall through */

      case 0:
        return;
    }

    tooManyParameters();
  }

  @Override
  protected final void runProgram () {
    connect(
      new ClientTask() {
        @Override
        public void run (Connection connection) {
          if (parameterName == null) {
            Parameter[] parameterArray = connection.getParameters().get();
            Parameters.sortByName(parameterArray);

            for (Parameter parameter : parameterArray) {
              if (parameter.isHidable()) continue;
              String value = parameter.toString();
              if (value != null) show(parameter.getLabel(), value);
            }
          } else {
            Parameter parameter = getParameter(connection, parameterName);
            String value;

            if (subparamValue == null) {
              value = parameter.toString();
            } else {
              long subparam = subparamValue;
              value = parameter.toString(subparam);
            }

            if (value == null) {
              StringBuilder message = new StringBuilder();
              message.append("no value");

              message.append(": ");
              message.append(parameterName);

              if (subparamValue != null) {
                message.append('[');
                message.append(subparamValue);
                message.append(']');
              }

              semanticError("%s", message);
            }

            show(value);
          }
        }
      }
    );
  }
}

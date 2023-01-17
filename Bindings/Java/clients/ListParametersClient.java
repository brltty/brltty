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

public class ListParametersClient extends Client {
  public ListParametersClient (String... arguments) {
    super(arguments);
    addOptionalParameters("parameter", "subparam");
  }

  private String parameterName = null;
  private Long subparamValue = null;

  @Override
  protected void processParameters (String[] parameters)
            throws SyntaxException
  {
    int count = parameters.length;
    int index = 0;

    if (index == count) return;
    parameterName = parameters[index++];

    if (index == count) return;
    subparamValue = Parse.asLong("subparam", parameters[index++]);

    if (index < count) throw new TooManyParametersException(parameters, index);
  }

  @Override
  protected final void runClient (Connection connection) throws SemanticException {
    if (parameterName == null) {
      Parameter[] parameterArray = connection.getParameters().get();
      Parameters.sortByName(parameterArray);

      for (Parameter parameter : parameterArray) {
        if (parameter.isHidable()) continue;
        String value = parameter.toString();
        if (value != null) printf("%s: %s\n", parameter.getLabel(), value);
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
        message.append("parameter has no value");

        message.append(": ");
        message.append(parameter.getName());

        if (subparamValue != null) {
          message.append('[');
          message.append(subparamValue);
          message.append(']');
        }

        throw new SemanticException("%s", message);
      }

      printf("%s\n", value);
    }
  }
}

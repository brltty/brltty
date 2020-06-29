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

package org.a11y.brlapi;

public abstract class Client extends Program {
  private final ConnectionSettings connectionSettings = new ConnectionSettings();

  public final Client setServerHost (String host) {
    connectionSettings.setServerHost(host);
    return this;
  }

  public final Client setAuthorizationSchemes (String schemes) {
    connectionSettings.setAuthorizationSchemes(schemes);
    return this;
  }

  protected Client (String... arguments) {
    super(arguments);

    addOption("server",
      (operands) -> {
        setServerHost(operands[0]);
      },
      "host specification"
    );

    addOption("authorization",
      (operands) -> {
        setAuthorizationSchemes(operands[0]);
      },
      "authorization scheme(s)"
    );
  }

  protected interface ClientTask {
    public void run (Connection connection);
  }

  private final void connect (ClientTask task) {
    try {
      Connection connection = new Connection(connectionSettings);

      try {
        task.run(connection);
      } finally {
        connection.close();
        connection = null;
      }
    } catch (ConnectionError error) {
      internalError(("connection error: " + error));
    }
  }

  protected abstract void runClient (Connection connection);

  @Override
  protected final void runProgram () {
    connect(
      (connection) -> {
        runClient(connection);
      }
    );
  }

  protected final void ttyMode (Connection connection, String driver, int[] path, ClientTask task) {
    try {
      connection.enterTtyModeWithPath(driver, path);

      try {
        task.run(connection);
      } finally {
        connection.leaveTtyMode();
      }
    } catch (ConnectionError error) {
      internalError(("tty mode error: " + error));
    }
  }

  protected final void ttyMode (Connection connection, boolean keys, int[] path, ClientTask task) {
    ttyMode(connection, (keys? connection.getDriverName(): null), path, task);
  }

  protected final void rawMode (Connection connection, String driver, ClientTask task) {
    try {
      connection.enterRawMode(driver);

      try {
        task.run(connection);
      } finally {
        connection.leaveRawMode();
      }
    } catch (ConnectionError error) {
      internalError(("raw mode error: " + error));
    }
  }

  protected final void rawMode (Connection connection, ClientTask task) {
    rawMode(connection, connection.getDriverName(), task);
  }

  protected final Parameter getParameter (Connection connection, String name) {
    Parameter parameter = connection.getParameters().get(name);

    if (parameter == null) {
      semanticError("unknown parameter: %s", name);
    }

    return parameter;
  }
}

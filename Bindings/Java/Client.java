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
    public void run (Connection connection) throws OperandException;
  }

  private final void connect (ClientTask task) throws OperandException {
    try {
      Connection connection = new Connection(connectionSettings);

      try {
        task.run(connection);
      } finally {
        connection.close();
        connection = null;
      }
    } catch (ConnectionError error) {
      onInternalError(("connection error: " + error));
    }
  }

  protected abstract void runClient (Connection connection)
            throws OperandException;

  @Override
  protected final void runProgram () throws OperandException {
    connect(
      (connection) -> {
        runClient(connection);
      }
    );
  }

  protected final Parameter getParameter (Connection connection, String name) {
    Parameter parameter = connection.getParameters().get(name);

    if (parameter == null) {
      onSemanticError("unknown parameter: %s", name);
    }

    return parameter;
  }

  protected interface TtyModeTask {
    public void run (Connection connection);
  }

  protected final void ttyMode (Connection connection, String driver, int[] path, TtyModeTask task) {
    try {
      connection.enterTtyModeWithPath(driver, path);

      try {
        task.run(connection);
      } finally {
        connection.leaveTtyMode();
      }
    } catch (ConnectionError error) {
      onInternalError(("tty mode error: " + error));
    }
  }

  protected final void ttyMode (Connection connection, boolean keys, int[] path, TtyModeTask task) {
    ttyMode(connection, (keys? connection.getDriverName(): null), path, task);
  }

  protected interface RawModeTask {
    public void run (Connection connection);
  }

  protected final void rawMode (Connection connection, String driver, RawModeTask task) {
    try {
      connection.enterRawMode(driver);

      try {
        task.run(connection);
      } finally {
        connection.leaveRawMode();
      }
    } catch (ConnectionError error) {
      onInternalError(("raw mode error: " + error));
    }
  }

  protected final void rawMode (Connection connection, RawModeTask task) {
    rawMode(connection, connection.getDriverName(), task);
  }
}

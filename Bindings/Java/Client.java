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
  protected abstract void runClient (Connection connection)
            throws ProgramException;

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
      "schemes"
    );
  }

  @Override
  protected void extendUsageSummary (StringBuilder usage) {
    super.extendUsageSummary(usage);

    usage.append('\n')
         .append("The default server host is ")
         .append(ConnectionSettings.DEFAULT_SERVER_HOST)
         .append('.')
         ;
  }

  protected interface ClientTask {
    public void run (Connection connection) throws ProgramException;
  }

  private final Client connect (ClientTask task) throws ProgramException {
    try {
      Connection connection = new Connection(connectionSettings);

      try {
        task.run(connection);
      } finally {
        connection.close();
        connection = null;
      }
    } catch (ConnectionError error) {
      throw new ProgramException(("connection error: " + error));
    }

    return this;
  }

  @Override
  protected final void runProgram () throws ProgramException {
    connect(
      (connection) -> {
        runClient(connection);
      }
    );
  }

  protected final Parameter getParameter (Connection connection, String name)
            throws SemanticException
  {
    Parameter parameter = connection.getParameters().get(name);
    if (parameter != null) return parameter;
    throw new SemanticException("unknown parameter: %s", name);
  }

  protected interface TtyModeTask {
    public void run (Connection connection);
  }

  protected final Client ttyMode (Connection connection, String driver, TtyModeTask task, int... path)
            throws ProgramException
  {
    try {
      connection.enterTtyModeWithPath(driver, path);

      try {
        task.run(connection);
      } finally {
        connection.leaveTtyMode();
      }
    } catch (ConnectionError error) {
      throw new ProgramException(("tty mode error: " + error));
    }

    return this;
  }

  protected final Client ttyMode (Connection connection, boolean keys, TtyModeTask task, int... path)
            throws ProgramException
  {
    return ttyMode(connection, (keys? connection.getDriverName(): null), task, path);
  }

  protected interface RawModeTask {
    public void run (Connection connection);
  }

  protected final Client rawMode (Connection connection, String driver, RawModeTask task)
            throws ProgramException
  {
    try {
      connection.enterRawMode(driver);

      try {
        task.run(connection);
      } finally {
        connection.leaveRawMode();
      }
    } catch (ConnectionError error) {
      throw new ProgramException(("raw mode error: " + error));
    }

    return this;
  }

  protected final Client rawMode (Connection connection, RawModeTask task)
            throws ProgramException
  {
    return rawMode(connection, connection.getDriverName(), task);
  }
}

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

package org.a11y.brlapi;

import java.io.InterruptedIOException;

public abstract class Client extends Program {
  protected abstract void runClient (Connection connection)
            throws ProgramException;

  private final ConnectionSettings connectionSettings = new ConnectionSettings();

  public final Client setServerHost (String host) throws SyntaxException {
    try {
      connectionSettings.setServerHost(host);
    } catch (IllegalArgumentException exception) {
      throw new SyntaxException(exception.getMessage());
    }

    return this;
  }

  public final Client setAuthenticationScheme (String scheme) throws SyntaxException {
    try {
      connectionSettings.setAuthenticationScheme(scheme);
    } catch (IllegalArgumentException exception) {
      throw new SyntaxException(exception.getMessage());
    }

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

    addOption("authentication",
      (operands) -> {
        setAuthenticationScheme(operands[0]);
      },
      "scheme"
    );
  }

  @Override
  protected void extendUsageSummary (StringBuilder usage) {
    super.extendUsageSummary(usage);

    usage.append('\n')
         .append("The default server host is ")
         .append(ConnectionSettings.DEFAULT_SERVER_HOST)
         .append(". ")

         .append("The format of a host specification is ")
         .append(USAGE_OPTIONAL_BEGIN)
         .append("host")
         .append(USAGE_OPTIONAL_END)
         .append(USAGE_OPTIONAL_BEGIN)
         .append(ConnectionSettings.HOST_PORT_SEPARATOR).append("port")
         .append(USAGE_OPTIONAL_END)
         .append(". ")

         .append("The host component may be either a host name or an IPV4 address")
         .append(" - if not specified, a local socket is used. ")

         .append("The port component must be an integer within the range ")
         .append(ConnectionSettings.MINIMUM_PORT_NUMBER)
         .append(" through ")
         .append(ConnectionSettings.MAXIMUM_PORT_NUMBER)
         .append(" - if not specified, ")
         .append(ConnectionSettings.DEFAULT_PORT_NUMBER)
         .append(" is assumed. ")
         ;

    usage.append('\n')
         .append("The default authentication scheme is ")
         .append(ConnectionSettings.DEFAULT_AUTHENTICATION_SCHEME)
         .append(". ")

         .append("The format of a scheme specification is name")
         .append(USAGE_OPTIONAL_BEGIN)
         .append(ConnectionSettings.AUTHENTICATION_OPERAND_PREFIX).append("operand")
         .append(USAGE_OPTIONAL_END)
         .append(". ")

         .append("More than one scheme, separated by ")
         .append(ConnectionSettings.AUTHENTICATION_SCHEME_SEPARATOR)
         .append(", may be specified. ")

         .append("The following schemes are supported:")
         .append("\n  ").append(ConnectionSettings.AUTHENTICATION_SCHEME_KEYFILE)
         .append(ConnectionSettings.AUTHENTICATION_OPERAND_PREFIX).append("path")
         .append("\n  ").append(ConnectionSettings.AUTHENTICATION_SCHEME_NONE)
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
    } catch (LostConnectionException exception) {
      throw new ExternalException("connection lost");
    } catch (APIError error) {
      throw new ExternalException(("API error: " + error));
    } catch (APIException exception) {
      throw new ExternalException(("API exception: " + exception));
    }

    return this;
  }

  public final boolean pause (Connection connection, int milliseconds) {
    try {
      connection.pause(milliseconds);
      return true;
    } catch (InterruptedIOException exception) {
      return false;
    }
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
    public void run (Connection connection) throws ProgramException;
  }

  protected final Client ttyMode (Connection connection, String driver, TtyModeTask task, int... path)
            throws ProgramException
  {
    try {
      connection.enterTtyModeWithPath(driver, path);

      try {
        task.run(connection);
      } finally {
        if (!connection.isUnusable()) connection.leaveTtyMode();
      }
    } catch (APIError error) {
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
    public void run (Connection connection) throws ProgramException;
  }

  protected final Client rawMode (Connection connection, String driver, RawModeTask task)
            throws ProgramException
  {
    try {
      connection.enterRawMode(driver);

      try {
        task.run(connection);
      } finally {
        if (!connection.isUnusable()) connection.leaveRawMode();
      }
    } catch (APIError error) {
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

/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2019 by
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

import java.io.PrintStream;

public class TestProgram implements Constants {
  private static void writeProperty (String name, String format, Object... values) {
    PrintStream stream = System.out;

    stream.print(name);
    stream.print(": ");
    stream.print(String.format(format, values));
    stream.println();
  }

  private static void showKey (Key key, Connection connection) {
    String text = String.format(
      "code=0X%X type=%X cmd=%X arg=%X flg=%X",
      key.getKeyCode(),
      key.getTypeValue(),
      key.getCommandValue(),
      key.getArgumentValue(),
      key.getFlagsValue()
    );

    writeProperty("Key", text);
    if (connection != null) connection.writeText(text);
  }

  private static void showKey (Key key) {
    showKey(key, null);
  }

  private static void syntaxError (String message) {
    System.err.println(message);
    System.exit(2);
  }

  public static void main (String arguments[]) {
    ConnectionSettings settings = new ConnectionSettings();

    {
      int argumentCount = arguments.length;
      int argumentIndex = 0;

      while (argumentIndex < argumentCount) {
        String argument = arguments[argumentIndex++];

        if (argument.equals("-host")) {
          if (argumentIndex == argumentCount) {
            syntaxError("missing host specification");
          }

          settings.setServerHost(arguments[argumentIndex++]);
          continue;
        }

        syntaxError("unrecognized option: " + argument);
      }
    }

    writeProperty("Library Version", "%d.%d.%d",
      LibraryVersion.getMajor(),
      LibraryVersion.getMinor(),
      LibraryVersion.getRevision()
    );

    try {
      Connection connection = new Connection(settings);
      try {
        writeProperty("File Descriptor", "%d", connection.getFileDescriptor());
        writeProperty("Server Host", "%s", connection.getServerHost());
        writeProperty("Authorization Schemes", "%s", connection.getAuthorizationSchemes());
        writeProperty("Driver Name", "%s", connection.getDriverName());
        writeProperty("Model Identifier", "%s", connection.getModelIdentifier());

        DisplaySize size = connection.getDisplaySize();
        writeProperty("Display Size",  "%dx%d", size.getWidth(), size.getHeight());

        int tty = connection.enterTtyMode();
        try {
          writeProperty("TTY Number", "%d", tty);

          long key[] = {0};
          connection.ignoreKeys(Constants.rangeType_all, key);
          key[0] = Constants.KEY_TYPE_CMD;
          connection.acceptKeys(Constants.rangeType_type, key);
          long keys[][] = {{0,2},{5,7}};
          connection.ignoreKeyRanges(keys);

          {
            int timeout = 10;
            connection.writeText(String.format("press keys (timeout is %d seconds)", timeout), Constants.CURSOR_OFF);

            while (true) {
              long code = connection.readKeyWithTimeout((timeout * 1000));
              if (code < 0) break;
              showKey(new Key(code), connection);
            }
          }
        } finally {
          connection.leaveTtyMode();
        }
      } finally {
        connection.closeConnection();
        connection = null;
      }
    } catch (Error error) {
      System.out.println("got error: " + error);
      System.exit(3);
    }
  }
}

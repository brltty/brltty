/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2018 by
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
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.BrlAPI;

import java.io.PrintStream;

public class Test implements Constants {
  private static void writeProperty (String name, String format, Object... values) {
    PrintStream stream = System.out;

    stream.print(name);
    stream.print(": ");
    stream.print(String.format(format, values));
    stream.println();
  }

  private static void showKey (Key key, Brlapi brlapi) {
    String text = String.format(
      "code=0X%X type=%X cmd=%X arg=%X flg=%X",
      key.getCode(), key.getType(), key.getCommand(), key.getArgument(), key.getFlags()
    );

    writeProperty("Key", text);
    if (brlapi != null) brlapi.writeText(text);
  }

  private static void showKey (Key key) {
    showKey(key, null);
  }

  public static void main (String argv[]) {
    ConnectionSettings settings = new ConnectionSettings();

    {
      int argi = 0;
      while (argi < argv.length) {
        String arg = argv[argi++];

        if (arg.equals("-host")) {
          if (argi == argv.length) {
            System.err.println("Missing host specification.");
            System.exit(2);
          }

          settings.host = argv[argi++];
          continue;
        }

        System.err.println("Invalid option: " + arg);
        System.exit(2);
      }
    }

    writeProperty("BrlAPI Version", "%d.%d.%d",
      Native.getMajorVersion(),
      Native.getMinorVersion(),
      Native.getRevision()
    );

    try {
      Brlapi brlapi = new Brlapi(settings);

      writeProperty("File Descriptor", "%d", brlapi.getFileDescriptor());
      writeProperty("Server Host", "%s", brlapi.getServerHost());
      writeProperty("Authorization Schemes", "%s", brlapi.getAuthorizationSchemes());
      writeProperty("Driver Name", "%s", brlapi.getDriverName());
      writeProperty("Model Identifier", "%s", brlapi.getModelIdentifier());

      DisplaySize size = brlapi.getDisplaySize();
      writeProperty("Display Size",  "%dx%d", size.getWidth(), size.getHeight());

      int tty = brlapi.enterTtyMode();
      writeProperty("TTY Number", "%d", tty);

      long key[] = {0};
      brlapi.ignoreKeys(Brlapi.rangeType_all, key);
      key[0] = Constants.KEY_TYPE_CMD;
      brlapi.acceptKeys(Brlapi.rangeType_type, key);
      long keys[][] = {{0,2},{5,7}};
      brlapi.ignoreKeyRanges(keys);

      {
        int timeout = 10;
        brlapi.writeText(String.format("press keys (timeout is %d seconds)", timeout), Brlapi.CURSOR_OFF);

        while (true) {
          long code = brlapi.readKeyWithTimeout((timeout * 1000));
          if (code < 0) break;
          showKey(new Key(code), brlapi);
        }
      }

      brlapi.leaveTtyMode();
      brlapi.closeConnection();
    } catch (Error error) {
      System.out.println("got error: " + error);
      System.exit(3);
    }
  }
}

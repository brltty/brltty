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

      brlapi.writeText("ok !! €", Brlapi.CURSOR_OFF);
      brlapi.writeText(null, 1);

      long key[] = {0};
      brlapi.ignoreKeys(Brlapi.rangeType_all, key);
      key[0] = Constants.KEY_TYPE_CMD;
      brlapi.acceptKeys(Brlapi.rangeType_type, key);
      long keys[][] = {{0,2},{5,7}};
      brlapi.ignoreKeyRanges(keys);

      printKey(new Key(brlapi.readKey(true)));

      {
        WriteArguments args = new WriteArguments();
        args.regionBegin = 10;
        args.regionSize = 20;
        args.text = "Key Pressed €       ";
        args.andMask = "????????????????????".getBytes();
        args.cursor = 3;
        brlapi.write(args);
      }

      printKey(new Key(brlapi.readKey(true)));

      {
        byte[] dots = new byte[] {
          /* o */ DOT1 | DOT3 | DOT5,
          /* t */ DOT2 | DOT3 | DOT4 | DOT5,
          /* h */ DOT1 | DOT2 | DOT5,
          /* e */ DOT1 | DOT5,
          /* r */ DOT1 | DOT2 | DOT3 | DOT5,
          /*   */ 0,
          /* k */ DOT1 | DOT3,
          /* e */ DOT1 | DOT5,
          /* y */ DOT1 | DOT3 | DOT4 | DOT5 | DOT6,
          /*   */ 0,
          /* p */ DOT1 | DOT2 | DOT3 | DOT4,
          /* r */ DOT1 | DOT2 | DOT3 | DOT5,
          /* e */ DOT1 | DOT5,
          /* s */ DOT2 | DOT3 | DOT4,
          /* s */ DOT2 | DOT3 | DOT4,
          /* e */ DOT1 | DOT5,
          /* d */ DOT1 | DOT4 | DOT5
        };

        brlapi.writeDots(dots);
      }

      printKey(new Key(brlapi.readKey(true)));

      brlapi.leaveTtyMode();
      brlapi.closeConnection();
    } catch (Error error) {
      System.out.println("got error: " + error);
      System.exit(3);
    }
  }

  private static void printKey (Key key) {
    System.out.println("got key " + Long.toHexString(key.getCode()) + " (" +
                       Integer.toHexString(key.getType()) + "," +
                       Integer.toHexString(key.getCommand()) + "," +
                       Integer.toHexString(key.getArgument()) + "," +
                       Integer.toHexString(key.getFlags()) + ")");
  }
}

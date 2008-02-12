/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2008 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 * Please see the file COPYING-API for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.BrlAPI;

public class Test {
  public static void main(String argv[]) {
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

    try {
      System.loadLibrary("brlapi_java");

      System.out.print("Connecting to BrlAPI... ");
      Brlapi brlapi = new Brlapi(settings);
      System.out.println("done (fd=" + brlapi.getFileDescriptor() + ")");

      System.out.print("Connected to " + brlapi.getHost());
      System.out.print(" using key file " + brlapi.getAuth());
      System.out.println();

      System.out.print("Driver is " + brlapi.getDriverName());
      System.out.println();

      DisplaySize size = brlapi.getDisplaySize();
      System.out.println("Display size is " + size.getWidth() + "x" + size.getHeight());

      int tty = brlapi.enterTtyMode();
      System.out.println("TTY is " + tty);

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

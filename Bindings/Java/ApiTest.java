/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2006 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 * All rights reserved.
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

public class ApiTest {
  public static void main(String argv[]) {
    try {
      BrlapiSize size;
      int tty;
      BrlapiKey key;
      long keylong;
      BrlapiSettings settings = new BrlapiSettings();
      BrlapiWriteStruct ws2 = new BrlapiWriteStruct(-1, 10, 20,
	  "Key Pressed         ",
	  "????????????????????".getBytes(),
	  null,3);
      System.out.print("Connecting to BrlAPI... ");
      System.loadLibrary("brlapi_java");
      Brlapi brlapi = new Brlapi(settings);
      size = brlapi.getDisplaySize();
      System.out.println("done (fd="+brlapi.getFileDescriptor()+")");
      System.out.println("Connected to "+brlapi.getHostName()+" using key file "+brlapi.getAuthKey());
      System.out.println("driver "+brlapi.getDriverId()+" "+brlapi.getDriverName());
      System.out.println("display "+size.x()+"x"+size.y());
      tty = brlapi.enterTtyMode(0,null);
      System.out.println("got tty "+tty);
      brlapi.writeText(0,"ok !!");
      keylong = brlapi.readKey(true);
      key = new BrlapiKey(keylong);
      System.out.println("got key "+keylong+": ("+key.command+","+key.argument+","+key.flags+")");
      brlapi.write(ws2);
      keylong = brlapi.readKey(true);
      key = new BrlapiKey(keylong);
      System.out.println("got key "+keylong+": ("+key.command+","+key.argument+","+key.flags+")");
      brlapi.leaveTtyMode();
      brlapi.closeConnection();
    } catch (BrlapiError e) {
      System.out.println("got error " + e);
    }
  }
}

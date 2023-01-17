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

package org.a11y.brlapi;

public class APIException extends RuntimeException {
  @Override
  public native String toString ();

  private final long connectionHandle;
  private final ConnectionBase connection;

  private final int errorNumber;
  private final int packetType;
  private final byte[] failedPacket;

  public APIException (long handle, int error, int type, byte[] packet) {
    super();

    Connection.setUnusable(handle);
    connectionHandle = handle;
    connection = ConnectionBase.getConnection(handle);

    errorNumber = error;
    packetType = type;
    failedPacket = packet;
  }

  public final int getErrorNumber () {
    return errorNumber;
  }

  public final int getPacketType () {
    return packetType;
  }

  public final byte[] getFailedPacket () {
    return failedPacket;
  }

  public native static String getPacketTypeName (int type);
}

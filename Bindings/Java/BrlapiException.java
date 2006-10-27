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

public class BrlapiException extends Exception {
  static final long serialVersionUID = 0;
  int errno;
  long packettype;
  byte buf[];
  public final native String toString();
  public BrlapiException(int errno, int packettype, byte buf[]) {
    this.errno = errno;
    this.packettype = packettype;
    this.buf = buf;
  }
}

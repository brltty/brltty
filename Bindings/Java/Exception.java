/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2009 by
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
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.BrlAPI;

public class Exception extends java.lang.Exception {
  static final long serialVersionUID = 0;
  long handle;
  int errno;
  long packettype;
  byte buf[];
  public final native String toString ();
  public Exception (long handle, int errno, int packettype, byte buf[]) {
    this.handle = handle;
    this.errno = errno;
    this.packettype = packettype;
    this.buf = buf;
  }
}

/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2007 by
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

public class BrlapiError extends Exception {
  static final long serialVersionUID = 0;
  int brlerrno;
  int libcerrno;
  int gaierrno;
  String errfun;
  public final native String toString();
  public BrlapiError(int brlerrno, int libcerrno, int gaierrno, String errfun) {
    this.brlerrno = brlerrno;
    this.libcerrno = libcerrno;
    this.gaierrno = gaierrno;
    this.errfun = errfun;
  }

  public final static int SUCCESS             =  0; /* Success */
  public final static int NOMEM               =  1; /* Not enough memory */
  public final static int TTYBUSY             =  2; /* Already a connection running in this tty */
  public final static int DEVICEBUSY          =  3; /* Already a connection using RAW mode */
  public final static int UNKNOWN_INSTRUCTION =  4; /* Not implemented in protocol */
  public final static int ILLEGAL_INSTRUCTION =  5; /* Forbiden in current mode */
  public final static int INVALID_PARAMETER   =  6; /* Out of range or have no sense */
  public final static int INVALID_PACKET      =  7; /* Invalid size */
  public final static int CONNREFUSED         =  8; /* Connection refused */
  public final static int OPNOTSUPP           =  9; /* Operation not supported */
  public final static int GAIERR              = 10; /* Getaddrinfo error */
  public final static int LIBCERR             = 11; /* Libc error */
  public final static int UNKNOWNTTY          = 12; /* Couldn't find out the tty number */
  public final static int PROTOCOL_VERSION    = 13; /* Bad protocol version */
  public final static int EOF                 = 14; /* Unexpected end of file */
  public final static int EMPTYKEY            = 15; /* Too many levels of recursion */
  public final static int DRIVERERROR         = 16; /* Packet returned by driver too large */
}

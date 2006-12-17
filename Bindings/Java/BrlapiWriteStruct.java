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

public class BrlapiWriteStruct {
  public int displayNumber;
  public int regionBegin;
  public int regionSize;
  public String text;
  public byte attrAnd[];
  public byte attrOr[];
  public int cursor;

  public BrlapiWriteStruct (
    int displayNumber, int regionBegin, int regionSize,
    String text, byte attrAnd[], byte attrOr[], int cursor
  ) {
    this.displayNumber = displayNumber;
    this.regionBegin = regionBegin;
    this.regionSize = regionSize;
    this.text = text;
    this.attrAnd = attrAnd;
    this.attrOr = attrOr;
    this.cursor = cursor;
  }

  public BrlapiWriteStruct () {
    this(
      Brlapi.DISPLAY_DEFAULT, 0, 0,
      null, null, null, Brlapi.CURSOR_LEAVE
    ); 
  }
}

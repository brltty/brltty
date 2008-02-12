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

public class WriteArguments {
  public int displayNumber = Brlapi.DISPLAY_DEFAULT;
  public int regionBegin = 0;
  public int regionSize = 0;
  public String text = null;
  public byte andMask[] = null;
  public byte orMask[] = null;
  public int cursor = Brlapi.CURSOR_LEAVE;

  public WriteArguments () {
  }
}

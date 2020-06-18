/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2020 by
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

public class KeyCode extends Code {
  public native static boolean isPress (long code);
  public native static long getValue (long code);
  public native static int getGroup (long code);
  public native static int getNumber (long code);

  public KeyCode (long code) {
    super(code);
  }

  public final boolean isPress () {
    return isPress(getCode());
  }

  public final long getValue () {
    return getValue(getCode());
  }

  public final int getGroup () {
    return getGroup(getCode());
  }

  public final int getNumber () {
    return getNumber(getCode());
  }

  @Override
  public String toString () {
    return String.format(
      "%s %s Grp:%d Num:%d",
      super.toString(), (isPress()? "press": "release"), getGroup(), getNumber()
    );
  }
}

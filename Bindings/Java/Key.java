/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2019 by
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

public class Key extends NativeLibrary {
  private native void expandKeyCode (long code);

  private final long keyCode;
  private int typeValue;
  private int commandValue;
  private int argumentValue;
  private int flagsValue;

  public Key (long code) {
    keyCode = code;
    expandKeyCode(code);
  }

  public final long getKeyCode () {
    return keyCode;
  }

  public final int getTypeValue () {
    return typeValue;
  }

  public final int getCommandValue () {
    return commandValue;
  }

  public final int getArgumentValue () {
    return argumentValue;
  }

  public final int getFlagsValue () {
    return flagsValue;
  }
}

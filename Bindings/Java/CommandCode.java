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

public class CommandCode extends Code {
  private native void expandCode (long code);

  private int typeValue;
  private int commandValue;
  private int argumentValue;
  private int flagsValue;

  public CommandCode (long code) {
    super(code);
    expandCode(code);
  }

  public final int getType () {
    return typeValue;
  }

  public final int getCommand () {
    return commandValue;
  }

  public final int getArgument () {
    return argumentValue;
  }

  public final int getFlags () {
    return flagsValue;
  }

  @Override
  public String toString () {
    return String.format(
      "Code:%08X Type:%08X Cmd:%04X Arg:%04X Flg:%04X",
      getCode(), getType(), getCommand(), getArgument(), getFlags()
    );
  }
}

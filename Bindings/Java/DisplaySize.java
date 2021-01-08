/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2021 by
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

public class DisplaySize {
  private final int displayWidth;
  private final int displayHeight;

  public DisplaySize (int width, int height) {
    displayWidth = width;
    displayHeight = height; 
  }

  public DisplaySize (int[] dimensions) {
    this(dimensions[0], dimensions[1]);
  }

  public int getWidth () {
    return displayWidth;
  }

  public int getHeight () {
    return displayHeight;
  }

  @Override
  public String toString () {
    return String.format("%dx%d", getWidth(), getHeight());
  }
}

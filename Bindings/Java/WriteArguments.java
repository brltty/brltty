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

public class WriteArguments {
  private String text = null;
  private byte andMask[] = null;
  private byte orMask[] = null;
  private int regionBegin = 0;
  private int regionSize = 0;
  private int cursorPosition = Constants.CURSOR_LEAVE;
  private int displayNumber = Constants.DISPLAY_DEFAULT;

  public WriteArguments () {
  }

  public String getText () {
    return text;
  }

  public WriteArguments setText (String text) {
    this.text = text;
    return this;
  }

  public byte[] getAndMask () {
    return andMask;
  }

  public WriteArguments setAndMask (byte[] mask) {
    andMask = mask;
    return this;
  }

  public byte[] getOrMask () {
    return orMask;
  }

  public WriteArguments setOrMask (byte[] mask) {
    orMask = mask;
    return this;
  }

  public int getRegionBegin () {
    return regionBegin;
  }

  public WriteArguments setRegionBegin (int begin) {
    regionBegin = begin;
    return this;
  }

  public int getRegionSize () {
    return regionSize;
  }

  public WriteArguments setRegionSize (int size) {
    regionSize = size;
    return this;
  }

  public WriteArguments setRegion (int begin, int size) {
    setRegionBegin(begin);
    setRegionSize(size);
    return this;
  }

  public int getCursorPosition () {
    return cursorPosition;
  }

  public WriteArguments setCursorPosition (int position) {
    cursorPosition = position;
    return this;
  }

  public int getDisplayNumber () {
    return displayNumber;
  }

  public WriteArguments setDisplayNumber (int number) {
    displayNumber = number;
    return this;
  }
}

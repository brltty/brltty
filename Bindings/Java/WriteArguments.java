/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2023 by
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

import java.util.Arrays;

public class WriteArguments extends Component {
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

  public final static String TEXT = "text";
  public final static String AND_MASK = "and-mask";
  public final static String OR_MASK = "or-mask";
  public final static String REGION_BEGIN = "region begin";
  public final static String REGION_SIZE = "region size";
  public final static String CURSOR_POSITION = "cursor position";
  public final static String DISPLAY_NUMBER = "display number";

  private static void checkRange (String description, int value, Integer minimum, Integer maximum) {
    try {
      if (minimum != null) Parse.checkMinimum(description, value, minimum);
      if (maximum != null) Parse.checkMaximum(description, value, maximum);
    } catch (SyntaxException exception) {
      throw new IllegalStateException(exception.getMessage());
    }
  }

  public final void check (int cellCount, boolean fix) {
    boolean haveRegionBegin = regionBegin != 0;
    boolean haveRegionSize = regionSize != 0;
    boolean haveRegion = haveRegionBegin || haveRegionSize;

    boolean haveText = text != null;
    boolean haveAndMask = andMask != null;
    boolean haveOrMask = orMask != null;
    boolean haveContent = haveText || haveAndMask || haveOrMask;

    if (haveRegion || haveContent) {
      if (!haveContent) {
        throw new IllegalStateException(
          String.format(
            "region content (%s, %s, and/or %s) not specified",
            TEXT, AND_MASK, OR_MASK
          )
        );
      }

      if (!haveRegionBegin) {
        if (!fix) {
          throw new IllegalStateException(
            String.format(
              "%s not set", REGION_BEGIN
            )
          );
        }

        regionBegin = 1;
        haveRegion = haveRegionBegin = true;
      }

      if (!haveRegionSize) {
        if (!fix) {
          throw new IllegalStateException(
            String.format(
              "%s not set", REGION_SIZE
            )
          );
        }

        regionSize = 0;
        if (haveText) regionSize = Math.max(regionSize, text.length());
        if (haveAndMask) regionSize = Math.max(regionSize, andMask.length);
        if (haveOrMask) regionSize = Math.max(regionSize, orMask.length);
        haveRegion = haveRegionSize = true;
      }

      checkRange(REGION_BEGIN, regionBegin, 1, cellCount);
      checkRange(REGION_SIZE, regionSize, 1, null);

      {
        int maximum = cellCount + 1 - regionBegin;

        if (!fix) {
          checkRange(REGION_SIZE, regionSize, null, maximum);
        } else if (regionSize > maximum) {
          regionSize = maximum;
        }
      }

      if (haveText) {
        int textLength = text.length();

        if (textLength > regionSize) {
          if (!fix) {
            throw new IllegalStateException(
              String.format(
                "%s length is greater than %s: %d > %d",
                TEXT, REGION_SIZE,
                textLength, regionSize
              )
            );
          }

          text = text.substring(0, regionSize);
        } else if (textLength < regionSize) {
          if (!fix) {
            throw new IllegalStateException(
              String.format(
                "%s length is less than %s: %d < %d",
                TEXT, REGION_SIZE,
                textLength, regionSize
              )
            );
          }

          StringBuilder newText = new StringBuilder(text);
          while (newText.length() < regionSize) newText.append(' ');
          text = newText.toString();
        }
      }

      if (haveAndMask) {
        int andSize = andMask.length;

        if (andSize > regionSize) {
          if (!fix) {
            throw new IllegalStateException(
              String.format(
                "%s size is greater than %s: %d > %d",
                AND_MASK, REGION_SIZE,
                andSize, regionSize
              )
            );
          }

          byte[] newMask = new byte[regionSize];
          System.arraycopy(andMask, 0, newMask, 0, regionSize);
          andMask = newMask;
        } else if (andSize < regionSize) {
          if (!fix) {
            throw new IllegalStateException(
              String.format(
                "%s size is less than %s: %d < %d",
                AND_MASK, REGION_SIZE,
                andSize, regionSize
              )
            );
          }

          byte[] newMask = new byte[regionSize];
          System.arraycopy(andMask, 0, newMask, 0, andSize);
          Arrays.fill(newMask, andSize, (regionSize - andSize), (byte)BYTE_MASK);
          andMask = newMask;
        }
      }

      if (haveOrMask) {
        int orSize = orMask.length;

        if (orSize > regionSize) {
          if (!fix) {
            throw new IllegalStateException(
              String.format(
                "%s size is greater than %s: %d > %d",
                OR_MASK, REGION_SIZE,
                orSize, regionSize
              )
            );
          }

          byte[] newMask = new byte[regionSize];
          System.arraycopy(orMask, 0, newMask, 0, regionSize);
          orMask = newMask;
        } else if (orSize < regionSize) {
          if (!fix) {
            throw new IllegalStateException(
              String.format(
                "%s size is less than %s: %d < %d",
                OR_MASK, REGION_SIZE,
                orSize, regionSize
              )
            );
          }

          byte[] newMask = new byte[regionSize];
          System.arraycopy(orMask, 0, newMask, 0, orSize);
          Arrays.fill(newMask, orSize, (regionSize - orSize), (byte)0);
          orMask = newMask;
        }
      }
    }

    if (cursorPosition != Constants.CURSOR_OFF) {
      if (cursorPosition != Constants.CURSOR_LEAVE) {
        checkRange(CURSOR_POSITION, cursorPosition, 1, cellCount);
      }
    }
  }
}

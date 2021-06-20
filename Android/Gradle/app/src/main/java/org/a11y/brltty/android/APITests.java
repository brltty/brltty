/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
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

package org.a11y.brltty.android;

import android.os.Build;

public abstract class APITests {
  private APITests () {
  }

  private static boolean haveAPILevel (int level) {
    return Build.VERSION.SDK_INT >= level;
  }

  public final static boolean haveIceCreamSandwich
  = haveAPILevel(Build.VERSION_CODES.ICE_CREAM_SANDWICH);

  public final static boolean haveJellyBean
  = haveAPILevel(Build.VERSION_CODES.JELLY_BEAN);

  public final static boolean haveJellyBeanMR1
  = haveAPILevel(Build.VERSION_CODES.JELLY_BEAN_MR1);

  public final static boolean haveJellyBeanMR2
  = haveAPILevel(Build.VERSION_CODES.JELLY_BEAN_MR2);

  public final static boolean haveKitkat
  = haveAPILevel(Build.VERSION_CODES.KITKAT);

  public final static boolean haveLollipop
  = haveAPILevel(Build.VERSION_CODES.LOLLIPOP);

  public final static boolean haveLollipopMR1
  = haveAPILevel(Build.VERSION_CODES.LOLLIPOP_MR1);

  public final static boolean haveMarshmallow
  = haveAPILevel(Build.VERSION_CODES.M);

  public final static boolean haveNougat
  = haveAPILevel(Build.VERSION_CODES.N);

  public final static boolean haveNougatMR1
  = haveAPILevel(Build.VERSION_CODES.N_MR1);

  public final static boolean haveOreo
  = haveAPILevel(Build.VERSION_CODES.O);

  public final static boolean haveOreoMR1
  = haveAPILevel(Build.VERSION_CODES.O_MR1);

  public final static boolean havePie
  = haveAPILevel(Build.VERSION_CODES.P);

  public final static boolean haveQ
  = haveAPILevel(Build.VERSION_CODES.Q);
}

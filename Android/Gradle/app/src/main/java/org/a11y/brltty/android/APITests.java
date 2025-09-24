/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
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

  // API level 1 - Android 1.0
  public final static boolean haveBase
  = haveAPILevel(Build.VERSION_CODES.BASE);

  // API level 2 - Android 1.1
  public final static boolean haveBase11
  = haveAPILevel(Build.VERSION_CODES.BASE_1_1);

  // API level 3 - Android 1.5
  public final static boolean haveCupcake
  = haveAPILevel(Build.VERSION_CODES.CUPCAKE);

  // API level 4 - Android 1.6
  public final static boolean haveDonut
  = haveAPILevel(Build.VERSION_CODES.DONUT);

  // API level 5 - Android 2.0
  public final static boolean haveEclair
  = haveAPILevel(Build.VERSION_CODES.ECLAIR);

  // API level 6 - Android 2.0.1
  public final static boolean haveEclair01
  = haveAPILevel(Build.VERSION_CODES.ECLAIR_0_1);

  // API level 7 - Android 2.1
  public final static boolean haveEclairMR1
  = haveAPILevel(Build.VERSION_CODES.ECLAIR_MR1);

  // API level 8 - Android 2.2
  public final static boolean haveFroyo
  = haveAPILevel(Build.VERSION_CODES.FROYO);

  // API level 9 - Android 2.3
  public final static boolean haveGingerbread
  = haveAPILevel(Build.VERSION_CODES.GINGERBREAD);

  // API level 10 - Android 2.3.3
  public final static boolean haveGingerbreadMR1
  = haveAPILevel(Build.VERSION_CODES.GINGERBREAD_MR1);

  // API level 11 - Android 3.0
  public final static boolean haveHoneycomb
  = haveAPILevel(Build.VERSION_CODES.HONEYCOMB);

  // API level 12 - Android 3.1
  public final static boolean haveHoneycombMR1
  = haveAPILevel(Build.VERSION_CODES.HONEYCOMB_MR1);

  // API level 13 - Android 3.2
  public final static boolean haveHoneycombMR2
  = haveAPILevel(Build.VERSION_CODES.HONEYCOMB_MR2);

  // API level 14 - Android 4.0
  public final static boolean haveIceCreamSandwich
  = haveAPILevel(Build.VERSION_CODES.ICE_CREAM_SANDWICH);

  // API level 15 - Android 4.0.3
  public final static boolean haveIceCreamSandwichMR1
  = haveAPILevel(Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1);

  // API level 16 - Android 4.1.2
  public final static boolean haveJellyBean
  = haveAPILevel(Build.VERSION_CODES.JELLY_BEAN);

  // API level 17 - Android 4.2.2
  public final static boolean haveJellyBeanMR1
  = haveAPILevel(Build.VERSION_CODES.JELLY_BEAN_MR1);

  // API level 18 - Android 4.3.1
  public final static boolean haveJellyBeanMR2
  = haveAPILevel(Build.VERSION_CODES.JELLY_BEAN_MR2);

  // API level 19 - Android 4.4.2
  public final static boolean haveKitkat
  = haveAPILevel(Build.VERSION_CODES.KITKAT);

  // API level 20 - Android 4.4W.2
  public final static boolean haveKitkatWatch
  = haveAPILevel(Build.VERSION_CODES.KITKAT_WATCH);

  // API level 21 - Android 5.0.1
  public final static boolean haveLollipop
  = haveAPILevel(Build.VERSION_CODES.LOLLIPOP);

  // API level 22 - Android 5.1.1
  public final static boolean haveLollipopMR1
  = haveAPILevel(Build.VERSION_CODES.LOLLIPOP_MR1);

  // API level 23 - Android 6.0
  public final static boolean haveMarshmallow
  = haveAPILevel(Build.VERSION_CODES.M);

  // API level 24 - Android 7.0
  public final static boolean haveNougat
  = haveAPILevel(Build.VERSION_CODES.N);

  // API level 25 - Android 7.1.1
  public final static boolean haveNougatMR1
  = haveAPILevel(Build.VERSION_CODES.N_MR1);

  // API level 26 - Android 8.0.0
  public final static boolean haveOreo
  = haveAPILevel(Build.VERSION_CODES.O);

  // API level 27 - Android 8.1.0
  public final static boolean haveOreoMR1
  = haveAPILevel(Build.VERSION_CODES.O_MR1);

  // API level 28 - Android 9
  public final static boolean havePie
  = haveAPILevel(Build.VERSION_CODES.P);

  // API level 29 - Android 10
  public final static boolean haveQ
  = haveAPILevel(Build.VERSION_CODES.Q);

  // API level 30 - Android 11
  public final static boolean haveR
  = haveAPILevel(Build.VERSION_CODES.R);

  // API level 31 - Android 12
  public final static boolean haveS
  = haveAPILevel(Build.VERSION_CODES.S);

  // API level 32 - Android 12L
  public final static boolean haveSV2
  = haveAPILevel(Build.VERSION_CODES.S_V2);

  // API level 33 - Android 13
  public final static boolean haveTiramisu
  = haveAPILevel(Build.VERSION_CODES.TIRAMISU);

  // API level 34 - Android 14
  public final static boolean haveUpsideDownCake
  = haveAPILevel(Build.VERSION_CODES.UPSIDE_DOWN_CAKE);

  // API level 35 - Android 15
  public final static boolean haveVanillaIceCream
  = haveAPILevel(Build.VERSION_CODES.VANILLA_ICE_CREAM);

  // API level 36 - Android 16
  public final static boolean haveBaklava
  = haveAPILevel(Build.VERSION_CODES.BAKLAVA);
}

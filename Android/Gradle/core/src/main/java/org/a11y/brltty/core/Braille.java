/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
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

package org.a11y.brltty.core;

public abstract class Braille {
  private Braille () {
  }

  public final static char ROW  = 0x2800;
  public final static char DOT1 = 0x0001;
  public final static char DOT2 = 0x0002;
  public final static char DOT3 = 0x0004;
  public final static char DOT4 = 0x0008;
  public final static char DOT5 = 0x0010;
  public final static char DOT6 = 0x0020;
  public final static char DOT7 = 0x0040;
  public final static char DOT8 = 0x0080;

  public final static char DOTS_LEFT  = DOT1 | DOT2 | DOT3 | DOT7;
  public final static char DOTS_RIGHT = DOT4 | DOT5 | DOT6 | DOT8;
  public final static char DOTS_ALL   = DOTS_LEFT | DOTS_RIGHT;

  public final static char DOTS_UPPER  = DOT1 | DOT4;
  public final static char DOTS_MIDDLE = DOT2 | DOT5;
  public final static char DOTS_LOWER  = DOT3 | DOT6;
  public final static char DOTS_BELOW  = DOT7 | DOT8;

  public final static char DOTS_LITERARY = DOTS_UPPER | DOTS_MIDDLE | DOTS_LOWER;
  public final static char DOTS_COMPUTER = DOTS_LITERARY | DOTS_BELOW;

  public final static char DOTS_A = DOT1;
  public final static char DOTS_B = DOT1 | DOT2;
  public final static char DOTS_C = DOT1 | DOT4;
  public final static char DOTS_D = DOT1 | DOT4 | DOT5;
  public final static char DOTS_E = DOT1 | DOT5;
  public final static char DOTS_F = DOT1 | DOT2 | DOT4;
  public final static char DOTS_G = DOT1 | DOT2 | DOT4 | DOT5;
  public final static char DOTS_H = DOT1 | DOT2 | DOT5;
  public final static char DOTS_I = DOT2 | DOT4;
  public final static char DOTS_J = DOT2 | DOT4 | DOT5;
  public final static char DOTS_K = DOT1 | DOT3;
  public final static char DOTS_L = DOT1 | DOT2 | DOT3;
  public final static char DOTS_M = DOT1 | DOT3 | DOT4;
  public final static char DOTS_N = DOT1 | DOT3 | DOT4 | DOT5;
  public final static char DOTS_O = DOT1 | DOT3 | DOT5;
  public final static char DOTS_P = DOT1 | DOT2 | DOT3 | DOT4;
  public final static char DOTS_Q = DOT1 | DOT2 | DOT3 | DOT4 | DOT5;
  public final static char DOTS_R = DOT1 | DOT2 | DOT3 | DOT5;
  public final static char DOTS_S = DOT2 | DOT3 | DOT4;
  public final static char DOTS_T = DOT2 | DOT3 | DOT4 | DOT5;
  public final static char DOTS_U = DOT1 | DOT3 | DOT6;
  public final static char DOTS_V = DOT1 | DOT2 | DOT3 | DOT6;
  public final static char DOTS_W = DOT2 | DOT4 | DOT5 | DOT6;
  public final static char DOTS_X = DOT1 | DOT3 | DOT4 | DOT6;
  public final static char DOTS_Y = DOT1 | DOT3 | DOT4 | DOT5 | DOT6;
  public final static char DOTS_Z = DOT1 | DOT3 | DOT5 | DOT6;

  public final static char DOTS_0 = DOT3 | DOT5 | DOT6;
  public final static char DOTS_1 = DOT2;
  public final static char DOTS_2 = DOT2 | DOT3;
  public final static char DOTS_3 = DOT2 | DOT5;
  public final static char DOTS_4 = DOT2 | DOT5 | DOT6;
  public final static char DOTS_5 = DOT2 | DOT6;
  public final static char DOTS_6 = DOT2 | DOT3 | DOT5;
  public final static char DOTS_7 = DOT2 | DOT3 | DOT5 | DOT6;
  public final static char DOTS_8 = DOT2 | DOT3 | DOT6;
  public final static char DOTS_9 = DOT3 | DOT5;
}

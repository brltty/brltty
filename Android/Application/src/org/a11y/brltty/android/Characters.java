/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2019 by The BRLTTY Developers.
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

public abstract class Characters {
  private Characters () {
  }

  public static final char BRAILLE_ROW  = 0x2800;
  public static final char BRAILLE_DOT1 = 0x0001;
  public static final char BRAILLE_DOT2 = 0x0002;
  public static final char BRAILLE_DOT3 = 0x0004;
  public static final char BRAILLE_DOT4 = 0x0008;
  public static final char BRAILLE_DOT5 = 0x0010;
  public static final char BRAILLE_DOT6 = 0x0020;
  public static final char BRAILLE_DOT7 = 0x0040;
  public static final char BRAILLE_DOT8 = 0x0080;

  public static final char CHECKBOX_BEGIN = BRAILLE_ROW
                                          | BRAILLE_DOT4
                                          | BRAILLE_DOT1
                                          | BRAILLE_DOT2
                                          | BRAILLE_DOT3
                                          | BRAILLE_DOT7
                                          | BRAILLE_DOT8
                                          ;

  public static final char CHECKBOX_MARK = BRAILLE_ROW
                                         | BRAILLE_DOT2
                                         | BRAILLE_DOT5
                                         | BRAILLE_DOT3
                                         | BRAILLE_DOT6
                                         ;

  public static final char CHECKBOX_END = BRAILLE_ROW
                                        | BRAILLE_DOT1
                                        | BRAILLE_DOT4
                                        | BRAILLE_DOT5
                                        | BRAILLE_DOT6
                                        | BRAILLE_DOT8
                                        | BRAILLE_DOT7
                                        ;

  public static final char RADIO_BEGIN = BRAILLE_ROW
                                       | BRAILLE_DOT4
                                       | BRAILLE_DOT2
                                       | BRAILLE_DOT3
                                       | BRAILLE_DOT8
                                       ;

  public static final char RADIO_MARK = BRAILLE_ROW
                                      | BRAILLE_DOT2
                                      | BRAILLE_DOT5
                                      | BRAILLE_DOT3
                                      | BRAILLE_DOT6
                                      ;

  public static final char RADIO_END = BRAILLE_ROW
                                     | BRAILLE_DOT1
                                     | BRAILLE_DOT5
                                     | BRAILLE_DOT6
                                     | BRAILLE_DOT7
                                     ;

  public static final char SWITCH_BEGIN = BRAILLE_ROW
                                        | BRAILLE_DOT4
                                        | BRAILLE_DOT5
                                        | BRAILLE_DOT6
                                        | BRAILLE_DOT8
                                        ;

  public static final char SWITCH_OFF = BRAILLE_ROW
                                      | BRAILLE_DOT1
                                      | BRAILLE_DOT4
                                      | BRAILLE_DOT3
                                      | BRAILLE_DOT6
                                      | BRAILLE_DOT7
                                      | BRAILLE_DOT8
                                      ;

  public static final char SWITCH_ON = BRAILLE_ROW
                                     | BRAILLE_DOT1
                                     | BRAILLE_DOT4
                                     | BRAILLE_DOT2
                                     | BRAILLE_DOT5
                                     | BRAILLE_DOT7
                                     | BRAILLE_DOT8
                                     ;

  public static final char SWITCH_END = BRAILLE_ROW
                                      | BRAILLE_DOT1
                                      | BRAILLE_DOT2
                                      | BRAILLE_DOT3
                                      | BRAILLE_DOT7
                                      ;
}

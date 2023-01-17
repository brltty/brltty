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

package org.a11y.brltty.android;
import org.a11y.brltty.core.Braille;

public abstract class Characters {
  private Characters () {
  }

  public final static char CHECKBOX_BEGIN = Braille.ROW
                                          | Braille.DOT4
                                          | Braille.DOT1
                                          | Braille.DOT2
                                          | Braille.DOT3
                                          | Braille.DOT7
                                          | Braille.DOT8
                                          ;

  public final static char CHECKBOX_MARK = Braille.ROW
                                         | Braille.DOT2
                                         | Braille.DOT5
                                         | Braille.DOT3
                                         | Braille.DOT6
                                         ;

  public final static char CHECKBOX_END = Braille.ROW
                                        | Braille.DOT1
                                        | Braille.DOT4
                                        | Braille.DOT5
                                        | Braille.DOT6
                                        | Braille.DOT8
                                        | Braille.DOT7
                                        ;

  public final static char RADIO_BEGIN = Braille.ROW
                                       | Braille.DOT4
                                       | Braille.DOT2
                                       | Braille.DOT3
                                       | Braille.DOT8
                                       ;

  public final static char RADIO_MARK = Braille.ROW
                                      | Braille.DOT2
                                      | Braille.DOT5
                                      | Braille.DOT3
                                      | Braille.DOT6
                                      ;

  public final static char RADIO_END = Braille.ROW
                                     | Braille.DOT1
                                     | Braille.DOT5
                                     | Braille.DOT6
                                     | Braille.DOT7
                                     ;

  public final static char SWITCH_BEGIN = Braille.ROW
                                        | Braille.DOT4
                                        | Braille.DOT5
                                        | Braille.DOT6
                                        | Braille.DOT8
                                        ;

  public final static char SWITCH_OFF = Braille.ROW
                                      | Braille.DOT1
                                      | Braille.DOT4
                                      | Braille.DOT3
                                      | Braille.DOT6
                                      | Braille.DOT7
                                      | Braille.DOT8
                                      ;

  public final static char SWITCH_ON = Braille.ROW
                                     | Braille.DOT1
                                     | Braille.DOT4
                                     | Braille.DOT2
                                     | Braille.DOT5
                                     | Braille.DOT7
                                     | Braille.DOT8
                                     ;

  public final static char SWITCH_END = Braille.ROW
                                      | Braille.DOT1
                                      | Braille.DOT2
                                      | Braille.DOT3
                                      | Braille.DOT7
                                      ;
}

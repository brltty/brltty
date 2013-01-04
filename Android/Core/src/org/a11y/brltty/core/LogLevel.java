/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brltty.core;

public enum LogLevel {
  EMERGENCY   (0),
  ALERT       (1),
  CRITICAL    (2),
  ERROR       (3),
  WARNING     (4),
  NOTICE      (5),
  INFORMATION (6),
  DEBUG       (7);

  public final int value;

  LogLevel (int value) {
    this.value = value;
  }
}

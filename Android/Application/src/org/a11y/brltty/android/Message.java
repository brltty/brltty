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
import org.a11y.brltty.core.*;

public abstract class Message {
  private Message () {
  }

  public static enum Type {
    WARNING("WRN"),
    NOTIFICATION("NTF"),
    TOAST("ALR"),
    ANNOUNCEMENT("ANN"),
    ; // end of enumeration

    private final String typeLabel;

    Type (String label) {
      typeLabel = label;
    }

    public final String getLabel () {
      return typeLabel;
    }
  }

  public static void show (Type type, String text) {
    if (text == null) return;
    text = text.replace('\n', ' ').trim();
    if (text.isEmpty()) return;

    text = type.getLabel() + ": " + text;
    final String message = text;

    CoreWrapper.runOnCoreThread(
      new Runnable() {
        @Override
        public void run () {
          CoreWrapper.showMessage(message);
        }
      }
    );
  }
}

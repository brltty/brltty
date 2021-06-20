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
import org.a11y.brltty.core.*;

public enum BrailleMessage {
  PLAIN(null),
  WINDOW("win"),
  WARNING("warn"),
  NOTIFICATION("ntfc"),
  TOAST("alrt"),
  DEBUG("dbg"),
  ANNOUNCEMENT("ann"),
  ; // end of enumeration

  private final String messageLabel;

  BrailleMessage (String label) {
    messageLabel = label;
  }

  public final String getLabel () {
    return messageLabel;
  }

  public final void show (String text) {
    if (text == null) return;
    text = text.replace('\n', ' ').trim();
    if (text.isEmpty()) return;

    String label = getLabel();
    if (!((label == null) || label.isEmpty())) text = label + ": " + text;
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

  public final void show (int text) {
    show(BrailleApplication.get().getString(text));
  }
}

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

import android.view.KeyEvent;

public class VirtualScreenElement extends ScreenElement {
  private final int globalAction;
  private final int keyCode;

  @Override
  public boolean onClick () {
    if (APITests.haveJellyBean) {
      return BrailleService.getBrailleService().performGlobalAction(globalAction);
    }

    if (keyCode != KeyEvent.KEYCODE_UNKNOWN) {
      return InputService.injectKey(keyCode);
    }

    return super.onClick();
  }

  public VirtualScreenElement (int text, int action, int key) {
    super(text);
    globalAction = action;
    keyCode = key;
  }

  public VirtualScreenElement (int text, int action) {
    this(text, action, KeyEvent.KEYCODE_UNKNOWN);
  }
}

/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

package org.a11y.BRLTTY.Android;

import org.a11y.BRLTTY.Core.*;

import android.util.Log;
import android.content.Context;

public class CoreThread extends Thread {
  private final String LOG_TAG = this.getClass().getName();

  private final Context coreContext;

  CoreThread (Context context) {
    super("Core");
    coreContext = context;

    UsbHelper.construct(coreContext);
  }

  @Override
  public void run () {
    ArgumentsBuilder builder = new ArgumentsBuilder();

    // required settings
    builder.setForegroundExecution(true);

    // optional settings
    builder.setLogLevel(LogLevel.DEBUG);
    builder.setLogFile("/data/local/tmp/brltty.log");

    // settings for testing - should be removed
    builder.setBrailleDriver("al");
    builder.setBrailleDevice("usb:,bluetooth:00:A0:96:18:54:7E");

    String[] arguments = builder.getArguments();
    CoreWrapper.run(arguments);
  }
}

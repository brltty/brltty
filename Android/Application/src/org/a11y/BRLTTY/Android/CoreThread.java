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

public class CoreThread extends Thread {
  protected final String TAG = this.getClass().getName();

  CoreThread () {
    super("Core");
  }

  public void run () {
    ArgumentsBuilder builder = new ArgumentsBuilder();

    // required arguments
    builder.setForegroundExecution(true);

    // optional arguments
    builder.setLogLevel(LogLevel.DEBUG);
    builder.setLogFilePath("/data/local/tmp/brltty.log");

    String[] arguments = builder.getArguments();
    Log.d(TAG, "core arguments: " + arguments.toString());
    Wrapper.main(arguments);
  }
}

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

package org.a11y.BRLTTY.Core;

public class Wrapper {
  protected static final String TAG = "BRLTTY_Core";

  public static native int construct (String[] arguments);
  public static native boolean update ();
  public static native void destruct ();

  public static volatile boolean stop = false;

  public static void main (String[] arguments) {
    int exitStatus = construct(arguments);

    if (exitStatus == ProgramExitStatus.SUCCESS.value) {
      while (update()) {
        if (stop) {
          break;
        }
      }
    } else if (exitStatus == ProgramExitStatus.FORCE.value) {
      exitStatus = ProgramExitStatus.SUCCESS.value;
    }

    destruct();
    System.exit(exitStatus);
  }

  static {
    System.loadLibrary("brltty_core");
    System.loadLibrary("brltty_jni");
  }
}

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

import java.util.Set;

import java.util.AbstractQueue;
import java.util.concurrent.LinkedBlockingDeque;

public class CoreWrapper {
  public static native int coreConstruct (String[] arguments, ClassLoader classLoader);
  public static native boolean coreDestruct ();

  public static native boolean coreEnableInterrupt ();
  public static native boolean coreDisableInterrupt ();
  public static native boolean coreInterrupt (boolean stop);

  public static native boolean coreWait (int duration);

  public static native boolean changeLogLevel (String level);
  public static native boolean changeLogCategories (String categories);

  public static native boolean changeTextTable (String name);
  public static native boolean changeAttributesTable (String name);
  public static native boolean changeContractionTable (String name);
  public static native boolean changeKeyboardKeyTable (String name);

  public static native boolean restartBrailleDriver ();
  public static native boolean changeBrailleDriver (String driver);
  public static native boolean changeBrailleDevice (String device);

  public static native boolean restartSpeechDriver ();
  public static native boolean changeSpeechDriver (String driver);

  public static boolean changeLogCategories (Set<String> categories) {
    StringBuffer sb = new StringBuffer();

    for (String category : categories) {
      if (sb.length() > 0) sb.append(',');
      sb.append(category);
    }

    return changeLogCategories(sb.toString());
  }

  public static boolean changeBrailleDevice (String qualifier, String reference) {
    return changeBrailleDevice(qualifier + ":" + reference);
  }

  private static final AbstractQueue<Runnable> runQueue = new LinkedBlockingDeque<Runnable>();

  public static void clearRunQueue () {
    runQueue.clear();
  }

  public static void processRunQueue () {
    Runnable runnable;
    while ((runnable = runQueue.poll()) != null) runnable.run();
  }

  public static boolean runOnCoreThread (Runnable runnable) {
    if (!runQueue.offer(runnable)) return false;
    coreInterrupt(false);
    return true;
  }

  public static void stop () {
    coreInterrupt(true);
  }

  public static int run (String[] arguments, int waitDuration) {
    clearRunQueue();

    int exitStatus = coreConstruct(arguments, CoreWrapper.class.getClassLoader());
    if (exitStatus == ProgramExitStatus.SUCCESS.value) {
      boolean interruptEnabled = coreEnableInterrupt();

      while (coreWait(waitDuration)) {
        processRunQueue();
      }

      if (interruptEnabled) {
        coreDisableInterrupt();
      }
    } else if (exitStatus == ProgramExitStatus.FORCE.value) {
      exitStatus = ProgramExitStatus.SUCCESS.value;
    }

    coreDestruct();
    return exitStatus;
  }

  public static void main (String[] arguments) {
    System.exit(run(arguments, Integer.MAX_VALUE));
  }

  static {
    System.loadLibrary("brltty_core");
    System.loadLibrary("brltty_jni");
  }
}

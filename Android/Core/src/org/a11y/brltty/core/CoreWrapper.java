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
  public static native int construct (String[] arguments);
  public static native boolean update ();
  public static native void destruct ();

  public static native boolean changeLogLevel (String level);
  public static native boolean changeLogCategories (String categories);

  public static native boolean changeTextTable (String name);
  public static native boolean changeAttributesTable (String name);
  public static native boolean changeContractionTable (String name);

  public static native boolean restartBrailleDriver ();
  public static native boolean changeBrailleDriver (String driver);
  public static native boolean changeBrailleDevice (String device);

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
    return runQueue.offer(runnable);
  }

  private static volatile boolean stop = false;

  public static void stop () {
    stop = true;
  }

  public static int run (String[] arguments) {
    stop = false;
    clearRunQueue();

    int exitStatus = construct(arguments);
    if (exitStatus == ProgramExitStatus.SUCCESS.value) {
      while (update()) {
        if (stop) break;
        processRunQueue();
      }
    } else if (exitStatus == ProgramExitStatus.FORCE.value) {
      exitStatus = ProgramExitStatus.SUCCESS.value;
    }

    destruct();
    return exitStatus;
  }

  public static void main (String[] arguments) {
    System.exit(run(arguments));
  }

  static {
    System.loadLibrary("brltty_core");
    System.loadLibrary("brltty_jni");
  }
}

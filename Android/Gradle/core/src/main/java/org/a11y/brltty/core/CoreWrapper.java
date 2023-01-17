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

package org.a11y.brltty.core;

import java.util.Set;

import java.util.AbstractQueue;
import java.util.concurrent.LinkedBlockingDeque;

public abstract class CoreWrapper {
  private CoreWrapper () {
  }

  static {
    System.loadLibrary("brltty_core");
    System.loadLibrary("brltty_jni");
  }

  public native static int coreConstruct (String[] arguments, ClassLoader classLoader);
  public native static boolean coreDestruct ();

  public native static boolean coreEnableInterrupt ();
  public native static boolean coreDisableInterrupt ();

  public native static boolean coreInterrupt (boolean stop);
  public native static boolean coreWait (int duration);

  public native static boolean changeLogLevel (String level);
  public native static boolean changeLogCategories (String categories);

  public native static boolean changeTextTable (String name);
  public native static boolean changeAttributesTable (String name);
  public native static boolean changeContractionTable (String name);
  public native static boolean changeKeyboardTable (String name);

  public native static boolean restartBrailleDriver ();
  public native static boolean changeBrailleDriver (String driver);
  public native static boolean changeBrailleParameters (String parameters);
  public native static boolean changeBrailleDevice (String device);

  public native static boolean restartSpeechDriver ();
  public native static boolean changeSpeechDriver (String driver);
  public native static boolean changeSpeechParameters (String parameters);

  public native static boolean restartScreenDriver ();
  public native static boolean changeScreenDriver (String driver);
  public native static boolean changeScreenParameters (String parameters);

  public native static boolean changeMessageLocale (String locale);
  public native static void showMessage (String text);

  public native static boolean setEnvironmentVariable (String name, String value);

  public static boolean changeLogCategories (Set<String> categories) {
    StringBuilder sb = new StringBuilder();

    for (String category : categories) {
      if (sb.length() > 0) sb.append(',');
      sb.append(category);
    }

    return changeLogCategories(sb.toString());
  }

  private static Thread coreThread = null;
  private final static AbstractQueue<Runnable> runQueue = new LinkedBlockingDeque<Runnable>();

  public static void clearRunQueue () {
    runQueue.clear();
  }

  public static void processRunQueue () {
    Runnable runnable;
    while ((runnable = runQueue.poll()) != null) runnable.run();
  }

  public static boolean runOnCoreThread (Runnable runnable) {
    if (Thread.currentThread() == coreThread) {
      runnable.run();
    } else {
      if (!runQueue.offer(runnable)) return false;
      coreInterrupt(false);
    }

    return true;
  }

  public static void stop () {
    coreInterrupt(true);
  }

  public static int run (String[] arguments, int waitDuration) {
    clearRunQueue();
    coreThread = Thread.currentThread();

    try {
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
    } finally {
      coreThread = null;
    }
  }

  public static void main (String[] arguments) {
    System.exit(run(arguments, Integer.MAX_VALUE));
  }
}

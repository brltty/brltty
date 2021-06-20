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

public abstract class DeferredTask implements Runnable {
  protected abstract void runTask ();

  protected boolean isStartable () {
    return true;
  }

  protected void releaseResources () {
  }

  private final long taskDelay;
  private boolean isScheduled = false;

  protected DeferredTask (long delay) {
    taskDelay = delay;
  }

  private final void scheduleTask () {
    synchronized (this) {
      if (!isScheduled) {
        BrailleApplication.postIn(taskDelay, this);
        isScheduled = true;
      }
    }
  }

  private final void cancelTask () {
    synchronized (this) {
      if (isScheduled) {
        BrailleApplication.unpost(this);
        isScheduled = false;
      }
    }
  }

  public final void restart () {
    synchronized (this) {
      cancelTask();
      if (isStartable()) scheduleTask();
    }
  }

  public final void stop () {
    synchronized (this) {
      cancelTask();
      releaseResources();
    }
  }

  @Override
  public final void run () {
    synchronized (this) {
      if (isScheduled) runTask();
      stop();
    }
  }
}

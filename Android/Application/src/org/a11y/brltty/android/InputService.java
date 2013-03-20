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

package org.a11y.brltty.android;

import android.util.Log;

import android.inputmethodservice.InputMethodService;
import android.content.ComponentName;

import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;

import android.view.inputmethod.EditorInfo;

public class InputService extends InputMethodService {
  private static final String LOG_TAG = InputService.class.getName();

  private static class InputConnection implements ServiceConnection {
    private IBinder inputBinder = null;

    @Override
    public void onServiceConnected (ComponentName component, IBinder binder) {
      Log.d(LOG_TAG, "input service connected");

      synchronized (this) {
        inputBinder = binder;
      }
    }

    @Override
    public void onServiceDisconnected (ComponentName component) {
      Log.d(LOG_TAG, "input service disconnected");

      synchronized (this) {
        inputBinder = null;
      }
    }
  }

  private static Context inputContext = null;
  private static Intent inputServiceIntent = null;
  private static ServiceConnection inputConnection = null;

  public static void startInputService (Context context) {
    inputContext = context;

    inputServiceIntent = new Intent(inputContext, InputService.class);
    inputContext.startService(inputServiceIntent);

    inputConnection = new InputConnection();
    inputContext.bindService(inputServiceIntent, inputConnection,
                             Context.BIND_AUTO_CREATE | Context.BIND_IMPORTANT);
  }

  public static void stopInputService () {
    inputContext.unbindService(inputConnection);
    inputConnection = null;

    inputContext.stopService(inputServiceIntent);
    inputServiceIntent = null;

    inputContext = null;
  }

  @Override
  public void onStartInput (EditorInfo info, boolean restarting) {
  }
}

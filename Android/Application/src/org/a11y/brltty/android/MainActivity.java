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

package org.a11y.brltty.android;

import org.a11y.brltty.core.*;

import android.util.Log;

import android.app.Activity;
import android.os.Bundle;

import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

import android.content.Intent;

public class MainActivity extends Activity {
  private static final String LOG_TAG = MainActivity.class.getName();

  private Thread coreThread = null;

  private final void handleStopButton () {
    Button button = (Button)findViewById(R.id.stopButton);

    button.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick (View view) {
        Log.d(LOG_TAG, "stop button pressed");
        CoreWrapper.stop = true;

        try {
          coreThread.join();
        } catch (InterruptedException exception) {
        }

        finish();
      }
    });
  }

  private final void handleSettingsButton () {
    Button button = (Button)findViewById(R.id.settingsButton);

    button.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick (View view) {
        Log.d(LOG_TAG, "settings button pressed");
        Intent intent = new Intent(MainActivity.this, SettingsActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
      }
    });
  }

  @Override
  public void onCreate (Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.main);

    handleStopButton();
    handleSettingsButton();

    coreThread = new CoreThread(this);
    coreThread.start();
  }
}

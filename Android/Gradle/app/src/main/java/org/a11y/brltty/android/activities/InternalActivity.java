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

package org.a11y.brltty.android.activities;
import org.a11y.brltty.android.*;

import android.util.Log;
import android.widget.Toast;

import android.app.AlertDialog;
import android.content.DialogInterface;

import android.app.Activity;
import android.content.Intent;
import android.content.ActivityNotFoundException;

import android.net.Uri;
import java.io.File;

public abstract class InternalActivity extends Activity {
  private final static String LOG_TAG = InternalActivity.class.getName();

  protected final String getResourceString (int identifier) {
    return getResources().getString(identifier);
  }

  protected final void showMessage (CharSequence message, boolean asToast) {
    if (asToast) {
      Toast.makeText(this, message, Toast.LENGTH_LONG).show();
    } else {
      DialogInterface.OnClickListener listener =
        new DialogInterface.OnClickListener() {
          @Override
          public void onClick (DialogInterface dialog, int button) {
            dialog.dismiss();
          }
        };

      new AlertDialog.Builder(this)
        .setMessage(message)
        .setNeutralButton(android.R.string.yes, listener)
        .create()
        .show();
    }
  }

  protected final void showMessage (CharSequence message) {
    showMessage(message, false);
  }

  protected final void showMessage (int message, boolean asToast) {
    showMessage(getResourceString(message), asToast);
  }

  protected final void showMessage (int message) {
    showMessage(message, false);
  }

  protected final void launch (Intent intent) {
    try {
      startActivity(intent);
    } catch (ActivityNotFoundException exception) {
      Log.w(LOG_TAG, "activity not found", exception);
    }
  }

  protected final void launch (Uri uri) {
    launch(new Intent(Intent.ACTION_VIEW, uri));
  }

  protected final void launch (File file) {
    launch(Uri.fromFile(file));
  }

  protected final void launch (String url) {
    launch(Uri.parse(url));
  }

  protected final void launch (int url) {
    launch(getResourceString(url));
  }
}

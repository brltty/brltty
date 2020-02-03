/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2020 by The BRLTTY Developers.
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

import android.util.Log;

import android.app.Activity;
import android.content.Intent;
import android.content.ActivityNotFoundException;

import android.net.Uri;
import java.io.File;
import android.support.v4.content.FileProvider;

public class PackageInstaller {
  private final static String LOG_TAG = PackageInstaller.class.getName();

  protected final Activity owningActivity;
  protected final String sourceURL;
  protected final File targetFile;

  public PackageInstaller (Activity activity, String url, File file) {
    owningActivity = activity;
    sourceURL = url;
    targetFile = file;
  }

  public PackageInstaller (Activity activity, int url, File file) {
    this(activity, activity.getResources().getString(url), file);
  }

  protected void onInstallFailed (String message) {
  }

  private void onInstallFailed (Exception exception) {
    String message = exception.getMessage();
    Log.w(LOG_TAG, String.format("package install failed: %s: %s", sourceURL, message));
    onInstallFailed(message);
  }

  public final void startInstall () {
    new FileDownloader(owningActivity, sourceURL, targetFile) {
      @Override
      protected void onDownloadProgress (long time, long position, Long length) {
        if (true) {
          long remaining = (length == null)? -1: (length - position);
          Log.d("dnld-prog", String.format("t=%d p=%d r=%d", time, position, remaining));
        }
      }

      @Override
      protected void onDownloadFinished () {
        Intent intent = new Intent(Intent.ACTION_VIEW);

        if (ApplicationUtilities.haveNougat) {
          String authority = getClass().getPackage().getName() + ".fileprovider";
          Uri uri = FileProvider.getUriForFile(owningActivity, authority, targetFile);

          intent.setData(uri);
          intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } else {
          intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
          intent.setDataAndType(Uri.fromFile(targetFile), "application/vnd.android.package-archive");
        }

        try {
          owningActivity.startActivity(intent);
        } catch (ActivityNotFoundException exception) {
          onInstallFailed(exception);
        }
      }

      @Override
      protected void onDownloadFailed (String message) {
        onInstallFailed(message);
      }
    }.startDownload();
  }
}

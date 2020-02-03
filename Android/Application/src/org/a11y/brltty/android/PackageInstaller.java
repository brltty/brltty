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
import android.os.AsyncTask;

import android.content.Context;
import android.content.Intent;
import android.content.ActivityNotFoundException;

import android.net.Uri;
import java.net.URL;
import java.net.URLConnection;
import java.net.HttpURLConnection;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileOutputStream;

import java.io.File;
import android.support.v4.content.FileProvider;

public class PackageInstaller {
  private final static String LOG_TAG = PackageInstaller.class.getName();

  protected final Context owningContext;
  protected final String sourceURL;
  protected final File targetFile;

  public PackageInstaller (Context context, String url, File file) {
    owningContext = context;
    sourceURL = url;
    targetFile = file;
  }

  public PackageInstaller (Context context, int url, File file) {
    this(context, context.getResources().getString(url), file);
  }

  private static void copy (InputStream input, OutputStream output) throws IOException {
    byte[] buffer = new byte[0X1000];
    int length;

    while ((length = input.read(buffer)) != -1) {
      output.write(buffer, 0, length);
    }
  }

  protected void onInstallFailed (String message) {
  }

  public final void startInstall () {
    new AsyncTask<Object, Object, String>() {
      @Override
      protected String doInBackground (Object... arguments) {
        try {
          URL url = new URL(sourceURL);
          HttpURLConnection connection = (HttpURLConnection)url.openConnection();
          connection.setRequestMethod("GET");
          connection.connect();

          try {
            InputStream input = connection.getInputStream();

            try {
              File file = targetFile;
              file.delete();

              try {
                OutputStream output = new FileOutputStream(file);

                try {
                  copy(input, output);
                  output.flush();

                  try {
                    Intent intent = new Intent(Intent.ACTION_VIEW);

                    if (ApplicationUtilities.haveNougat) {
                      String authority = getClass().getPackage().getName() + ".fileprovider";
                      Uri uri = FileProvider.getUriForFile(owningContext, authority, file);

                      intent.setData(uri);
                      intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    } else {
                      intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                      intent.setDataAndType(Uri.fromFile(file), "application/vnd.android.package-archive");
                    }

                    owningContext.startActivity(intent);
                    file = null;
                  } catch (ActivityNotFoundException exception) {
                    throw new IOException(exception.getMessage());
                  }
                } finally {
                  output.close();
                  output = null;
                }
              } finally {
                if (file != null) file.delete();
              }
            } finally {
              input.close();
              input = null;
            }
          } finally {
            connection.disconnect();
            connection = null;
          }
        } catch (IOException exception) {
          String message = exception.getMessage();
          Log.w(LOG_TAG, String.format("package install failed: %s: %s", sourceURL, message));
          return message;
        }

        return null;
      }

      @Override
      protected void onPostExecute (String message) {
        if (message != null) onInstallFailed(message);
      }
    }.execute();
  }
}

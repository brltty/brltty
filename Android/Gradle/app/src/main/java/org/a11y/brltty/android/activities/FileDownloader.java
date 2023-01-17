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
import android.os.AsyncTask;

import android.content.Context;
import java.util.Locale;

import android.view.LayoutInflater;
import android.app.AlertDialog;

import android.view.View;
import android.widget.TextView;
import android.widget.ProgressBar;

import java.io.File;
import java.net.URL;
import java.net.URLConnection;
import java.net.HttpURLConnection;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileOutputStream;

public class FileDownloader extends UpgradeComponent {
  private final static String LOG_TAG = FileDownloader.class.getName();

  protected final String sourceURL;
  protected final File targetFile;

  public FileDownloader (InternalActivity owner, String url, File file) {
    super(owner);
    sourceURL = url;
    targetFile = file;
  }

  protected void onDownloadStarted () {
  }

  protected void onDownloadProgress (long time, long position, Long length) {
    if (false) {
      long remaining = (length == null)? -1: (length - position);
      Log.d("dnld-prog", String.format("t=%d p=%d r=%d", time, position, remaining));
    }
  }

  protected void onDownloadFinished () {
  }

  protected void onDownloadFailed (String message) {
    getActivity().showMessage(
      String.format(
        "%s: %s",
        getString(R.string.fileDownloader_problem_failed),
        message
      )
    );
  }

  public final void startDownload () {
    new AsyncTask<Object, Object, String>() {
      private final Object PROGRESS_LOCK = new Object();
      private boolean progressInitialized = false;

      private long startTime;
      private long currentTime;
      private long currentPosition;
      private Long contentLength;

      private AlertDialog alertDialog;
      private TextView stateView;
      private View progressView;
      private ProgressBar progressBar;
      private TextView progressCurrent;
      private TextView progressRemaining;

      private long getTime () {
        return System.currentTimeMillis();
      }

      private void setState (int state) {
        stateView.setText(state);
      }

      private void setBytes (TextView view, long value) {
        view.setText(String.format(Locale.getDefault(), "%d", value));
      }

      private AlertDialog makeDialog () {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity())
          .setTitle(R.string.fileDownloader_title)
          .setMessage(sourceURL)
          ;

        {
          int layout = R.layout.file_downloader;

          if (APITests.haveLollipop) {
            builder.setView(layout);
          } else {
            Context context = builder.getContext();
            LayoutInflater inflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            builder.setView(inflater.inflate(layout, null));
          }
        }

        return builder.create();
      }

      private void showDialog () {
        alertDialog = makeDialog();
        alertDialog.show();

        stateView = (TextView)alertDialog.findViewById(R.id.state);
        progressView = alertDialog.findViewById(R.id.progress);
        progressBar = (ProgressBar)alertDialog.findViewById(R.id.bar);
        progressCurrent = (TextView)alertDialog.findViewById(R.id.current);
        progressRemaining = (TextView)alertDialog.findViewById(R.id.remaining);
      }

      @Override
      protected void onProgressUpdate (Object... arguments) {
        synchronized (PROGRESS_LOCK) {
          if (!progressInitialized) {
            progressView.setVisibility(View.VISIBLE);
            setState(R.string.fileDownloader_state_downloading);

            if (contentLength == null) {
              progressBar.setIndeterminate(true);
              progressRemaining.setVisibility(View.GONE);
            } else {
              progressBar.setIndeterminate(false);
              progressBar.setMax((int)(long)contentLength);

              if (APITests.haveOreo) {
                progressBar.setMin(0);
              }
            }

            progressInitialized = true;
          }

          progressBar.setProgress((int)currentPosition);
          setBytes(progressCurrent, currentPosition);

          if (contentLength != null) {
            setBytes(progressRemaining, (currentPosition - contentLength));
          }

          onDownloadProgress(currentTime, currentPosition, contentLength);
        }
      }

      private void copy (InputStream input, OutputStream output) throws IOException {
        synchronized (PROGRESS_LOCK) {
          currentPosition = 0;
          publishProgress();
        }

        byte[] buffer = new byte[0X1000];
        int count;

        while ((count = input.read(buffer)) != -1) {
          output.write(buffer, 0, count);
          currentPosition += count;

          synchronized (PROGRESS_LOCK) {
            long newTime = getTime() - startTime;

            if ((newTime - currentTime) > 100) {
              currentTime = newTime;
              publishProgress();
            }
          }
        }

        synchronized (PROGRESS_LOCK) {
          publishProgress();
        }
      }

      @Override
      protected void onPreExecute () {
        onDownloadStarted();
        showDialog();
        setState(R.string.fileDownloader_state_connecting);
      }

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
                  if (APITests.haveNougat) {
                    contentLength = connection.getContentLengthLong();
                  } else {
                    contentLength = (long)connection.getContentLength();
                  }

                  if (contentLength < 0) contentLength = null;
                  startTime = getTime();
                  currentTime = 0;

                  copy(input, output);
                  output.flush();

                  file = null;
                  return null;
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
          Log.w(LOG_TAG, String.format("file download failed: %s: %s", sourceURL, message));
          return message;
        }
      }

      @Override
      protected void onPostExecute (String message) {
        alertDialog.dismiss();

        if (message == null) {
          onDownloadFinished();
        } else {
          onDownloadFailed(message);
        }
      }
    }.execute();
  }
}

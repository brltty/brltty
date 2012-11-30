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

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import java.io.File;
import java.io.FileOutputStream;

import android.util.Log;

import android.content.Context;
import android.content.res.AssetManager;

import android.preference.PreferenceManager;
import android.content.SharedPreferences;

public class CoreThread extends Thread {
  private static final String LOG_TAG = CoreThread.class.getName();

  private final Context coreContext;

  public static final int DATA_MODE = Context.MODE_PRIVATE;
  public static final String DATA_TYPE_TABLES = "tables";
  public static final String DATA_TYPE_DRIVERS = "drivers";

  private final File getDataDirectory (String type) {
    return coreContext.getDir(type, DATA_MODE);
  }

  private final void extractAsset (AssetManager assets, String type, File path) {
    String asset = new File(type, path.getName()).getPath();
    Log.d(LOG_TAG, "extracting asset: " + asset + " -> " + path);

    try {
      InputStream input = null;
      OutputStream output = null;

      try {
        input = assets.open(asset);
        output = new FileOutputStream(path);

        byte[] buffer = new byte[0X4000];
        int count;

        while ((count = input.read(buffer)) > 0) {
          output.write(buffer, 0, count);
        }
      } finally {
        if (input != null) {
          input.close();
        }

        if (output != null) {
          output.close();
        }
      }
    } catch (IOException exception) {
      Log.e(LOG_TAG, "cannot extract asset: " + asset + " -> " + path, exception);
    }
  }

  private final void extractAssets (AssetManager assets, String type) {
    File directory = getDataDirectory(type);

    if (directory.canWrite()) {
      try {
        String[] files = assets.list(type);

        for (String file : files) {
          File path = new File(directory, file);

          if (path.exists()) {
            if (!path.canWrite()) {
              continue;
            }

            path.delete();
          }

          extractAsset(assets, type, path);
          path.setReadOnly();
        }

        directory.setReadOnly();
      } catch (IOException exception) {
        Log.e(LOG_TAG, "cannot list assets directory: " + type, exception);
      }
    }
  }

  private final void extractAssets () {
    AssetManager assets = coreContext.getAssets();
    extractAssets(assets, DATA_TYPE_TABLES);
    extractAssets(assets, DATA_TYPE_DRIVERS);
  }

  CoreThread (Context context) {
    super("Core");
    coreContext = context;

    UsbHelper.construct(coreContext);
  }

  @Override
  public void run () {
    extractAssets();

    ArgumentsBuilder builder = new ArgumentsBuilder();

    // required settings
    builder.setForegroundExecution(true);
    builder.setTablesDirectory(getDataDirectory(DATA_TYPE_TABLES).getPath());
    builder.setDriversDirectory(getDataDirectory(DATA_TYPE_DRIVERS).getPath());
    builder.setWritableDirectory(coreContext.getFilesDir().getPath());

    // optional settings
    SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(coreContext);
    builder.setBrailleDriver(prefs.getString("braille-driver", "auto"));
    builder.setTextTable(prefs.getString("text-table", "auto"));
    builder.setAttributesTable(prefs.getString("attributes-table", "attributes"));
    builder.setContractionTable(prefs.getString("contraction-table", "en-us-g2"));

    // settings for testing - should be removed
    builder.setLogLevel(LogLevel.DEBUG);
    builder.setLogFile("/data/local/tmp/brltty.log");
    builder.setBrailleDevice("usb:,bluetooth:00:A0:96:18:54:7E");

    String[] arguments = builder.getArguments();
    CoreWrapper.run(arguments);
  }
}

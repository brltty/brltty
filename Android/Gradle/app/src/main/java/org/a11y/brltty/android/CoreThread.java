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

package org.a11y.brltty.android;
import org.a11y.brltty.core.*;

import org.a11y.brltty.android.settings.DeviceManager;
import org.a11y.brltty.android.settings.DeviceDescriptor;

import java.util.Collections;
import java.util.ArrayList;
import java.util.Map;
import java.util.Set;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import java.io.File;
import java.io.FileOutputStream;

import android.util.Log;
import android.os.Environment;

import android.content.Context;
import android.content.res.AssetManager;
import android.content.SharedPreferences;

public class CoreThread extends Thread {
  private final static String LOG_TAG = CoreThread.class.getName();

  private final Context coreContext;

  private final void emptyDirectory (File directory) {
    if (!directory.canWrite()) {
      directory.setWritable(true, true);
    }

    for (File file : directory.listFiles()) {
      if (file.isDirectory()) {
        emptyDirectory(file);
      }

      file.delete();
    }
  }

  private final void extractAsset (AssetManager assets, String asset, File path) {
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

  private final void extractAssets (AssetManager assets, String asset, File path, boolean executable) {
    try {
      String[] names = assets.list(asset);

      if (names.length == 0) {
        extractAsset(assets, asset, path);

        if (executable) {
          path.setExecutable(true, false);
        }
      } else {
        if (!path.exists()) {
          path.mkdir();
        } else if (!path.isDirectory()) {
          Log.d(LOG_TAG, "not a directory: " + path);
          return;
        }

        for (String name : names) {
          extractAssets(assets, new File(asset, name).getPath(), new File(path, name), executable);
        }
      }

      path.setReadOnly();
    } catch (IOException exception) {
      Log.e(LOG_TAG, "cannot list asset: " + asset, exception);
    }
  }

  private final void extractAssets (AssetManager assets, DataType type, boolean executable) {
    File directory = type.getDirectory();
    emptyDirectory(directory);
    extractAssets(assets, type.getName(), directory, executable);
  }

  private final void extractAssets () {
    Log.d(LOG_TAG, "extracting assets");
    AssetManager assets = coreContext.getAssets();
    extractAssets(assets, DataType.LOCALE, false);
    extractAssets(assets, DataType.TABLES, false);
    Log.d(LOG_TAG, "assets extracted");
  }

  private String getStringResource (int resource) {
    return coreContext.getResources().getString(resource);
  }

  private static SharedPreferences getPreferences () {
    return DataType.getPreferences();
  }

  private boolean getBooleanSetting (int key, boolean defaultValue) {
    return getPreferences().getBoolean(getStringResource(key), defaultValue);
  }

  private boolean getBooleanSetting (int key) {
    return getBooleanSetting(key, false);
  }

  private String getStringSetting (int key, String defaultValue) {
    return getPreferences().getString(getStringResource(key), defaultValue);
  }

  private String getStringSetting (int key, int defaultValue) {
    return getStringSetting(key, getStringResource(defaultValue));
  }

  private String getStringSetting (int key) {
    return getStringSetting(key, "");
  }

  private Set<String> getStringSetSetting (int key) {
    return getPreferences().getStringSet(getStringResource(key), Collections.EMPTY_SET);
  }

  private final void updateDataFiles () {
    SharedPreferences prefs = getPreferences();
    File file = new File(coreContext.getPackageCodePath());

    String prefKey_size = getStringResource(R.string.PREF_KEY_PACKAGE_SIZE);
    long oldSize = prefs.getLong(prefKey_size, -1);
    long newSize = file.length();

    String prefKey_time = getStringResource(R.string.PREF_KEY_PACKAGE_TIME);
    long oldTime = prefs.getLong(prefKey_time, -1);
    long newTime = file.lastModified();

    if ((newSize != oldSize) || (newTime != oldTime)) {
      Log.d(LOG_TAG, "package size: " + oldSize + " -> " + newSize);
      Log.d(LOG_TAG, "package time: " + oldTime + " -> " + newTime);
      extractAssets();

      {
        SharedPreferences.Editor editor = prefs.edit();
        editor.putLong(prefKey_size, newSize);
        editor.putLong(prefKey_time, newTime);
        editor.apply();
      }
    }
  }

  private static File getStateFile (File directory, String extension) {
    String newName = "brltty." + extension;
    File newFile = new File(directory, newName);

    if (!newFile.exists()) {
      String oldName = "default." + extension;
      File oldFile = new File(directory, oldName);

      if (oldFile.exists()) {
        if (oldFile.renameTo(newFile)) {
          Log.d(LOG_TAG,
            String.format(
              "\"%s\" renamed to \"%s\"",
              oldName, newName
            )
          );
        } else {
          Log.w(LOG_TAG,
            String.format(
              "couldn't rename \"%s\" to \"%s\"",
              oldName, newName
            )
          );
        }
      } else {
        try {
          if (!newFile.createNewFile()) {
            Log.w(LOG_TAG,
              String.format(
                "couldn't create \"%s\"", newName
              )
            );
          }
        } catch (IOException exception) {
          Log.w(LOG_TAG,
            String.format(
              "file creation error: %s: %s",
              newName, exception.getMessage()
            )
          );
        }
      }
    }

    return newFile;
  }

  private String[] makeArguments () {
    ArgumentsBuilder builder = new ArgumentsBuilder();

    builder.setForegroundExecution(true);
    builder.setReleaseDevice(true);

    builder.setLocaleDirectory(DataType.LOCALE.getDirectory());
    builder.setTablesDirectory(DataType.TABLES.getDirectory());
    builder.setWritableDirectory(DataType.WRITABLE.getDirectory());

    {
      File directory = DataType.STATE.getDirectory();
      builder.setUpdatableDirectory(directory);

      builder.setConfigurationFile(getStateFile(directory, "conf"));
      builder.setPreferencesFile(getStateFile(directory, "prefs"));
    }

    builder.setTextTable(getStringSetting(R.string.PREF_KEY_TEXT_TABLE, R.string.DEFAULT_TEXT_TABLE));
    builder.setAttributesTable(getStringSetting(R.string.PREF_KEY_ATTRIBUTES_TABLE, R.string.DEFAULT_ATTRIBUTES_TABLE));
    builder.setContractionTable(getStringSetting(R.string.PREF_KEY_CONTRACTION_TABLE, R.string.DEFAULT_CONTRACTION_TABLE));
    builder.setKeyboardTable(getStringSetting(R.string.PREF_KEY_KEYBOARD_TABLE, R.string.DEFAULT_KEYBOARD_TABLE));

    {
      DeviceDescriptor descriptor = DeviceManager.getDeviceDescriptor();
      builder.setBrailleDevice(descriptor.getIdentifier());
      builder.setBrailleDriver(descriptor.getDriver());
    }

    builder.setSpeechDriver(getStringSetting(R.string.PREF_KEY_SPEECH_SUPPORT, R.string.DEFAULT_SPEECH_SUPPORT));
    builder.setQuietIfNoBraille(true);

    builder.setApiEnabled(true);
    builder.setApiParameters("host=127.0.0.1:0,auth=none");

    {
      ArrayList<String> keywords = new ArrayList<String>();
      keywords.add(getStringSetting(R.string.PREF_KEY_LOG_LEVEL, R.string.DEFAULT_LOG_LEVEL));
      keywords.addAll(getStringSetSetting(R.string.PREF_KEY_LOG_CATEGORIES));
      StringBuilder operand = new StringBuilder();

      for (String keyword : keywords) {
        if (keyword.length() > 0) {
          if (operand.length() > 0) operand.append(',');
          operand.append(keyword);
        }
      }

      builder.setLogLevel(operand.toString());
    }

    return builder.getArguments();
  }

  @Override
  public void run () {
    updateDataFiles();

    {
      UsbHelper.begin();

      try {
        CoreWrapper.run(makeArguments(), ApplicationParameters.CORE_WAIT_DURATION);
      } finally {
        UsbHelper.end();
      }
    }
  }

  private final void restoreSettings () {
    BrailleRenderer.setBrailleRenderer(getStringSetting(R.string.PREF_KEY_NAVIGATION_MODE, R.string.DEFAULT_NAVIGATION_MODE));

    ApplicationSettings.RELEASE_BRAILLE_DEVICE = getBooleanSetting(R.string.PREF_KEY_RELEASE_BRAILLE_DEVICE);

    ApplicationSettings.SHOW_NOTIFICATIONS = getBooleanSetting(R.string.PREF_KEY_SHOW_NOTIFICATIONS);
    ApplicationSettings.SHOW_ALERTS = getBooleanSetting(R.string.PREF_KEY_SHOW_ALERTS);
    ApplicationSettings.SHOW_ANNOUNCEMENTS = getBooleanSetting(R.string.PREF_KEY_SHOW_ANNOUNCEMENTS);

    ApplicationSettings.LOG_ACCESSIBILITY_EVENTS = getBooleanSetting(R.string.PREF_KEY_LOG_ACCESSIBILITY_EVENTS);
    ApplicationSettings.LOG_RENDERED_SCREEN = getBooleanSetting(R.string.PREF_KEY_LOG_RENDERED_SCREEN);
    ApplicationSettings.LOG_KEYBOARD_EVENTS = getBooleanSetting(R.string.PREF_KEY_LOG_KEYBOARD_EVENTS);
    ApplicationSettings.LOG_UNHANDLED_EVENTS = getBooleanSetting(R.string.PREF_KEY_LOG_UNHANDLED_EVENTS);
  }

  private final void setOverrideDirectories () {
    StringBuilder paths = new StringBuilder();

    File[] directories = new File[] {
      Environment.getExternalStorageDirectory()
    };

    for (File directory : directories) {
      String path = directory.getAbsolutePath();
      if (paths.length() > 0) paths.append(':');
      paths.append(path);
    }

    CoreWrapper.setEnvironmentVariable("XDG_CONFIG_DIRS", paths.toString());
  }

  public CoreThread (Context context) {
    super("Core");
    coreContext = context;

    restoreSettings();
    setOverrideDirectories();
  }
}

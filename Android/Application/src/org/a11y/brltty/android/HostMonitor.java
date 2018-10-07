package org.a11y.brltty.android;

import android.util.Log;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import android.preference.PreferenceManager;

import java.io.IOException;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.channels.FileChannel;

public class HostMonitor extends BroadcastReceiver {
  private final static String LOG_TAG = HostMonitor.class.getName();

  private static void migrateData (File from, File to) {
    if (from.exists()) {
      if (from.isDirectory()) {
        if (!to.exists()) {
          if (!to.mkdir()) {
            Log.w(LOG_TAG, ("directory not created: " + to.getAbsolutePath()));
            return;
          }
        }

        for (String name : from.list()) {
          migrateData(
            new File(from, name),
            new File(to, name)
          );
        }
      } else if (!to.exists()) {
        try {
          FileChannel input = new FileInputStream(from).getChannel();

          try {
            FileChannel output = new FileOutputStream(to).getChannel();

            try {
              input.transferTo(0, input.size(), output);
            } finally {
              output.close();
              output = null;
            }
          } finally {
            input.close();
            input = null;
          }
        } catch (IOException exception) {
          Log.w(LOG_TAG, ("data migration error: " + exception.getMessage()));
        }
      }

      from.delete();
    }
  }

  private static void migrateData (Context fromContext) {
    Context toContext = DataType.getContext();

    if (toContext != fromContext) {
      toContext.moveSharedPreferencesFrom(
        fromContext,
        PreferenceManager.getDefaultSharedPreferencesName(fromContext)
      );

      for (DataType type : DataType.values()) {
        migrateData(
          fromContext.getDir(type.getName(), Context.MODE_PRIVATE),
          type.getDirectory()
        );
      }
    }
  }

  @Override
  public void onReceive (Context context, Intent intent) {
    String action = intent.getAction();
    if (action == null) return;
    Log.d(LOG_TAG, ("host event: " + action));

    if (ApplicationUtilities.haveNougat) {
      if (action.equals(Intent.ACTION_LOCKED_BOOT_COMPLETED)) {
        return;
      }
    }

    if (action.equals(Intent.ACTION_MY_PACKAGE_REPLACED)) {
      migrateData(context);
      return;
    }
  }
}

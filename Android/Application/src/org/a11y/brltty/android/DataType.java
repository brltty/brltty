package org.a11y.brltty.android;

import java.io.File;
import android.content.Context;

import android.preference.PreferenceManager;
import android.content.SharedPreferences;

public enum DataType {
  TABLES,
  DRIVERS,
  STATE,
  WRITABLE,
  ;

  private final String typeName;

  DataType () {
    typeName = name().toLowerCase();
  }

  public final String getName () {
    return typeName;
  }

  private final static Object DATA_CONTEXT_LOCK = new Object();
  private static Context dataContext = null;

  public static Context getContext () {
    synchronized (DATA_CONTEXT_LOCK) {
      if (dataContext == null) {
        Context context = ApplicationContext.get();

        if (ApplicationUtilities.haveNougat) {
          dataContext = context.createDeviceProtectedStorageContext();
        } else {
          dataContext = context;
        }
      }
    }

    return dataContext;
  }

  public static SharedPreferences getPreferences () {
    return PreferenceManager.getDefaultSharedPreferences(getContext());
  }

  private final static Integer DIRECTORY_MODE = Context.MODE_PRIVATE;
  private File directoryObject;

  public final File getDirectory () {
    synchronized (DIRECTORY_MODE) {
      if (directoryObject == null) {
        directoryObject = getContext().getDir(typeName, DIRECTORY_MODE);
      }
    }

    return directoryObject;
  }
}

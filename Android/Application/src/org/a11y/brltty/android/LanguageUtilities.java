/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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

import java.lang.reflect.*;

import android.util.Log;

public final class LanguageUtilities {
  private final static String LOG_TAG = LanguageUtilities.class.getName();

  public static String[] newStringArray (CharSequence[] charSequences) {
    int size = charSequences.length;
    String[] strings = new String[size];

    for (int i=0; i<size; i+=1) {
      strings[i] = charSequences[i].toString();
    }

    return strings;
  }

  public static Object newInstance (String instanceType, String[] argumentTypes, Object[] arguments) {
    Class instanceClass;
    try {
      instanceClass = Class.forName(instanceType);
    } catch (ClassNotFoundException exception) {
      Log.w(LOG_TAG, "class not found: " + instanceType);
      return null;
    }

    Class[] argumentClasses = new Class[argumentTypes.length];
    for (int i=0; i<argumentTypes.length; i+=1) {
      String type = argumentTypes[i];

      if (type == null) {
        argumentClasses[i] = arguments[i].getClass();
      } else {
        try {
          argumentClasses[i] = Class.forName(type);
        } catch (ClassNotFoundException exception) {
          Log.w(LOG_TAG, "class not found: " + type);
          return null;
        }
      }
    }

    Constructor constructor;
    try {
      constructor = instanceClass.getConstructor(argumentClasses);
    } catch (NoSuchMethodException exception) {
      Log.w(LOG_TAG, "constructor not found: " + instanceType);
      return null;
    }

    Object instance;
    try {
      instance = constructor.newInstance(arguments);
    } catch (java.lang.InstantiationException exception) {
      Log.w(LOG_TAG, "uninstantiatable class: " + instanceType);
      return null;
    } catch (IllegalAccessException exception) {
      Log.w(LOG_TAG, "inaccessible constructor: " + instanceType);
      return null;
    } catch (InvocationTargetException exception) {
      Log.w(LOG_TAG, "construction failed: " + instanceType);
      return null;
    }

    return instance;
  }

  public static Object newInstance (String instanceType) {
    return newInstance(instanceType, new String[0], new Object[0]);
  }

  public static boolean canAssign (Class target, Class source) {
    return target.isAssignableFrom(source);
  }

  public static boolean canAssign (Class target, String source) {
    try {
      return canAssign(target, Class.forName(source));
    } catch (ClassNotFoundException exception) {
    }

    return false;
  }

  public static int compare (int value1, int value2) {
    if (value1 < value2) return -1;
    if (value1 > value2) return 1;
    return 0;
  }

  private LanguageUtilities () {
  }
}

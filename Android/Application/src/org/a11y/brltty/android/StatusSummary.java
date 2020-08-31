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

import java.util.Date;
import java.text.SimpleDateFormat;
import android.text.format.DateFormat;

import android.content.Context;
import android.os.BatteryManager;

public abstract class StatusSummary {
  private StatusSummary () {
  }

  private static Context getContext () {
    return BrailleApplication.get();
  }

  private static Object getService (String name) {
    return getContext().getSystemService(name);
  }

  private interface StatusGetter {
    public String getLabel ();
    public String getValue ();
  }

  private final static StatusGetter TIME =
    new StatusGetter() {
      @Override
      public String getLabel () {
        return null;
      }

      @Override
      public String getValue () {
        Context context = getContext();
        String format = DateFormat.is24HourFormat(context)? "HH:mm": "h:mma";
        return new SimpleDateFormat(format).format(new Date());
      }
    };

  private final static StatusGetter BATTERY =
    new StatusGetter() {
      private final int INT_NO_VALUE = 0;
      private final long LONG_NO_VALUE = Long.MIN_VALUE;

      @Override
      public String getLabel () {
        return "Bat";
      }

      @Override
      public String getValue () {
        BatteryManager bm = (BatteryManager)getService(Context.BATTERY_SERVICE);

        if (APITests.haveLollipop) {
          int value = bm.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY);

          if (value != INT_NO_VALUE) {
            return Integer.toString(value) + '%';
          }
        }

        return null;
      }
    };

  private final static StatusGetter statusGetters[] = {
    TIME, BATTERY
  };

  public static String get () {
    StringBuilder status = new StringBuilder();

    for (StatusGetter getter : statusGetters) {
      String value = getter.getValue();
      if (value == null) continue;
      if (value.isEmpty()) continue;
      if (status.length() > 0) status.append(' ');

      String label = getter.getLabel();
      if (!((label == null) || label.isEmpty())) status.append(label).append(':');

      status.append(value);
    }

    if (status.length() == 0) return null;
    return status.toString();
  }
}

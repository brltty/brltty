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

import java.util.List;

import java.util.Date;
import java.text.SimpleDateFormat;
import android.text.format.DateFormat;

import android.content.Context;
import android.os.BatteryManager;

import android.telephony.TelephonyManager;
import android.telephony.CellInfo;
import android.telephony.CellInfoCdma;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellSignalStrength;

import android.net.wifi.WifiManager;
import android.net.wifi.WifiInfo;

public abstract class StatusSummary {
  private StatusSummary () {
  }

  private final static Context context = BrailleApplication.get();

  private final static BatteryManager batteryManager = (BatteryManager)
          context.getSystemService(Context.BATTERY_SERVICE);

  private final static TelephonyManager telephonyManager = (TelephonyManager)
          context.getSystemService(Context.TELEPHONY_SERVICE);

  private final static WifiManager wifiManager = (WifiManager)
          context.getSystemService(Context.WIFI_SERVICE);

  public interface Getter {
    public String getLabel ();
    public String getValue ();
  }

  private static class Group {
    private final Getter groupGetters[];

    public Group (Getter... getters) {
      groupGetters = getters;
    }

    public final Getter[] getGetters () {
      return groupGetters;
    }
  }

  public final static Getter DEVICE_TIME = new Getter() {
    @Override
    public String getLabel () {
      return null;
    }

    @Override
    public String getValue () {
      String format = DateFormat.is24HourFormat(context)? "HH:mm": "h:mma";
      return new SimpleDateFormat(format).format(new Date());
    }
  };

  public final static Getter DEVICE_BATTERY = new Getter() {
    private final int INT_NO_VALUE = APITests.havePie? Integer.MIN_VALUE: 0;
    private final long LONG_NO_VALUE = Long.MIN_VALUE;

    @Override
    public String getLabel () {
      return "Bat";
    }

    @Override
    public String getValue () {
      if (APITests.haveLollipop) {
        int value = batteryManager.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY);

        if (value != INT_NO_VALUE) {
          return Integer.toString(value) + '%';
        }
      }

      return null;
    }
  };

  public final static Getter CELL_OPERATOR = new Getter() {
    @Override
    public String getLabel () {
      return "Cell";
    }

    @Override
    public String getValue () {
      String name = telephonyManager.getNetworkOperatorName();
      return name;
    }
  };

  public final static Getter CELL_SIGNAL = new Getter() {
    @Override
    public String getLabel () {
      return null;
    }

    @Override
    public String getValue () {
      if (APITests.haveJellyBeanMR1) {
        List<CellInfo> infoList = telephonyManager.getAllCellInfo();

        if (infoList != null) {
          CellInfo info = infoList.get(0);
          CellSignalStrength css = null;

          if (info instanceof CellInfoCdma) {
            css = ((CellInfoCdma)info).getCellSignalStrength();
          } else if (info instanceof CellInfoGsm) {
            css = ((CellInfoGsm)info).getCellSignalStrength();
          } else if (info instanceof CellInfoLte) {
            css = ((CellInfoLte)info).getCellSignalStrength();
          }

          if (css != null) {
            int level = css.getLevel();
            return Integer.toString((level * 100) / 4) + '%';
          }
        }
      }

      return null;
    }
  };

  private static WifiInfo wifiInfo = null;

  public final static Getter WIFI_SSID = new Getter() {
    @Override
    public String getLabel () {
      return "WiFi";
    }

    @Override
    public String getValue () {
      wifiInfo = wifiManager.getConnectionInfo();

      if (wifiInfo != null) {
        String ssid = wifiInfo.getSSID();
        if ((ssid != null) && !ssid.isEmpty()) return ssid;
      }

      return null;
    }
  };

  public final static Getter WIFI_SIGNAL = new Getter() {
    @Override
    public String getLabel () {
      return null;
    }

    @Override
    public String getValue () {
      if (wifiInfo != null) {
        int rssi = wifiInfo.getRssi();
        int percentage = wifiManager.calculateSignalLevel(rssi, 100);
        return Integer.toString(percentage) + '%';
      }

      return null;
    }
  };

  public final static Getter WIFI_SPEED = new Getter() {
    @Override
    public String getLabel () {
      return null;
    }

    @Override
    public String getValue () {
      if (wifiInfo != null) {
        int speed = wifiInfo.getLinkSpeed();
        return Integer.toString(speed) + WifiInfo.LINK_SPEED_UNITS;
      }

      return null;
    }
  };

  public final static Getter WIFI_FREQUENCY = new Getter() {
    @Override
    public String getLabel () {
      return null;
    }

    @Override
    public String getValue () {
      if (APITests.haveLollipop) {
        if (wifiInfo != null) {
          int frequency = wifiInfo.getFrequency();
          return Integer.toString(frequency) + WifiInfo.FREQUENCY_UNITS;
        }
      }

      return null;
    }
  };

  private final static Group GROUP_DEVICE = new Group(
    DEVICE_TIME
  , DEVICE_BATTERY
  );

  private final static Group GROUP_CELL = new Group(
    CELL_OPERATOR
  , CELL_SIGNAL
  );

  private final static Group GROUP_WIFI = new Group(
    WIFI_SSID
  , WIFI_SIGNAL
  , WIFI_SPEED
  , WIFI_FREQUENCY
  );

  private final static Group GROUPS[] = {
    GROUP_DEVICE
  , GROUP_CELL
  , GROUP_WIFI
  };

  public static String get () {
    StringBuilder status = new StringBuilder();

    for (Group group : GROUPS) {
      boolean first = true;

      for (Getter getter : group.getGetters()) {
        String value = getter.getValue();

        if ((value == null) || value.isEmpty()) {
          if (first) break;
          continue;
        }

        if (first) {
          first = false;
          if (status.length() > 0) status.append(',');
        }

        if (status.length() > 0) status.append(' ');
        String label = getter.getLabel();
        if ((label != null) && !label.isEmpty()) status.append(label).append(':');
        status.append(value);
      }
    }

    if (status.length() == 0) return null;
    return status.toString();
  }
}

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

import android.util.Log;
import android.annotation.SuppressLint;

import android.content.Context;
import java.util.Locale;

import android.os.BatteryManager;

import java.util.Date;
import java.text.SimpleDateFormat;
import android.text.format.DateFormat;

import android.telephony.TelephonyManager;
import android.telephony.CellSignalStrength;
import android.telephony.CellInfo;
import android.telephony.CellInfoCdma;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoNr;
import android.telephony.CellInfoTdscdma;
import android.telephony.CellInfoWcdma;

import android.net.wifi.WifiManager;
import android.net.wifi.WifiInfo;

import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.HashMap;

public abstract class StatusIndicators {
  private final static String LOG_TAG = StatusIndicators.class.getName();

  private StatusIndicators () {
  }

  private final static Context context = BrailleApplication.get();

  private final static TelephonyManager telephonyManager = (TelephonyManager)
          context.getSystemService(Context.TELEPHONY_SERVICE);

  private final static WifiManager wifiManager = (WifiManager)
          context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);

  public interface Item {
    public String getLabel ();
    public String getValue ();
  }

  public static class Group {
    private final Item[] groupItems;

    public final Item[] getItems () {
      Item[] array = groupItems;
      return Arrays.copyOf(array, array.length);
    }

    public Group (Item... items) {
      groupItems = items;
    }

    public boolean prepare () {
      return true;
    }
  }

  private static class DeviceGroup extends Group {
    public final static Item TIME = new Item() {
      @Override
      public String getLabel () {
        return null;
      }

      @SuppressLint("ConstantLocale")
      @Override
      public String getValue () {
        String format = DateFormat.is24HourFormat(context)? "HH:mm": "h:mma";
        return new SimpleDateFormat(format, Locale.getDefault()).format(new Date());
      }
    };

    public final static Item BATTERY = new Item() {
      private final int INT_NO_VALUE = APITests.havePie? Integer.MIN_VALUE: 0;
      private final long LONG_NO_VALUE = Long.MIN_VALUE;

      @Override
      public String getLabel () {
        return "Bat";
      }

      @Override
      public String getValue () {
        if (APITests.haveLollipop) {
          BatteryManager batteryManager = (BatteryManager)
            context.getSystemService(Context.BATTERY_SERVICE);

          int value = batteryManager.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY);

          if (value != INT_NO_VALUE) {
            return Integer.toString(value) + '%';
          }
        }

        return null;
      }
    };

    public DeviceGroup () {
      super(TIME, BATTERY);
    }
  }

  private static class CellGroup extends Group {
    public final static Item OPERATOR = new Item() {
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

    public final static Item SIGNAL = new Item() {
      @Override
      public String getLabel () {
        return "Sig";
      }

      private final CellInfo getCellInfo () {
        List<CellInfo> infoList = null;

        if (APITests.haveJellyBeanMR1) {
          try {
            infoList = telephonyManager.getAllCellInfo();
          } catch (SecurityException exception) {
            Log.w(LOG_TAG, ("security exception: " + exception.getMessage()));
          }
        }

        if (infoList != null) {
          if (!infoList.isEmpty()) {
            return infoList.get(0);
          }
        }

        return null;
      }

      private final CellSignalStrength getCellSignalStrength (CellInfo info) {
        if (APITests.haveJellyBeanMR1) {
          if (info instanceof CellInfoCdma) {
            return ((CellInfoCdma)info).getCellSignalStrength();
          }

          if (info instanceof CellInfoGsm) {
            return ((CellInfoGsm)info).getCellSignalStrength();
          }

          if (info instanceof CellInfoLte) {
            return ((CellInfoLte)info).getCellSignalStrength();
          }
        }

        if (APITests.haveJellyBeanMR2) {
          if (info instanceof CellInfoWcdma) {
            return ((CellInfoWcdma)info).getCellSignalStrength();
          }
        }

        if (APITests.haveQ) {
          if (info instanceof CellInfoNr) {
            return ((CellInfoNr)info).getCellSignalStrength();
          }

          if (info instanceof CellInfoTdscdma) {
            return ((CellInfoTdscdma)info).getCellSignalStrength();
          }
        }

        return null;
      }

      private final String[] levelNames = new String[] {
        "none", "poor", "fair", "good", "high"
      };

      @Override
      public String getValue () {
        if (APITests.haveJellyBeanMR1) {
          CellInfo info = getCellInfo();

          if (info != null) {
            CellSignalStrength css = getCellSignalStrength(info);

            if (css != null) {
              int level = css.getLevel();
              return levelNames[level];
            }
          }
        }

        return null;
      }
    };

    public final static Item TECHNOLOGY = new Item() {
      private final Map<Integer, String> networkTypes = new HashMap<>();

      {
        networkTypes.put(TelephonyManager.NETWORK_TYPE_1xRTT, "1xRTT");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_CDMA, "CDMA");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_EDGE, "EDGE");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_EHRPD, "eHRPD");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_EVDO_0, "EVDO revision 0");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_EVDO_A, "EVDO revision A");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_EVDO_B, "EVDO revision B");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_GPRS, "GPRS");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_HSDPA, "HSDPA");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_HSPA, "HSPA");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_HSPAP, "HSPA+");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_HSUPA, "HSUPA");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_IDEN, "iDen");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_LTE, "LTE");
        networkTypes.put(TelephonyManager.NETWORK_TYPE_UMTS, "UMTS");
      }

      @Override
      public String getLabel () {
        return null;
      }

      @Override
      public String getValue () {
        int type = TelephonyManager.NETWORK_TYPE_UNKNOWN;

        try {
          type = telephonyManager.getNetworkType();
        } catch (SecurityException exception) {
        }

        if (type == TelephonyManager.NETWORK_TYPE_UNKNOWN) return null;
        return networkTypes.get(type);
      }
    };

    public final static Item ROAMING = new Item() {
      @Override
      public String getLabel () {
        return null;
      }

      @Override
      public String getValue () {
        return telephonyManager.isNetworkRoaming()? "roaming": null;
      }
    };

    public CellGroup () {
      super(OPERATOR, SIGNAL, TECHNOLOGY, ROAMING);
    }
  }

  private static class WifiGroup extends Group {
    private static WifiInfo wifiInfo = null;

    @Override
    public boolean prepare () {
      if (!super.prepare()) return false;

      try {
        wifiInfo = wifiManager.getConnectionInfo();
      } catch (SecurityException exception) {
        wifiInfo = null;
      }

      return wifiInfo != null;
    }

    public final static Item SSID = new Item() {
      @Override
      public String getLabel () {
        return "WiFi";
      }

      @Override
      public String getValue () {
        if (wifiInfo != null) {
          String ssid = wifiInfo.getSSID();
          if ((ssid != null) && !ssid.isEmpty()) return ssid;
        }

        return null;
      }
    };

    public final static Item SIGNAL = new Item() {
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

    public final static Item SPEED = new Item() {
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

    public final static Item FREQUENCY = new Item() {
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

    public WifiGroup () {
      super(SSID, SIGNAL, SPEED, FREQUENCY);
    }
  }

  public final static Group DEVICE = new DeviceGroup();
  public final static Group CELL = new CellGroup();
  public final static Group WIFI = new WifiGroup();

  private final static Group[] GROUPS = {
    DEVICE, CELL, WIFI
  };

  public static String get () {
    StringBuilder status = new StringBuilder();

    for (Group group : GROUPS) {
      if (!group.prepare()) continue;
      boolean first = true;

      for (Item item : group.getItems()) {
        String value = item.getValue();

        if ((value == null) || value.isEmpty()) {
          if (first) break;
          continue;
        }

        if (first) {
          first = false;
          if (status.length() > 0) status.append(',');
        }

        if (status.length() > 0) status.append(' ');
        String label = item.getLabel();
        if ((label != null) && !label.isEmpty()) status.append(label).append(':');
        status.append(value);
      }
    }

    if (status.length() == 0) return null;
    return status.toString();
  }
}

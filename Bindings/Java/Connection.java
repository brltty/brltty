/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2020 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
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

package org.a11y.brlapi;

public class Connection extends BasicConnection implements Constants {
  public Connection (ConnectionSettings settings) {
    super(settings);
  }

  public int getCellCount () {
    DisplaySize size = getDisplaySize();
    return size.getWidth() * size.getHeight();
  }

  public int enterTtyMode (int tty) {
    return enterTtyMode(tty, null);
  }

  public int enterTtyMode (String driver) {
    return enterTtyMode(Constants.TTY_DEFAULT, driver);
  }

  public int enterTtyMode () {
    return enterTtyMode(null);
  }

  public void enterTtyModeWithPath (int[] ttys, String driver) {
    enterTtyModeWithPath(driver, ttys);
  }

  public void enterTtyModeWithPath (int... ttys) {
    enterTtyModeWithPath(null, ttys);
  }

  public void writeDots (byte[] dots) {
    int count = getCellCount();

    if (dots.length != count) {
      byte[] d = new byte[count];
      while (count > dots.length) d[--count] = 0;
      System.arraycopy(dots, 0, d, 0, count);
      dots = d;
    }

    super.writeDots(dots);
  }

  public void writeText (int cursor, String text) {
    if (text != null) {
      int count = getCellCount();

      {
        StringBuilder sb = new StringBuilder(text);
        while (sb.length() < count) sb.append(' ');
        text = sb.toString();
      }

      text = text.substring(0, count);
    }

    super.writeText(cursor, text);
  }

  public void writeText (String text, int cursor) {
    writeText(cursor, text);
  }

  public void writeText (int cursor) {
    writeText(cursor, null);
  }

  public void writeText (String text) {
    writeText(Constants.CURSOR_OFF, text);
  }

  public Object getLocalParameter (int parameter, long subparam) {
    return getParameter(parameter, subparam, false);
  }

  public Object getLocalParameter (int parameter) {
    return getLocalParameter(parameter, 0);
  }

  public Object getGlobalParameter (int parameter, long subparam) {
    return getParameter(parameter, subparam, true);
  }

  public Object getGlobalParameter (int parameter) {
    return getGlobalParameter(parameter, 0);
  }

  public void setLocalParameter (int parameter, long subparam, Object value) {
    setParameter(parameter, subparam, false, value);
  }

  public void setLocalParameter (int parameter, Object value) {
    setLocalParameter(parameter, 0, value);
  }

  public void setGlobalParameter (int parameter, long subparam, Object value) {
    setParameter(parameter, subparam, true, value);
  }

  public void setGlobalParameter (int parameter, Object value) {
    setGlobalParameter(parameter, 0, value);
  }

  public long watchLocalParameter (int parameter, long subparam, ParameterWatcher watcher) {
    return watchParameter(parameter, subparam, false, watcher);
  }

  public long watchLocalParameter (int parameter, ParameterWatcher watcher) {
    return watchLocalParameter(parameter, 0, watcher);
  }

  public long watchGlobalParameter (int parameter, long subparam, ParameterWatcher watcher) {
    return watchParameter(parameter, subparam, true, watcher);
  }

  public long watchGlobalParameter (int parameter, ParameterWatcher watcher) {
    return watchGlobalParameter(parameter, 0, watcher);
  }

  public int getServerVersion () {
    return Parameter.toInt(getGlobalParameter(PARAM_SERVER_VERSION));
  }

  public int getClientPriority () {
    return Parameter.toInt(getLocalParameter(PARAM_CLIENT_PRIORITY));
  }

  public void setClientPriority (int priority) {
    setLocalParameter(
      PARAM_CLIENT_PRIORITY,
      new int[] {priority}
    );
  }

  public String getDriverName () {
    return Parameter.toString(getGlobalParameter(PARAM_DRIVER_NAME));
  }

  public String getDriverCode () {
    return Parameter.toString(getGlobalParameter(PARAM_DRIVER_CODE));
  }

  public String getDriverVersion () {
    return Parameter.toString(getGlobalParameter(PARAM_DRIVER_VERSION));
  }

  public String getDeviceModel () {
    return Parameter.toString(getGlobalParameter(PARAM_DEVICE_MODEL));
  }

  public byte getDeviceCellSize () {
    return Parameter.toByte(getGlobalParameter(PARAM_DEVICE_CELL_SIZE));
  }

  public DisplaySize getDisplaySize () {
    return Parameter.toDisplaySize(getGlobalParameter(PARAM_DISPLAY_SIZE));
  }

  public String getDeviceIdentifier () {
    return Parameter.toString(getGlobalParameter(PARAM_DEVICE_IDENTIFIER));
  }

  public int getDeviceSpeed () {
    return Parameter.toInt(getGlobalParameter(PARAM_DEVICE_SPEED));
  }

  public boolean getDeviceOnline () {
    return Parameter.toBoolean(getGlobalParameter(PARAM_DEVICE_ONLINE));
  }

  public boolean getRetainDots () {
    return Parameter.toBoolean(getLocalParameter(PARAM_RETAIN_DOTS));
  }

  public void setRetainDots (boolean yes) {
    setLocalParameter(
      PARAM_RETAIN_DOTS,
      new boolean[] {yes}
    );
  }

  public byte getComputerBrailleCellSize () {
    return Parameter.toByte(getGlobalParameter(PARAM_COMPUTER_BRAILLE_CELL_SIZE));
  }

  public void setComputerBrailleCellSize (byte size) {
    setGlobalParameter(
      PARAM_COMPUTER_BRAILLE_CELL_SIZE,
      new byte[] {size}
    );
  }

  public boolean getLiteraryBraille () {
    return Parameter.toBoolean(getGlobalParameter(PARAM_LITERARY_BRAILLE));
  }

  public void setLiteraryBraille (boolean yes) {
    setGlobalParameter(
      PARAM_LITERARY_BRAILLE,
      new boolean[] {yes}
    );
  }

  public byte getCursorDots () {
    return Parameter.toByte(getGlobalParameter(PARAM_CURSOR_DOTS));
  }

  public void setCursorDots (byte dots) {
    setGlobalParameter(
      PARAM_CURSOR_DOTS,
      new byte[] {dots}
    );
  }

  public int getCursorBlinkPeriod () {
    return Parameter.toInt(getGlobalParameter(PARAM_CURSOR_BLINK_PERIOD));
  }

  public void setCursorBlinkPeriod (int period) {
    setGlobalParameter(
      PARAM_CURSOR_BLINK_PERIOD,
      new int[] {period}
    );
  }

  public byte getCursorBlinkPercentage () {
    return Parameter.toByte(getGlobalParameter(PARAM_CURSOR_BLINK_PERCENTAGE));
  }

  public void setCursorBlinkPercentage (byte percentage) {
    setGlobalParameter(
      PARAM_CURSOR_BLINK_PERCENTAGE,
      new byte[] {percentage}
    );
  }

  public byte[] getRenderedCells () {
    return Parameter.toByteArray(getLocalParameter(PARAM_RENDERED_CELLS));
  }

  public boolean getSkipIdenticalLines () {
    return Parameter.toBoolean(getGlobalParameter(PARAM_SKIP_IDENTICAL_LINES));
  }

  public void setSkipIdenticalLines (boolean yes) {
    setGlobalParameter(
      PARAM_SKIP_IDENTICAL_LINES,
      new boolean[] {yes}
    );
  }

  public boolean getAudibleAlerts () {
    return Parameter.toBoolean(getGlobalParameter(PARAM_AUDIBLE_ALERTS));
  }

  public void setAudibleAlerts (boolean yes) {
    setGlobalParameter(
      PARAM_AUDIBLE_ALERTS,
      new boolean[] {yes}
    );
  }

  public String getClipboardContent () {
    return Parameter.toString(getGlobalParameter(PARAM_CLIPBOARD_CONTENT));
  }

  public void setClipboardContent (String text) {
    setGlobalParameter(PARAM_CLIPBOARD_CONTENT, text);
  }

  public long[] getBoundCommandCodes () {
    return Parameter.toLongArray(getGlobalParameter(PARAM_BOUND_COMMAND_CODES));
  }

  public String getCommandShortName (long code) {
    return Parameter.toString(getGlobalParameter(PARAM_COMMAND_SHORT_NAME, code));
  }

  public String getCommandLongName (long code) {
    return Parameter.toString(getGlobalParameter(PARAM_COMMAND_LONG_NAME, code));
  }

  public long[] getDeviceKeyCodes () {
    return Parameter.toLongArray(getGlobalParameter(PARAM_DEVICE_KEY_CODES));
  }

  public String getKeyShortName (long code) {
    return Parameter.toString(getGlobalParameter(PARAM_KEY_SHORT_NAME, code));
  }

  public String getKeyLongName (long code) {
    return Parameter.toString(getGlobalParameter(PARAM_KEY_LONG_NAME, code));
  }

  public BitMask getComputerBrailleRowsMask () {
    return Parameter.toBitMask(getGlobalParameter(PARAM_COMPUTER_BRAILLE_ROWS_MASK));
  }

  public RowCells getComputerBrailleRowCells (long row) {
    return Parameter.toRowCells(getGlobalParameter(PARAM_COMPUTER_BRAILLE_ROW_CELLS, row));
  }

  public String getComputerBrailleTable () {
    return Parameter.toString(getGlobalParameter(PARAM_COMPUTER_BRAILLE_TABLE));
  }

  public void setComputerBrailleTable (String name) {
    setGlobalParameter(PARAM_COMPUTER_BRAILLE_TABLE, name);
  }

  public String getLiteraryBrailleTable () {
    return Parameter.toString(getGlobalParameter(PARAM_LITERARY_BRAILLE_TABLE));
  }

  public void setLiteraryBrailleTable (String name) {
    setGlobalParameter(PARAM_LITERARY_BRAILLE_TABLE, name);
  }

  public String getMessageLocale () {
    return Parameter.toString(getGlobalParameter(PARAM_MESSAGE_LOCALE));
  }

  public void setMessageLocale (String locale) {
    setGlobalParameter(PARAM_MESSAGE_LOCALE, locale);
  }
}

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

  public void enterTtyModeWithPath (int... ttys) {
    enterTtyModeWithPath(ttys, null);
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

  public Object getParameter (int parameter, boolean global) {
    return getParameter(parameter, 0, global);
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

  public int getServerVersion () {
    return ((int[])getGlobalParameter(PARAM_SERVER_VERSION))[0];
  }

  public int getClientPriority () {
    return ((int[])getLocalParameter(PARAM_CLIENT_PRIORITY))[0];
  }

  public String getDriverName () {
    return (String)getGlobalParameter(PARAM_DRIVER_NAME);
  }

  public String getDriverCode () {
    return (String)getGlobalParameter(PARAM_DRIVER_CODE);
  }

  public String getDriverVersion () {
    return (String)getGlobalParameter(PARAM_DRIVER_VERSION);
  }

  public String getDeviceModel () {
    return (String)getGlobalParameter(PARAM_DEVICE_MODEL);
  }

  public byte getDeviceCellSize () {
    return ((byte[])getGlobalParameter(PARAM_DEVICE_CELL_SIZE))[0];
  }

  public DisplaySize getDisplaySize () {
    int[] dimensions = (int[])getGlobalParameter(PARAM_DISPLAY_SIZE);
    if (dimensions == null) return null;
    return new DisplaySize(dimensions);
  }

  public String getDeviceIdentifier () {
    return (String)getGlobalParameter(PARAM_DEVICE_IDENTIFIER);
  }

  public int getDeviceSpeed () {
    return ((int[])getGlobalParameter(PARAM_DEVICE_SPEED))[0];
  }

  public boolean getDeviceOnline () {
    return ((boolean[])getGlobalParameter(PARAM_DEVICE_ONLINE))[0];
  }

  public boolean getRetainDots () {
    return ((boolean[])getLocalParameter(PARAM_RETAIN_DOTS))[0];
  }

  public byte getComputerBrailleCellSize () {
    return ((byte[])getGlobalParameter(PARAM_COMPUTER_BRAILLE_CELL_SIZE))[0];
  }

  public boolean getLiteraryBraille () {
    return ((boolean[])getGlobalParameter(PARAM_LITERARY_BRAILLE))[0];
  }

  public byte getCursorDots () {
    return ((byte[])getGlobalParameter(PARAM_CURSOR_DOTS))[0];
  }

  public int getCursorBlinkPeriod () {
    return ((int[])getGlobalParameter(PARAM_CURSOR_BLINK_PERIOD))[0];
  }

  public byte getCursorBlinkPercentage () {
    return ((byte[])getGlobalParameter(PARAM_CURSOR_BLINK_PERCENTAGE))[0];
  }

  public byte[] getRenderedCells () {
    return (byte[])getLocalParameter(PARAM_RENDERED_CELLS);
  }

  public boolean getSkipIdenticalLines () {
    return ((boolean[])getGlobalParameter(PARAM_SKIP_IDENTICAL_LINES))[0];
  }

  public boolean getAudibleAlerts () {
    return ((boolean[])getGlobalParameter(PARAM_AUDIBLE_ALERTS))[0];
  }

  public String getClipboardContent () {
    return (String)getGlobalParameter(PARAM_CLIPBOARD_CONTENT);
  }

  public long[] getBoundCommandCodes () {
    return (long[])getGlobalParameter(PARAM_BOUND_COMMAND_CODES);
  }

  public String getCommandShortName (long code) {
    return (String)getGlobalParameter(PARAM_COMMAND_SHORT_NAME, code);
  }

  public String getCommandLongName (long code) {
    return (String)getGlobalParameter(PARAM_COMMAND_LONG_NAME, code);
  }

  public long[] getDeviceKeyCodes () {
    return (long[])getGlobalParameter(PARAM_DEVICE_KEY_CODES);
  }

  public String getKeyShortName (long code) {
    return (String)getGlobalParameter(PARAM_KEY_SHORT_NAME, code);
  }

  public String getKeyLongName (long code) {
    return (String)getGlobalParameter(PARAM_KEY_LONG_NAME, code);
  }

  public byte[] getComputerBrailleRowsMask () {
    return (byte[])getGlobalParameter(PARAM_COMPUTER_BRAILLE_ROWS_MASK);
  }

  public ComputerBrailleRowCells getComputerBrailleRowCells (long row) {
    byte[] bytes = (byte[])getGlobalParameter(PARAM_COMPUTER_BRAILLE_ROW_CELLS, row);
    if (bytes == null) return null;
    return new ComputerBrailleRowCells(bytes);
  }

  public String getComputerBrailleTable () {
    return (String)getGlobalParameter(PARAM_COMPUTER_BRAILLE_TABLE);
  }

  public String getLiteraryBrailleTable () {
    return (String)getGlobalParameter(PARAM_LITERARY_BRAILLE_TABLE);
  }

  public String getMessageLocale () {
    return (String)getGlobalParameter(PARAM_MESSAGE_LOCALE);
  }
}

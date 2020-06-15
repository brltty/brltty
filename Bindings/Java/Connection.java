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
import org.a11y.brlapi.parameters.*;

public class Connection extends BasicConnection {
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

  public static class Parameters {
    public final ServerVersionParameter serverVersion;
    public final ClientPriorityParameter clientPriority;
    public final DriverNameParameter driverName;
    public final DriverCodeParameter driverCode;
    public final DriverVersionParameter driverVersion;
    public final DeviceModelParameter deviceModel;
    public final DeviceCellSizeParameter deviceCellSize;
    public final DisplaySizeParameter displaySize;
    public final DeviceIdentifierParameter deviceIdentifier;
    public final DeviceSpeedParameter deviceSpeed;
    public final DeviceOnlineParameter deviceOnline;
    public final RetainDotsParameter retainDots;
    public final ComputerBrailleCellSizeParameter computerBrailleCellSize;
    public final LiteraryBrailleParameter literaryBraille;
    public final CursorDotsParameter cursorDots;
    public final CursorBlinkPeriodParameter cursorBlinkPeriod;
    public final CursorBlinkPercentageParameter cursorBlinkPercentage;
    public final RenderedCellsParameter renderedCells;
    public final SkipIdenticalLinesParameter skipIdenticalLines;
    public final AudibleAlertsParameter audibleAlerts;
    public final ClipboardContentParameter clipboardContent;
    public final BoundCommandCodesParameter boundCommandCodes;
    public final CommandShortNameParameter commandShortName;
    public final CommandLongNameParameter commandLongName;
    public final DeviceKeyCodesParameter deviceKeyCodes;
    public final KeyShortNameParameter keyShortName;
    public final KeyLongNameParameter keyLongName;
    public final ComputerBrailleRowsMaskParameter computerBrailleRowsMask;
    public final ComputerBrailleRowCellsParameter computerBrailleRowCells;
    public final ComputerBrailleTableParameter computerBrailleTable;
    public final LiteraryBrailleTableParameter literaryBrailleTable;
    public final MessageLocaleParameter messageLocale;

    private Parameters (Connection connection) {
      serverVersion = new ServerVersionParameter(connection);
      clientPriority = new ClientPriorityParameter(connection);
      driverName = new DriverNameParameter(connection);
      driverCode = new DriverCodeParameter(connection);
      driverVersion = new DriverVersionParameter(connection);
      deviceModel = new DeviceModelParameter(connection);
      deviceCellSize = new DeviceCellSizeParameter(connection);
      displaySize = new DisplaySizeParameter(connection);
      deviceIdentifier = new DeviceIdentifierParameter(connection);
      deviceSpeed = new DeviceSpeedParameter(connection);
      deviceOnline = new DeviceOnlineParameter(connection);
      retainDots = new RetainDotsParameter(connection);
      computerBrailleCellSize = new ComputerBrailleCellSizeParameter(connection);
      literaryBraille = new LiteraryBrailleParameter(connection);
      cursorDots = new CursorDotsParameter(connection);
      cursorBlinkPeriod = new CursorBlinkPeriodParameter(connection);
      cursorBlinkPercentage = new CursorBlinkPercentageParameter(connection);
      renderedCells = new RenderedCellsParameter(connection);
      skipIdenticalLines = new SkipIdenticalLinesParameter(connection);
      audibleAlerts = new AudibleAlertsParameter(connection);
      clipboardContent = new ClipboardContentParameter(connection);
      boundCommandCodes = new BoundCommandCodesParameter(connection);
      commandShortName = new CommandShortNameParameter(connection);
      commandLongName = new CommandLongNameParameter(connection);
      deviceKeyCodes = new DeviceKeyCodesParameter(connection);
      keyShortName = new KeyShortNameParameter(connection);
      keyLongName = new KeyLongNameParameter(connection);
      computerBrailleRowsMask = new ComputerBrailleRowsMaskParameter(connection);
      computerBrailleRowCells = new ComputerBrailleRowCellsParameter(connection);
      computerBrailleTable = new ComputerBrailleTableParameter(connection);
      literaryBrailleTable = new LiteraryBrailleTableParameter(connection);
      messageLocale = new MessageLocaleParameter(connection);
    }
  }

  private Parameters connectionParameters = null;

  public Parameters getParameters () {
    synchronized (this) {
      if (connectionParameters == null) {
        connectionParameters = new Parameters(this);
      }
    }

    return connectionParameters;
  }
}

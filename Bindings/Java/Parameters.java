/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2023 by
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

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;

import java.util.Arrays;
import java.util.Comparator;

public class Parameters extends ParameterComponent {
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
  public final BoundCommandKeycodesParameter boundCommandKeycodes;
  public final CommandKeycodeNameParameter commandKeycodeName;
  public final CommandKeycodeSummaryParameter commandKeycodeSummary;
  public final DefinedDriverKeycodesParameter definedDriverKeycodes;
  public final DriverKeycodeNameParameter driverKeycodeName;
  public final DriverKeycodeSummaryParameter driverKeycodeSummary;
  public final ComputerBrailleRowsMaskParameter computerBrailleRowsMask;
  public final ComputerBrailleRowCellsParameter computerBrailleRowCells;
  public final ComputerBrailleTableParameter computerBrailleTable;
  public final LiteraryBrailleTableParameter literaryBrailleTable;
  public final MessageLocaleParameter messageLocale;

  public Parameters (ConnectionBase connection) {
    super();

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
    boundCommandKeycodes = new BoundCommandKeycodesParameter(connection);
    commandKeycodeName = new CommandKeycodeNameParameter(connection);
    commandKeycodeSummary = new CommandKeycodeSummaryParameter(connection);
    definedDriverKeycodes = new DefinedDriverKeycodesParameter(connection);
    driverKeycodeName = new DriverKeycodeNameParameter(connection);
    driverKeycodeSummary = new DriverKeycodeSummaryParameter(connection);
    computerBrailleRowsMask = new ComputerBrailleRowsMaskParameter(connection);
    computerBrailleRowCells = new ComputerBrailleRowCellsParameter(connection);
    computerBrailleTable = new ComputerBrailleTableParameter(connection);
    literaryBrailleTable = new LiteraryBrailleTableParameter(connection);
    messageLocale = new MessageLocaleParameter(connection);
  }

  private final Parameter[] newParameterArray () {
    Class<Parameters> type = Parameters.class;

    Field[] fields = type.getFields();
    int fieldCount = fields.length;

    Parameter[] parameters = new Parameter[fieldCount];
    int parameterCount = 0;

    for (Field field : fields) {
      int modifiers = field.getModifiers();
      if ((modifiers & Modifier.PUBLIC) == 0) continue;
      if ((modifiers & Modifier.FINAL) == 0) continue;
      if (!type.equals(field.getDeclaringClass())) continue;

      try {
        parameters[parameterCount] = (Parameter)field.get(this);
        parameterCount += 1;
      } catch (IllegalAccessException exception) {
        continue;
      }
    }

    Parameter[] result = new Parameter[parameterCount];
    System.arraycopy(parameters, 0, result, 0, parameterCount);
    return result;
  }

  private Parameter[] parameterArray = null;
  public final Parameter[] get () {
    synchronized (this) {
      if (parameterArray == null) {
        parameterArray = newParameterArray();
      }
    }

    int count = parameterArray.length;
    Parameter[] result = new Parameter[count];
    System.arraycopy(parameterArray, 0, result, 0, count);
    return result;
  }

  private final KeywordMap<Parameter> newParameterNameMap () {
    KeywordMap<Parameter> map = new KeywordMap<>();

    for (Parameter parameter : get()) {
      map.put(parameter.getName(), parameter);
    }

    return map;
  }

  private KeywordMap<Parameter> parameterNameMap = null;
  public final Parameter get (String name) {
    synchronized (this) {
      if (parameterNameMap == null) {
        parameterNameMap = newParameterNameMap();
      }
    }

    return parameterNameMap.get(name);
  }

  public static void sortByName (Parameter[] parameters) {
    Arrays.sort(parameters,
      (parameter1, parameter2) -> {
        return parameter1.getName().compareTo(parameter2.getName());
      }
    );
  }
}

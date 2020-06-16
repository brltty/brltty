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

public abstract class Parameter {
  private final BasicConnection clientConnection;
  private final int parameterValue;
  private final boolean isGlobal;

  protected Parameter (BasicConnection connection, int parameter, boolean global) {
    clientConnection = connection;
    parameterValue = parameter;
    isGlobal = global;
  }

  protected final Object getValue (long subparam) {
    return clientConnection.getParameter(parameterValue, subparam, isGlobal);
  }

  protected final Object getValue () {
    return getValue(0);
  }

  protected final void setValue (long subparam, Object value) {
    clientConnection.setParameter(parameterValue, subparam, isGlobal, value);
  }

  protected final void setValue (Object value) {
    setValue(0, value);
  }

  public final long watch (long subparam, ParameterWatcher watcher) {
    return clientConnection.watchParameter(parameterValue, subparam, isGlobal, watcher);
  }

  public final long watch (ParameterWatcher watcher) {
    return watch(0, watcher);
  }

  public static String toString (Object value) {
    return (String)value;
  }

  public static boolean[] toBooleanArray (Object value) {
    return (boolean[])value;
  }

  public static byte[] toByteArray (Object value) {
    return (byte[])value;
  }

  public static short[] toShortArray (Object value) {
    return (short[])value;
  }

  public static int[] toIntArray (Object value) {
    return (int[])value;
  }

  public static long[] toLongArray (Object value) {
    return (long[])value;
  }

  public static boolean toBoolean (Object value) {
    return toBooleanArray(value)[0];
  }

  public static byte toByte (Object value) {
    return toByteArray(value)[0];
  }

  public static short toShort (Object value) {
    return toShortArray(value)[0];
  }

  public static int toInt (Object value) {
    return toIntArray(value)[0];
  }

  public static long toLong (Object value) {
    return toLongArray(value)[0];
  }

  public static DisplaySize toDisplaySize (Object value) {
    return new DisplaySize(toIntArray(value));
  }

  public static BitMask toBitMask (Object value) {
    return new BitMask(toByteArray(value));
  }

  public static RowCells toRowCells (Object value) {
    return new RowCells(toByteArray(value));
  }
}

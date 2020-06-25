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

public abstract class Parameter extends ParameterHelper {
  private final BasicConnection clientConnection;

  protected Parameter (BasicConnection connection) {
    super();
    clientConnection = connection;
  }

  private String parameterLabel = null;
  public String getLabel () {
    synchronized (this) {
      if (parameterLabel == null) {
        parameterLabel = getClass()
                        .getSimpleName()
                        .replaceAll("Parameter$", "")
                        .replaceAll("(?<=.)(?=\\p{Upper})", " ")
                        ;
      }
    }

    return parameterLabel;
  }

  private String parameterName = null;
  public String getName () {
    synchronized (this) {
      if (parameterName == null) {
        parameterName = getLabel()
                       .replace(' ', '-')
                       .toLowerCase()
                       ;
      }
    }

    return parameterName;
  }

  public abstract int getParameter ();
  public abstract boolean isGlobal ();

  public Object get (long subparam) {
    return null;
  }

  public Object get () {
    return null;
  }

  public final static class WatcherHandle implements AutoCloseable {
    private long watchIdentifier;

    private WatcherHandle (long identifier) {
      watchIdentifier = identifier;
    }

    @Override
    public void close () {
      synchronized (this) {
        if (watchIdentifier != 0) {
          BasicConnection.unwatchParameter(watchIdentifier);
          watchIdentifier = 0;
        }
      }
    }
  }

  public final WatcherHandle watch (long subparam, ParameterWatcher watcher) {
    return new WatcherHandle(
      clientConnection.watchParameter(
        getParameter(), subparam, isGlobal(), watcher
      )
    );
  }

  public final WatcherHandle watch (ParameterWatcher watcher) {
    return watch(0, watcher);
  }

  protected final Object getValue (long subparam) {
    return clientConnection.getParameter(getParameter(), subparam, isGlobal());
  }

  public String toString (long subparam) {
    return toString(getValue(subparam));
  }

  protected final Object getValue () {
    return getValue(0);
  }

  @Override
  public String toString () {
    return toString(getValue());
  }

  protected final void setValue (long subparam, Object value) {
    clientConnection.setParameter(getParameter(), subparam, isGlobal(), value);
  }

  protected final void setValue (Object value) {
    setValue(0, value);
  }

  public interface Settable {
  }

  public interface StringSettable extends Settable {
    public void set (String value);
  }

  public interface BooleanSettable extends Settable {
    public void set (boolean value);
  }

  public interface ByteSettable extends Settable {
    public void set (byte value);
  }

  public interface ShortSettable extends Settable {
    public void set (short value);
  }

  public interface IntSettable extends Settable {
    public void set (int value);
  }

  public interface LongSettable extends Settable {
    public void set (long value);
  }
}

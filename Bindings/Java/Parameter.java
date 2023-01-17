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

public abstract class Parameter extends ParameterComponent {
  private final ConnectionBase clientConnection;

  protected Parameter (ConnectionBase connection) {
    super();
    clientConnection = connection;
  }

  private String parameterLabel = null;
  public String getLabel () {
    synchronized (this) {
      if (parameterLabel == null) {
        parameterLabel = Strings.wordify(
          Strings.replaceAll(getClass().getSimpleName(), "Parameter$", "")
        );
      }
    }

    return parameterLabel;
  }

  private String parameterName = null;
  public String getName () {
    synchronized (this) {
      if (parameterName == null) {
        parameterName = toOperandName(getLabel());
      }
    }

    return parameterName;
  }

  public abstract int getParameter ();
  public abstract boolean isGlobal ();

  public boolean isHidable () {
    return false;
  }

  protected final Object getValue (long subparam) {
    return clientConnection.getParameter(getParameter(), subparam, isGlobal());
  }

  protected final Object getValue () {
    return getValue(0);
  }

  protected final void setValue (long subparam, Object value) {
    clientConnection.setParameter(getParameter(), subparam, isGlobal(), value);
  }

  protected final void setValue (Object value) {
    setValue(0, value);
  }

  public Object get (long subparam) {
    return null;
  }

  public String toString (long subparam) {
    return toString(get(subparam));
  }

  public Object get () {
    return null;
  }

  @Override
  public String toString () {
    return toString(get());
  }

  public interface Settable {
  }

  public final boolean isSettable () {
    return this instanceof Settable;
  }

  public interface StringSettable extends Settable {
    public void set (String value) throws SyntaxException;
  }

  public interface BooleanSettable extends Settable {
    public void set (boolean value);
  }

  public interface ByteSettable extends Settable {
    public void set (byte value);

    public default byte getMinimum () {
      return 0;
    }

    public default byte getMaximum () {
      return Byte.MAX_VALUE;
    }
  }

  public interface ShortSettable extends Settable {
    public void set (short value);

    public default short getMinimum () {
      return 0;
    }

    public default short getMaximum () {
      return Short.MAX_VALUE;
    }
  }

  public interface IntSettable extends Settable {
    public void set (int value);

    public default int getMinimum () {
      return 0;
    }

    public default int getMaximum () {
      return Integer.MAX_VALUE;
    }
  }

  public interface LongSettable extends Settable {
    public void set (long value);

    public default long getMinimum () {
      return 0;
    }

    public default long getMaximum () {
      return Long.MAX_VALUE;
    }
  }

  protected final String getParseDescription () {
    return getName() + " value";
  }

  public void set (String value) throws OperandException {
    if (!isSettable()) {
      throw new SemanticException("parameter is not settable: %s", getName());
    }

    if (this instanceof StringSettable) {
      StringSettable settable = (StringSettable)this;
      settable.set(value);
      return;
    }

    if (this instanceof BooleanSettable) {
      BooleanSettable settable = (BooleanSettable)this;
      settable.set(Parse.asBoolean(getParseDescription(), value));
      return;
    }

    if (this instanceof ByteSettable) {
      ByteSettable settable = (ByteSettable)this;
      settable.set(Parse.asByte(getParseDescription(), value, settable.getMinimum(), settable.getMaximum()));
      return;
    }

    if (this instanceof ShortSettable) {
      ShortSettable settable = (ShortSettable)this;
      settable.set(Parse.asShort(getParseDescription(), value, settable.getMinimum(), settable.getMaximum()));
      return;
    }

    if (this instanceof IntSettable) {
      IntSettable settable = (IntSettable)this;
      settable.set(Parse.asInt(getParseDescription(), value, settable.getMinimum(), settable.getMaximum()));
      return;
    }

    if (this instanceof LongSettable) {
      LongSettable settable = (LongSettable)this;
      settable.set(Parse.asLong(getParseDescription(), value, settable.getMinimum(), settable.getMaximum()));
      return;
    }

    {
      Class<?> type = Settable.class;

      for (Class<?> i : getClass().getInterfaces()) {
        if (type.isAssignableFrom(i)) {
          type = i;
          break;
        }
      }

      throw new UnsupportedOperationException(
        String.format(
          "set not supported: %s: %s", getName(), type.getSimpleName()
        )
      );
    }
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
          ConnectionBase.unwatchParameter(watchIdentifier);
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
}

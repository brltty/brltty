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
  private final int parameterValue;
  private final boolean isGlobal;

  protected Parameter (BasicConnection connection, int parameter, boolean global) {
    super();

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

  public final static class WatchIdentifier implements AutoCloseable {
    private long watchIdentifier;

    public WatchIdentifier (long identifier) {
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

  public final WatchIdentifier watch (long subparam, ParameterWatcher watcher) {
    return new WatchIdentifier(clientConnection.watchParameter(parameterValue, subparam, isGlobal, watcher));
  }

  public final WatchIdentifier watch (ParameterWatcher watcher) {
    return watch(0, watcher);
  }
}

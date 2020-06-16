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

  public abstract int getParameter ();
  public abstract boolean isGlobal ();

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
}

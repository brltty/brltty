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

public class ConnectionSettings {
  public ConnectionSettings () {
  }

  public final static char AUTHORIZATION_SCHEME_DELIMITER = '+';
  public final static String AUTHORIZATION_SCHEME_NONE = "none";
  public final static String AUTHORIZATION_SCHEME_KEYFILE = "keyfile";

  public final static String DEFAULT_SERVER_HOST = "localhost:0";
  public final static String DEFAULT_AUTHORIZATION_SCHEME = AUTHORIZATION_SCHEME_NONE;

  private String serverHost = DEFAULT_SERVER_HOST;
  private String authorizationScheme = DEFAULT_AUTHORIZATION_SCHEME;

  public final String getServerHost () {
    return serverHost;
  }

  public final ConnectionSettings setServerHost (String host) {
    serverHost = host;
    return this;
  }

  public final String getAuthorizationScheme () {
    return authorizationScheme;
  }

  public final ConnectionSettings setAuthorizationScheme (String scheme) {
    authorizationScheme = scheme;
    return this;
  }

  @Override
  public String toString () {
    return String.format(
      "Server:%s Scheme:%s", serverHost, authorizationScheme
    );
  }
}

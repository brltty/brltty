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
  public final static char HOST_PORT_SEPARATOR = ':';
  public final static String DEFAULT_HOST_NAME = "localhost";

  public final static int MINIMUM_PORT_NUMBER =  0;
  public final static int MAXIMUM_PORT_NUMBER = 99;
  public final static int DEFAULT_PORT_NUMBER = MINIMUM_PORT_NUMBER;

  public final static String DEFAULT_SERVER_HOST =
    DEFAULT_HOST_NAME + HOST_PORT_SEPARATOR + DEFAULT_PORT_NUMBER;

  private String serverHost = null;

  public static void checkHostSpecification (String specification) {
    int index = specification.indexOf(HOST_PORT_SEPARATOR);

    if (index == 0) {
      throw new IllegalArgumentException("missing host name/address");
    }

    if (index < 0) {
      specification = String.format(
        "%s%c%s", specification, HOST_PORT_SEPARATOR, DEFAULT_PORT_NUMBER
      );
    } else {
      String number = specification.substring(index+1);

      if (number.isEmpty()) {
        throw new IllegalArgumentException("missing port number");
      }

      try {
        Parse.asInt(
          "port number", number,
          MINIMUM_PORT_NUMBER, MAXIMUM_PORT_NUMBER
        );
      } catch (SyntaxException exception) {
        throw new IllegalArgumentException(exception.getMessage());
      }
    }
  }

  public final ConnectionSettings setServerHost (String specification) {
    checkHostSpecification(specification);
    serverHost = specification;
    return this;
  }

  public final String getServerHost () {
    return serverHost;
  }

  public final static char AUTHORIZATION_SCHEME_SEPARATOR = '+';
  public final static char AUTHORIZATION_OPERAND_PREFIX = ':';

  public final static String AUTHORIZATION_SCHEME_NONE = "none";
  public final static String AUTHORIZATION_SCHEME_KEYFILE = "keyfile";

  public final static String DEFAULT_AUTHORIZATION_SCHEME = AUTHORIZATION_SCHEME_NONE;
  private String authorizationScheme = null;

  public static void checkAuthorizationScheme (String scheme) {
  }

  public final ConnectionSettings setAuthorizationScheme (String scheme) {
    checkAuthorizationScheme(scheme);
    authorizationScheme = scheme;
    return this;
  }

  public final String getAuthorizationScheme () {
    return authorizationScheme;
  }

  public ConnectionSettings () {
    setServerHost(DEFAULT_SERVER_HOST);
    setAuthorizationScheme(DEFAULT_AUTHORIZATION_SCHEME);
  }

  @Override
  public String toString () {
    return String.format(
      "Server:%s Scheme:%s", serverHost, authorizationScheme
    );
  }
}

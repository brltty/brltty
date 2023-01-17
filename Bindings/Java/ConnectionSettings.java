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

import java.io.File;

public class ConnectionSettings extends NativeComponent {
  public final static char HOST_PORT_SEPARATOR = ':';
  public final static String DEFAULT_HOST_NAME = "";

  public final static int MINIMUM_PORT_NUMBER =  0;
  public final static int MAXIMUM_PORT_NUMBER = 99;
  public final static int DEFAULT_PORT_NUMBER = MINIMUM_PORT_NUMBER;

  public final static String DEFAULT_SERVER_HOST =
    DEFAULT_HOST_NAME + HOST_PORT_SEPARATOR + DEFAULT_PORT_NUMBER;

  private String serverHost = null;

  public static void checkHostSpecification (String specification) {
    int index = specification.indexOf(HOST_PORT_SEPARATOR);

    if (index == 0) {
      return;
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

  public final static char AUTHENTICATION_SCHEME_SEPARATOR = '+';
  public final static char AUTHENTICATION_OPERAND_PREFIX = ':';

  public final static String AUTHENTICATION_SCHEME_KEYFILE = "keyfile";
  public final static String AUTHENTICATION_SCHEME_NONE = "none";

  private native static String getKeyfileDirectory ();
  public final static String AUTHENTICATION_KEYFILE_DIRECTORY = getKeyfileDirectory();

  private native static String getKeyfileName ();
  public final static String AUTHENTICATION_KEYFILE_NAME = getKeyfileName();

  public final static String DEFAULT_AUTHENTICATION_SCHEME;
  static {
    StringBuilder builder = new StringBuilder();

    {
      File file = new File(
        AUTHENTICATION_KEYFILE_DIRECTORY,
        AUTHENTICATION_KEYFILE_NAME
      );

      if (file.isFile() && file.canRead()) {
        builder.append(AUTHENTICATION_SCHEME_KEYFILE)
               .append(AUTHENTICATION_OPERAND_PREFIX)
               .append(file.getAbsolutePath());
      }
    }

    if (builder.length() > 0) builder.append(AUTHENTICATION_SCHEME_SEPARATOR);
    builder.append(AUTHENTICATION_SCHEME_NONE);

    DEFAULT_AUTHENTICATION_SCHEME = builder.toString();
  }

  private String authenticationScheme = null;

  public static void checkAuthenticationScheme (String scheme) {
  }

  public final ConnectionSettings setAuthenticationScheme (String scheme) {
    checkAuthenticationScheme(scheme);
    authenticationScheme = scheme;
    return this;
  }

  public final String getAuthenticationScheme () {
    return authenticationScheme;
  }

  public ConnectionSettings () {
    setServerHost(DEFAULT_SERVER_HOST);
    setAuthenticationScheme(DEFAULT_AUTHENTICATION_SCHEME);
  }

  @Override
  public String toString () {
    return String.format(
      "Server:%s Scheme:%s", serverHost, authenticationScheme
    );
  }
}

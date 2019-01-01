/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2019 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
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

package org.a11y.brltty.android;

import java.util.Collection;

import java.util.Map;
import java.util.LinkedHashMap;

import java.util.List;
import java.util.ArrayList;

public abstract class DeviceCollection {
  protected DeviceCollection () {
  }

  protected static interface StringMaker<T> {
    public String makeString (T object);
  }

  protected static <T> String[] makeStringArray (Collection<T> collection, StringMaker<T> stringMaker) {
    List<String> strings = new ArrayList<String>(collection.size());

    for (T element : collection) {
      strings.add(stringMaker.makeString(element));
    }

    return strings.toArray(new String[strings.size()]);
  }

  public static final char IDENTIFIER_SEPARATOR = ',';
  public static final char QUALIFIER_DELIMITER = ':';
  public static final char PARAMETER_SEPARATOR = '+';
  public static final char ASSIGNMENT_DELIMITER = '=';

  public abstract String getQualifier ();
  public abstract String[] makeValues ();
  public abstract String[] makeLabels ();
  protected abstract void putParameters (Map<String, String> parameters, String value);

  public static String makeReference (Map<String, String> parameters) {
    StringBuilder reference = new StringBuilder();

    for (String key : parameters.keySet()) {
      String value = parameters.get(key);
      if (value == null) continue;
      if (value.isEmpty()) continue;

      if (value.indexOf(PARAMETER_SEPARATOR) >= 0) continue;
      if (value.indexOf(IDENTIFIER_SEPARATOR) >= 0) continue;

      if (reference.length() > 0) reference.append(PARAMETER_SEPARATOR);
      reference.append(key);
      reference.append(ASSIGNMENT_DELIMITER);
      reference.append(value);
    }

    if (reference.length() == 0) return null;
    return reference.toString();
  }

  public final String makeReference (String value) {
    Map<String, String> parameters = new LinkedHashMap<String, String>();
    putParameters(parameters, value);
    return makeReference(parameters);
  }

  public final String makeIdentifier (String value) {
    String reference = makeReference(value);
    if (reference == null) return null;
    return getQualifier() + QUALIFIER_DELIMITER + reference;
  }
}

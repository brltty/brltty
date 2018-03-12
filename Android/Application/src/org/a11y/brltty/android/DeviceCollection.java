/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
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

  public static final char IDENTIFIER_DELIMITER = ',';
  public static final char QUALIFIER_DELIMITER = ':';

  public abstract String getQualifier ();
  public abstract String[] getIdentifiers ();
  public abstract String[] getLabels ();
  protected abstract void fillParameters (Map<String, String> parameters, String identifier);

  public final String makeReference (String identifier) {
    Map<String, String> parameters = new LinkedHashMap<String, String>();
    fillParameters(parameters, identifier);
    StringBuilder reference = new StringBuilder();

    for (String key : parameters.keySet()) {
      String value = parameters.get(key);
      if (value == null) continue;
      if (value.isEmpty()) continue;

      if (reference.length() > 0) reference.append('+');
      reference.append(key);
      reference.append('=');
      reference.append(value);
    }

    if (reference.length() == 0) return null;
    return reference.toString();
  }
}

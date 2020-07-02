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

import java.util.Map;
import java.util.HashMap;

import java.util.regex.Pattern;

public abstract class Strings {
  private Strings () {
  }

  private final static Map<String, Pattern> patternCache = new HashMap<>();

  public static Pattern getPattern (String expression) {
    synchronized (patternCache) {
      Pattern pattern = patternCache.get(expression);
      if (pattern != null) return pattern;

      pattern = Pattern.compile(expression);
      patternCache.put(expression, pattern);
      return pattern;
    }
  }

  public static String replaceAll (String string, String expression, String replacement) {
    return getPattern(expression).matcher(string).replaceAll(replacement);
  }

  public static String wordify (String string) {
    string = replaceAll(string, "^\\s*(.*?)\\s*$", "$1");

    if (!string.isEmpty()) {
      string = replaceAll(string, "(?<=.)(?=\\p{Upper})", " ");
      string = replaceAll(string, "\\s+", " ");
    }

    return string;
  }
  
  public static int findNonemptyLine (CharSequence text) {
    int length = text.length();
    int at = 0;

    for (int index=0; index<length; index+=1) {
      char character = text.charAt(index);

      if (character == '\n') {
        at = index + 1;
      } else if (!Character.isWhitespace(character)) {
        break;
      }
    }

    return at;
  }

  public static int findTrailingWhitespace (CharSequence text) {
    int index = text.length();

    while (--index >= 0) {
      if (!Character.isWhitespace(text.charAt(index))) break;
    }

    return index + 1;
  }
}

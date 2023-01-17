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

import java.util.Map;
import java.util.HashMap;

import java.util.regex.Pattern;
import java.util.regex.Matcher;

public abstract class Strings {
  private Strings () {
  }

  public static boolean isAbbreviation (String actual, String supplied) {
    int count = supplied.length();
    if (count == 0) return false;
    if (count > actual.length()) return false;
    return actual.regionMatches(true, 0, supplied, 0, count);
  }

  private final static Map<String, Pattern> patternCache = new HashMap<>();

  public static Pattern getPattern (String expression) {
    synchronized (patternCache) {
      return patternCache.computeIfAbsent(expression, Pattern::compile);
    }
  }

  public static Matcher getMatcher (String expression, String text) {
    return getPattern(expression).matcher(text);
  }

  public static String replaceAll (String text, String expression, String replacement) {
    return getMatcher(expression, text).replaceAll(replacement);
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

  public static String removeTrailingWhitespace (String text) {
    return text.substring(0, findTrailingWhitespace(text));
  }

  public static String compressWhitespace (String text) {
    text = removeTrailingWhitespace(text);
    if (!text.isEmpty()) text = replaceAll(text, "(?<=\\S)\\s+", " ");
    return text;
  }

  public static String compressEmptyLines (String text) {
    return replaceAll(text, "\n{2,}", "\n\n");
  }

  public static String formatParagraphs (String text, int width) {
    StringBuilder result = new StringBuilder();

    int length = text.length();
    int from = 0;

    while (from < length) {
      int to = text.indexOf('\n', from);
      if (to < 0) to = length;
      String line = compressWhitespace(text.substring(from, to));

      if (!line.isEmpty()) {
        if (!Character.isWhitespace(line.charAt(0))) {
          if (result.length() > 0) result.append('\n');

          while (line.length() > width) {
            int end = line.lastIndexOf(' ', width);

            if (end < 0) {
              end = line.indexOf(' ', width);
              if (end < 0) break;
            }

            result.append(line.substring(0, end)).append('\n');
            line = line.substring(end+1);
          }
        }

        result.append(line);
      }

      result.append('\n');
      from = to + 1;
    }

    result.setLength(findTrailingWhitespace(result));
    result.delete(0, findNonemptyLine(result));

    return compressEmptyLines(result.toString());
  }

  public static String formatParagraphs (String text) {
    return formatParagraphs(text, 72);
  }
}

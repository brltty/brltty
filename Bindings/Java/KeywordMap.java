/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2021 by
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

import java.util.Set;
import java.util.Map;
import java.util.HashMap;
import java.util.LinkedHashMap;

public class KeywordMap<V> {
  private final Map<String, V> valueMap = new LinkedHashMap<>();
  private final Map<String, String> aliasMap = new HashMap<>();
  private final static String AMBIGUOUS = "";

  private static String normalizeKeyword (String keyword) {
    return keyword.toLowerCase();
  }

  private final void addAliases (String keyword) {
    String alias = keyword;

    while (true) {
      alias = alias.substring(0, alias.length()-1);
      if (alias.isEmpty()) break;

      String value = aliasMap.get(alias);
      if (value == null) {
        aliasMap.put(alias, keyword);
      } else if (value == AMBIGUOUS) {
        break;
      } else {
        aliasMap.put(alias, AMBIGUOUS);
      }
    }
  }

  public KeywordMap<V> put (String keyword, V value) {
    keyword = normalizeKeyword(keyword);
    valueMap.put(keyword, value);
    addAliases(keyword);
    return this;
  }

  public final V get (String keyword) {
    keyword = normalizeKeyword(keyword);
    V value = valueMap.get(keyword);
    if (value != null) return value;

    keyword = aliasMap.get(keyword);
    if (keyword == null) return null;
    if (keyword == AMBIGUOUS) return null;
    return valueMap.get(keyword);
  }

  public final String[] getKeywords () {
    Set<String> keywords = valueMap.keySet();
    return keywords.toArray(new String[keywords.size()]);
  }

  public final boolean isEmpty () {
    return valueMap.isEmpty();
  }

  public KeywordMap () {
  }
}

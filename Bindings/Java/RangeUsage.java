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

import java.util.List;
import java.util.LinkedList;

public class RangeUsage {
  private final String rangeDescription;
  private final int minimumValue;
  private final int maximumValue;

  public RangeUsage (String description, int minimum, int maximum) {
    rangeDescription = description;
    minimumValue = minimum;
    maximumValue = maximum;
  }

  private static class KeywordEntry {
    public final String name;
    public final String explanation;

    public KeywordEntry (String name, String explanation) {
      this.name = name;
      this.explanation = explanation;
    }
  }

  private String defaultValue = null;
  private String rangeClarification = null;
  private final List<KeywordEntry> keywordList = new LinkedList<>();

  public final RangeUsage setDefault (String text) {
    defaultValue = text;
    return this;
  }

  public final RangeUsage setDefault (int value) {
    return setDefault(Integer.toString(value));
  }

  public final RangeUsage setClarification (String text) {
    rangeClarification = text;
    return this;
  }

  public final RangeUsage addKeyword (String name, String explanation) {
    keywordList.add(new KeywordEntry(name, explanation));
    return this;
  }

  public final RangeUsage addKeyword (String name) {
    return addKeyword(name, null);
  }

  public final RangeUsage addKeywords (String... names) {
    for (String name : names) {
      addKeyword(name);
    }

    return this;
  }

  public final void append (StringBuilder usage) {
    usage.append('\n')
         .append("The ").append(rangeDescription)
         .append(" must be an integer")
         ;

    if (rangeClarification != null) {
      usage.append(' ').append(rangeClarification);
    }

    usage.append(" within the range ").append(minimumValue)
         .append(" through ").append(maximumValue)
         ;

    if (!keywordList.isEmpty()) {
      usage.append(", or");

      int count = keywordList.size();
      if (count > 1) usage.append(" one of");
      int number = 0;

      for (KeywordEntry keyword : keywordList) {
        number += 1;

        if (number > 1) {
          if (count > 2) usage.append(',');
          if (number == count) usage.append(" or");
        }

        usage.append(' ').append(keyword.name);

        {
          String explanation = keyword.explanation;

          if (explanation != null) {
            usage.append(" (").append(explanation).append(')');
          }
        }
      }
    }

    usage.append(". ");

    if (defaultValue != null) {
      usage.append("If not specified, ")
           .append(defaultValue)
           .append(" is assumed. ");
    }
  }
}

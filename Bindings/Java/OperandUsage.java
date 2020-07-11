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

public class OperandUsage {
  private final String operandDescription;

  public OperandUsage (String description) {
    operandDescription = description;
  }

  private boolean haveRange = false;
  private Integer rangeMinimum = null;
  private Integer rangeMaximum = null;
  private String rangeUnits = null;
  private String rangeComment = null;

  public final OperandUsage setRange (int minimum, int maximum) {
    rangeMinimum = minimum;
    rangeMaximum = maximum;
    haveRange = true;
    return this;
  }

  public final OperandUsage setRangeUnits (String units) {
    rangeUnits = units;
    return this;
  }

  public final OperandUsage setRangeComment (String text) {
    rangeComment = text;
    return this;
  }

  private static class WordEntry {
    public final String word;
    public final Integer value;
    public final String comment;

    public WordEntry (String word, Integer value, String comment) {
      this.word = word;
      this.value = value;
      this.comment = comment;
    }
  }

  private final List<WordEntry> wordList = new LinkedList<>();

  public final OperandUsage addWord (String word, Integer value, String comment) {
    wordList.add(new WordEntry(word, value, comment));
    return this;
  }

  public final OperandUsage addWord (String word, String comment) {
    return addWord(word, null, comment);
  }

  public final OperandUsage addWord (String word, int value) {
    return addWord(word, value, null);
  }

  public final OperandUsage addWord (String word) {
    return addWord(word, null, null);
  }

  private String defaultWord = null;
  private Integer defaultValue = null;

  public final OperandUsage setDefault (String word) {
    defaultWord = word;
    defaultValue = null;
    return this;
  }

  public final OperandUsage setDefault (int value) {
    defaultWord = null;
    defaultValue = value;
    return this;
  }

  public final String getDefaultWord () {
    if (defaultWord != null) return defaultWord;
    if (defaultValue == null) return null;

    for (WordEntry entry : wordList) {
      if (entry.value == defaultValue) return entry.word;
    }

    return defaultValue.toString();
  }

  public final Integer getDefaultValue () {
    if (defaultValue != null) return defaultValue;
    if (defaultWord == null) return null;

    for (WordEntry entry : wordList) {
      if (entry.word.equals(defaultWord)) return entry.value;
    }

    return null;
  }

  public final StringBuilder appendTo (StringBuilder usage) {
    usage.append('\n')
         .append("The ").append(operandDescription).append(" must be");

    if (haveRange) {
      usage.append(" an integer");

      if (rangeUnits != null) {
        usage.append(" number of ").append(rangeUnits);
      }

      usage.append(" within the range ").append(rangeMinimum)
           .append(" through ").append(rangeMaximum);

      if (rangeComment != null) {
        usage.append(" (").append(rangeComment).append(')');
      }
    }

    if (!wordList.isEmpty()) {
      if (haveRange) usage.append(", or");

      int count = wordList.size();
      if (count > 1) usage.append(" any of");
      int number = 0;

      for (WordEntry entry : wordList) {
        number += 1;

        if (number > 1) {
          if (count > 2) usage.append(',');
          if (number == count) usage.append(" or");
        }

        usage.append(' ').append(entry.word);
        String comment = entry.comment;

        if (comment != null) {
          usage.append(" (").append(comment).append(')');
        }
      }
    }

    usage.append(". ");

    {
      String word = getDefaultWord();

      if (word != null) {
        usage.append("If not specified, ").append(word).append(" is assumed. ");
      }
    }

    return usage;
  }

  @Override
  public String toString () {
    return appendTo(new StringBuilder()).toString();
  }
}

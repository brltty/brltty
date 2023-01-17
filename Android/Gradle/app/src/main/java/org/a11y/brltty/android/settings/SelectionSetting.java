/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
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

package org.a11y.brltty.android.settings;
import org.a11y.brltty.android.*;

import android.preference.Preference;

import java.util.Arrays;
import java.util.Map;
import java.util.LinkedHashMap;

import java.text.Collator;
import java.text.CollationKey;

public abstract class SelectionSetting<P extends Preference> extends PreferenceSetting<P> {
  protected SelectionSetting (SettingsFragment fragment, int key) {
    super(fragment, key);
  }

  public abstract CharSequence[] getAllValues ();
  public abstract CharSequence getValueAt (int index);
  public abstract int indexOf (String value);

  public abstract CharSequence[] getAllLabels ();
  public abstract CharSequence getLabelAt (int index);

  public final CharSequence getLabelFor (String value) {
    int index = indexOf(value);
    if (index < 0) return null;
    return getLabelAt(index);
  }

  public final int getElementCount () {
    return getAllValues().length;
  }

  protected abstract void setElements (String[] values, String[] labels);

  protected final void setElements (String[] values) {
    setElements(values, values);
  }

  protected final void sortElementsByLabel (int fromIndex) {
    Collator collator = Collator.getInstance();
    collator.setStrength(Collator.PRIMARY);
    collator.setDecomposition(Collator.CANONICAL_DECOMPOSITION);

    String[] values = LanguageUtilities.newStringArray(getAllValues());
    String[] labels = LanguageUtilities.newStringArray(getAllLabels());

    int size = values.length;
    Map<String, String> map = new LinkedHashMap<String, String>();
    CollationKey keys[] = new CollationKey[size];

    for (int i=0; i<size; i+=1) {
      String label = labels[i];
      map.put(label, values[i]);
      keys[i] = collator.getCollationKey(label);
    }

    Arrays.sort(keys, fromIndex, keys.length);

    for (int i=0; i<size; i+=1) {
      String label = keys[i].getSourceString();
      labels[i] = label;
      values[i] = map.get(label);
    }

    setElements(values, labels);
  }

  protected final void sortElementsByLabel () {
    sortElementsByLabel(0);
  }
}

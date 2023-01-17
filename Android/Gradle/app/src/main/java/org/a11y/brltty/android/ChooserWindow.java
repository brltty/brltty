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

package org.a11y.brltty.android;

import android.view.View;
import android.widget.TextView;
import android.widget.Button;

import android.widget.ListView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;

public class ChooserWindow extends OverlayWindow {
  private final ListView itemList;
  private final Button dismissButton;

  private final void dismiss () {
    itemList.setOnItemClickListener(null);
    dismissButton.setOnClickListener(null);
    removeView();
  }

  public static interface ItemClickListener {
    public void onClick (int position);
  }

  public ChooserWindow (CharSequence[] itemLabels, int title, final ItemClickListener itemClickListener) {
    super();

    View view = setView(R.layout.chooser);
    itemList = (ListView)view.findViewById(R.id.chooser_list);
    dismissButton = (Button)view.findViewById(R.id.chooser_dismiss);

    {
      TextView titleView = (TextView)view.findViewById(R.id.chooser_title);
      titleView.setText(title);
    }

    {
      ArrayAdapter<CharSequence> adapter = new ArrayAdapter<CharSequence>(
        getContext(), android.R.layout.simple_list_item_1, itemLabels
      );

      itemList.setAdapter(adapter);
    }

    itemList.setOnItemClickListener(
      new AdapterView.OnItemClickListener() {
        @Override
        public void onItemClick (AdapterView adapter, View view, int position, long id) {
          itemClickListener.onClick(position);
          dismiss();
        }
      }
    );
    dismissButton.setOnClickListener(
      new View.OnClickListener() {
        @Override
        public void onClick (View view) {
          dismiss();
        }
      }
    );

    itemList.requestFocus();
  }
}

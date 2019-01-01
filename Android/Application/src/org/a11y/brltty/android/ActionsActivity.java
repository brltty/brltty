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

import android.os.Bundle;
import android.view.View;

import android.content.Intent;
import android.net.Uri;

public class ActionsActivity extends InternalActivity {
  @Override
  protected void onCreate (Bundle savedState) {
    super.onCreate(savedState);
    setContentView(R.layout.actions_activity);
  }

  public void switchInputMethod (View view) {
    InputService.switchInputMethod();
  }

  public void launchSettingsActivity (View view) {
    SettingsActivity.launch();
  }

  public void viewUserGuide (View view) {
    launch(R.string.user_guide_url);
  }

  public void browseWebSite (View view) {
    launch(R.string.web_site_url);
  }

  public void browseCommunityMessages (View view) {
    launch(R.string.community_messages_url);
  }

  public void postCommunityMessage (View view) {
    Intent intent = new Intent(Intent.ACTION_SENDTO, Uri.parse("mailto:"));

    {
      StringBuilder recipient = new StringBuilder();
      recipient.append("BRLTTY Mailing List");
      recipient.append(' ');
      recipient.append('<');
      recipient.append(getResourceString(R.string.community_message_address));
      recipient.append('>');
      intent.putExtra(Intent.EXTRA_EMAIL, new String[] {recipient.toString()});
    }

    launch(intent);
  }

  public void manageCommunityMembership (View view) {
    launch(R.string.community_membership_url);
  }

  public void updateApplication (View view) {
    launch(R.string.application_package_url);
  }

  public void launchAboutActivity (View view) {
    AboutActivity.launch();
  }

  public static void launch () {
    ApplicationUtilities.launch(ActionsActivity.class);
  }
}

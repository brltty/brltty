/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
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

package org.a11y.brltty.android.activities;
import org.a11y.brltty.android.*;
import org.a11y.brltty.android.settings.SettingsActivity;

import android.os.Bundle;
import android.content.pm.PackageManager;

import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;

import android.content.Intent;
import android.net.Uri;
import java.io.File;

public class ActionsActivity extends InternalActivity {
  private final static int BLUETOOTH_PERMISSIONS_REQUEST_CODE = 0;

  @Override
  public void onRequestPermissionsResult (int code, String[] permissions, int[] results) {
    if (code == BLUETOOTH_PERMISSIONS_REQUEST_CODE) {
      for (int i=0; i<results.length; i+=1) {
        if (results[i] == PackageManager.PERMISSION_GRANTED) {
          BrailleNotification.create();
          break;
        }
      }

      return;
    }
  }

  public void switchInputMethod (View view) {
    InputService.switchInputMethod();
  }

  public void allowBluetoothConnections (View view) {
    requestPermissions(
      BluetoothConnection.requiredPermissions,
      BLUETOOTH_PERMISSIONS_REQUEST_CODE
    );
  }

  public void launchSettingsActivity (View view) {
    SettingsActivity.launch();
  }

  public void viewUserGuide (View view) {
    launch(R.string.user_guide_url);
  }

  public void browseWebSite (View view) {
    launch(R.string.appConfig_webSite);
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
      recipient.append(getResourceString(R.string.appConfig_emailAddress));
      recipient.append('>');
      intent.putExtra(Intent.EXTRA_EMAIL, new String[] {recipient.toString()});
    }

    launch(intent);
  }

  public void manageCommunityMembership (View view) {
    launch(R.string.community_membership_url);
  }

  private CheckBox developerBuild = null;
  private CheckBox allowDowngrade = null;

  public void updateApplication (View view) {
    File directory = new File(getCacheDir(), "public");
    directory.mkdir();
    File file = new File(directory, "latest.apk");

    int url =
      developerBuild.isChecked()?
      R.string.developer_package_url:
      R.string.application_package_url;

    new PackageInstaller(this, url, file) {
      @Override
      protected void onInstallFailed (String message) {
        showMessage(
          String.format(
            "%s: %s",
            getString(R.string.updateApplication_problem_failed),
            message
          )
        );
      }
    }.setAllowDowngrade(allowDowngrade.isChecked())
     .startInstall();
  }

  public void launchAboutActivity (View view) {
    AboutActivity.launch();
  }

  @Override
  protected void onCreate (Bundle savedState) {
    super.onCreate(savedState);
    setContentView(R.layout.actions_activity);

    developerBuild = findViewById(R.id.GLOBAL_CHECKBOX_DEVELOPER_BUILD);
    allowDowngrade = findViewById(R.id.GLOBAL_CHECKBOX_ALLOW_DOWNGRADE);

    { // update application options
      View button = findViewById(R.id.GLOBAL_BUTTON_UPDATE_APPLICATION);
      button.setVisibility(BuildConfig.DEBUG? View.VISIBLE: View.GONE);

      final View[] options = new View[] {
        developerBuild,
        allowDowngrade
      };

      for (View option : options) {
        option.setVisibility(View.GONE);
      }

      button.setOnLongClickListener(
        new View.OnLongClickListener() {
          @Override
          public boolean onLongClick (View view) {
            for (View option : options) {
              option.setVisibility(View.VISIBLE);
            }

            return true;
          }
        }
      );
    }
  }

  @Override
  protected void onResume () {
    super.onResume();

    {
      Button allowBluetoothConnections = findViewById(R.id.GLOBAL_BUTTON_ALLOW_BLUETOOTH_CONNECTIONS);
      int visibility = BrailleApplication.havePermissions(BluetoothConnection.requiredPermissions)? View.GONE: View.VISIBLE;
      allowBluetoothConnections.setVisibility(visibility);
    }
  }

  public static void launch () {
    ApplicationUtilities.launch(ActionsActivity.class);
  }
}

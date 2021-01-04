/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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

import android.util.Log;

import android.content.Intent;
import android.content.ActivityNotFoundException;

import android.net.Uri;
import java.io.File;
import android.support.v4.content.FileProvider;

import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;

public class PackageInstaller extends UpgradeComponent {
  private final static String LOG_TAG = PackageInstaller.class.getName();

  public final static String PROVIDER_AUTHORITY = "org.a11y.brltty.android.fileprovider";
  public final static String MIME_TYPE = "application/vnd.android.package-archive";

  private final String sourceURL;
  private final File targetFile;
  private boolean allowDowngrade = false;

  public PackageInstaller (InternalActivity owner, String url, File file) {
    super(owner);
    sourceURL = url;
    targetFile = file;
  }

  public PackageInstaller (InternalActivity owner, int url, File file) {
    this(owner, owner.getResources().getString(url), file);
  }

  public final PackageInstaller setAllowDowngrade (boolean yes) {
    allowDowngrade = yes;
    return this;
  }

  private final PackageManager getPackageManager () {
    return getActivity().getPackageManager();
  }

  private final String getPackageName () {
    return getActivity().getPackageName();
  }

  private final PackageInfo getPackageInfo (String name) {
    try {
      return getPackageManager().getPackageInfo(name, 0);
    } catch (PackageManager.NameNotFoundException exception) {
      Log.w(LOG_TAG, ("package not installed: " + name));
    }

    return null;
  }

  private final PackageInfo getPackageInfo () {
    return getPackageInfo(getPackageName());
  }

  private final PackageInfo getPackageInfo (File file) {
    return getPackageManager().getPackageArchiveInfo(file.getAbsolutePath(), 0);
  }

  protected void onInstallFailed (String message) {
    getActivity().showMessage(
      String.format(
        "%s: %s",
        getString(R.string.packageInstaller_problem_failed),
        message
      )
    );
  }

  private void onInstallFailed (Exception exception) {
    String message = exception.getMessage();
    Log.w(LOG_TAG, String.format("package install failed: %s: %s", sourceURL, message));
    onInstallFailed(message);
  }

  private final boolean isUpgrade (PackageInfo oldInfo, PackageInfo newInfo) {
    int oldCode = oldInfo.versionCode;
    int newCode = newInfo.versionCode;

    String oldName = oldInfo.versionName;
    String newName = newInfo.versionName;

    if (newCode == oldCode) {
      onInstallFailed(
        String.format(
          "%s: %s (%d)",
          getString(R.string.packageInstaller_problem_same),
          newName, newCode
        )
      );

      return false;
    }

    if (newCode < oldCode) {
      final String operator = "<";

      onInstallFailed(
        String.format(
          "%s: %s %s %s (%d %s %d)",
          getString(R.string.packageInstaller_problem_downgrade),
          newName, operator, oldName,
          newCode, operator, oldCode
        )
      );

      return false;
    }

    return true;
  }

  private final boolean canInstallPackage () {
    if (allowDowngrade) return true;

    PackageInfo oldInfo = getPackageInfo();
    PackageInfo newInfo = getPackageInfo(targetFile);

    if (oldInfo == null) return true;
    return isUpgrade(oldInfo, newInfo);
  }

  public final void startInstall () {
    new FileDownloader(getActivity(), sourceURL, targetFile) {
      @Override
      protected void onDownloadFinished () {
        if (canInstallPackage()) {
          Uri uri;
          int flags;

          if (APITests.haveNougat) {
            uri = FileProvider.getUriForFile(getActivity(), PROVIDER_AUTHORITY, targetFile);
            flags = Intent.FLAG_GRANT_READ_URI_PERMISSION;
          } else {
            uri = Uri.fromFile(targetFile);
            flags = Intent.FLAG_ACTIVITY_NEW_TASK;
          }

          Intent intent = new Intent(Intent.ACTION_VIEW);
          intent.setDataAndType(uri, MIME_TYPE);
          intent.setFlags(flags);

          try {
            getActivity().startActivity(intent);
          } catch (ActivityNotFoundException exception) {
            onInstallFailed(exception);
          }
        }
      }

      @Override
      protected void onDownloadFailed (String message) {
        onInstallFailed(message);
      }
    }.startDownload();
  }
}

/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2020 by The BRLTTY Developers.
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

import android.util.Log;

import android.content.Intent;
import android.content.ActivityNotFoundException;

import android.net.Uri;
import java.io.File;
import android.support.v4.content.FileProvider;

import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;

public class PackageInstaller extends InternalActivityComponent {
  private final static String LOG_TAG = PackageInstaller.class.getName();

  protected final String sourceURL;
  protected final File targetFile;

  public PackageInstaller (InternalActivity owner, String url, File file) {
    super(owner);
    sourceURL = url;
    targetFile = file;
  }

  public PackageInstaller (InternalActivity owner, int url, File file) {
    this(owner, owner.getResources().getString(url), file);
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

  private final void reportProblem (String... components) {
    StringBuilder problem = new StringBuilder();

    for (String component : components) {
      if (problem.length() > 0) problem.append(": ");
      problem.append(component);
    }

    getActivity().showMessage(problem);
  }

  private final void reportNotUpdated (String reason) {
    reportProblem(getString(R.string.packageInstaller_problem_reject), reason);
  }

  private final boolean checkVersion (PackageInfo oldInfo, PackageInfo newInfo) {
    int oldCode = oldInfo.versionCode;
    int newCode = newInfo.versionCode;

    String oldName = oldInfo.versionName;
    String newName = newInfo.versionName;

    if (newCode == oldCode) {
      reportNotUpdated(
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

      reportNotUpdated(
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

  private final boolean checkArchive () {
    PackageInfo oldInfo = getPackageInfo();
    PackageInfo newInfo = getPackageInfo(targetFile);
    return checkVersion(oldInfo, newInfo);
  }

  protected void onInstallFailed (String message) {
  }

  private void onInstallFailed (Exception exception) {
    String message = exception.getMessage();
    Log.w(LOG_TAG, String.format("package install failed: %s: %s", sourceURL, message));
    onInstallFailed(message);
  }

  public final void startInstall () {
    new FileDownloader(getActivity(), sourceURL, targetFile) {
      @Override
      protected void onDownloadProgress (long time, long position, Long length) {
        if (true) {
          long remaining = (length == null)? -1: (length - position);
          Log.d("dnld-prog", String.format("t=%d p=%d r=%d", time, position, remaining));
        }
      }

      @Override
      protected void onDownloadFinished () {
        if (checkArchive()) {
          Intent intent = new Intent(Intent.ACTION_VIEW);

          if (ApplicationUtilities.haveNougat) {
            String authority = getClass().getPackage().getName() + ".fileprovider";
            Uri uri = FileProvider.getUriForFile(getActivity(), authority, targetFile);

            intent.setData(uri);
            intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
          } else {
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.setDataAndType(Uri.fromFile(targetFile), "application/vnd.android.package-archive");
          }

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

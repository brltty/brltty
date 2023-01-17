#!/bin/sh
###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2023 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any
# later version. Please see the file LICENSE-LGPL for details.
#
# Web Page: http://brltty.app/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

listInstalledPackages() {
   apk list --installed | awk '
      {
         gsub(/-[0-9].*/, "", $1)
         print $1
      }
   ' | sort -u
}

installPackages() {
   apk add --quiet --no-progress -- "${@}"
}

removePackages() {
   apk del --quiet --no-progress -- "${@}"
}

describePackage() {
   apk info -- "${1}"
}

whichPackage() {
   unsupportedPackageAction which
}

searchPackage() {
   apk search --description -- "${1}"
}


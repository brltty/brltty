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
   zypper --quiet packages --installed-only | awk --field-separator "|" '
      NR > 2 {
         sub(/^\s*/, "", $3)
         sub(/\s*$/, "", $3)
         print $3
      }
   ' | sort -u
}

installPackages() {
   zypper --quiet install --no-confirm -- "${@}"
}

removePackages() {
   zypper --quiet remove --no-confirm -- "${@}"
}

describePackage() {
   zypper --quiet info -- "${1}"
}

whichPackage() {
   zypper --quiet what-provides -- "${1}"
}

searchPackage() {
   zypper --quiet search --sort-by-name --search-descriptions -- "${1}"
}


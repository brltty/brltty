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
   brew list -1 --full-name
}

installPackages() {
   brew install "${@}"
}

removePackages() {
   brew uninstall "${@}"
}

describePackage() {
   brew desc --eval-all "${1}"
}

whichPackage() {
   brew which-formula --explain "${1}"
}

searchPackage() {
   brew search "${1}"
}

export HOMEBREW_NO_ENV_HINTS=1
export HOMEBREW_NO_INSTALL_CLEANUP=1

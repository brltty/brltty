#!/bin/bash
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

setPackageManager() {
   local -r commands=(
      /usr/local/bin/brew

      /sbin/apk
      /usr/bin/apt
      /usr/bin/dnf
      /usr/sbin/pacman
      /usr/sbin/pkg
      /usr/sbin/pkg_info
      /sbin/xbps-install
      /usr/bin/zypper
   )

   local command
   for command in "${commands[@]}"
   do
      [ -x "${command}" ] && {
         packageManager="${command##*/}"
         packageManager="${packageManager%%-*}"
         return 0
      }
   done

   semanticError "unknown package manager"
}

normalizePackageList() {
   sort | uniq
}

unsupportedPackageAction() {
   local action="${1}"
   semanticError "unsupported package action: ${action}"
}

setPackageManager
logNote "package manager: ${packageManager}"
. "${programDirectory}/packages.d/${packageManager}.sh"

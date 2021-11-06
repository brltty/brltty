#!/bin/bash
###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2021 by The BRLTTY Developers.
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

. "$(dirname "${BASH_SOURCE[0]}")/../prologue.sh"
setSourceRoot

readonly mkLogFileExtension="log"
readonly mkLogsDirectory="${sourceRoot}/Build/Logs"
mkDiffOptions=()

mkIgnoreMismatch() {
   local pattern="${1}"
   mkDiffOptions+=( -I "${pattern}" )
}

mkBuild() {
   [ -n "${mkPlatformName}" ] || internalError "platform name not defined"
   [ -n "${mkOldLogName}" ] || internalError "old log name not defined"
   [ -n "${mkNewLogName}" ] || internalError "new log name not defined"

   logMessage task "Building for ${mkPlatformName}"
   local newLogFile="${mkNewLogName}.${mkLogFileExtension}"
   local oldLogFile="${mkLogsDirectory}/${mkOldLogName}.${mkLogFileExtension}"

   "${@}" &>"${newLogFile}" || {
      local exitStatus="${?}"
      logMessage error "build failed with exit status ${exitStatus} - see ${newLogFile}"
      exit 10
   }

   logMessage task "build completed successfully"
   sed -e "s%${sourceRoot}%.../brltty%g" -i "${newLogFile}"
   diff "${mkDiffOptions[@]}" -- "${oldLogFile}" "${newLogFile}"
}

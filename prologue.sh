###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2010 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any
# later version. Please see the file LICENSE-GPL for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

initialDirectory="`pwd`"
programName="`basename "${0}"`"

programMessage() {
   echo >&2 "${programName}: ${1}"
}
programError() {
   [ -n "${2}" ] && programMessage "${2}"
   exit "${1:-3}"
}
syntaxError() {
   programError 2 "${1}"
}

needTemporaryDirectory() {
   cleanup() {
      set +e
      cd /
      [ -n "${temporaryDirectory}" ] && rm -f -r -- "${temporaryDirectory}"
   }
   trap "cleanup" 0

   umask 022
   [ -n "${TMPDIR}" -a -d "${TMPDIR}" -a -r "${TMPDIR}" -a -w "${TMPDIR}" -a -x "${TMPDIR}" ] || {
      TMPDIR="/tmp"
      export TMPDIR
   }
   temporaryDirectory="`mktemp -d "${TMPDIR}/${programName}.XXXXXX"`" && cd "${temporaryDirectory}" || exit "${?}"
}

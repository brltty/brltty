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

initialDirectory=`pwd`
programName=`basename "${0}"`

programMessage() {
   echo >&2 "${programName}: ${1}"
}

syntaxError() {
   programMessage "${1}"
   exit 2
}

semanticError() {
   programMessage "${1}"
   exit 3
}

verifyProgram() {
   [ -e "${1}" ] || semanticError "program not found: ${1}"
   [ -f "${1}" ] || semanticError "not a file: ${1}"
   [ -x "${1}" ] || semanticError "not executable: ${1}"
}

verifyDirectory() {
   [ -e "${1}" ] || return 1
   [ -d "${1}" ] || semanticError "not a directory: ${1}"
   return 0
}

verifyInputDirectory() {
   verifyDirectory "${1}" || semanticError "directory not found: ${1}"
}

verifyOutputDirectory() {
   if verifyDirectory "${1}"
   then
      [ -w "${1}" ] || semanticError "directory not writable: ${1}"
      rm -f -r -- "${1}/"*
   else
      mkdir -p "${1}"
   fi
}

needTemporaryDirectory() {
   cleanup() {
      set +e
      cd /
      [ -z "${temporaryDirectory}" ] || rm -f -r -- "${temporaryDirectory}"
   }
   trap "cleanup" 0

   umask 022
   [ -n "${TMPDIR}" -a -d "${TMPDIR}" -a -r "${TMPDIR}" -a -w "${TMPDIR}" -a -x "${TMPDIR}" ] || {
      TMPDIR="/tmp"
      export TMPDIR
   }
   temporaryDirectory=`mktemp -d "${TMPDIR}/${programName}.XXXXXX"` && cd "${temporaryDirectory}" || exit "${?}"
}

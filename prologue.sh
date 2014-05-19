###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2014 by The BRLTTY Developers.
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
programDirectory=`dirname "${0}"`
programDirectory=`realpath "${programDirectory}"`

programMessage() {
   echo >&2 "${programName}: ${1}"
}

syntaxError() {
   [ "${#}" -eq 0 ] || programMessage "${1}"
   exit 2
}

semanticError() {
   [ "${#}" -eq 0 ] || programMessage "${1}"
   exit 3
}

verifyProgram() {
   [ -e "${1}" ] || semanticError "program not found: ${1}"
   [ -f "${1}" ] || semanticError "not a file: ${1}"
   [ -x "${1}" ] || semanticError "not executable: ${1}"
}

testDirectory() {
   [ -e "${1}" ] || return 1
   [ -d "${1}" ] || semanticError "not a directory: ${1}"
   return 0
}

verifyInputDirectory() {
   testDirectory "${1}" || semanticError "directory not found: ${1}"
}

verifyOutputDirectory() {
   if testDirectory "${1}"
   then
      [ -w "${1}" ] || semanticError "directory not writable: ${1}"
      rm -f -r -- "${1}/"*
   else
      mkdir -p "${1}"
   fi
}

resolveDirectory() {
   (cd "${1}" && pwd)
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

programOptionsString=""
programOptionDefault_string=""
programOptionDefault_flag=false
programOptionDefault_counter=0

addProgramOption() {
   programOptionsString="${programOptionsString}${1}"
   eval 'programOptionType_'"${1}"'="'"${2}"'"'
   eval 'programOptionVariable_'"${1}"'="'"${3}"'"'
   eval 'programOptionUsage_'"${1}"'="'"${4}"'"'
   eval "${3}"'="${programOptionDefault_'"${2}"'}"'
   eval [ -n '"${'"${3}"'}"' ] || programOptionsString="${programOptionsString}:"
}

parseProgramOptions() {
   while getopts ":${programOptionsString}" programOptionLetter
   do
      case "${programOptionLetter}"
      in
        \?) syntaxError "unrecognized option: -${OPTARG}";;
         :) syntaxError "missing operand: -${OPTARG}";;

         *) eval 'programOptionVariable="${programOptionVariable_'"${programOptionLetter}"'}"'
            eval 'programOptionType="${programOptionType_'"${programOptionLetter}"'}"'

            case "${programOptionType}"
            in
               counter) eval "${programOptionVariable}"'=`expr "${'"${programOptionVariable}"'}" + 1`';;
               flag) eval "${programOptionVariable}"'=true';;
               string) eval "${programOptionVariable}"'="${OPTARG}"';;
               *) semanticError "unimplemented program option type: ${programOptionType} (-${programOptionLetter})";;
            esac
            ;;
      esac
   done
}

parseProgramOptions='
   parseProgramOptions "${@}"
   shift `expr "${OPTIND}" - 1`
'

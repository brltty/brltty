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

getVariable() {
   typeset variable="${1}"

   echo "${!variable}"
}

setVariable() {
   typeset variable="${1}"
   typeset value="${2}"

   eval "${variable}"'="${value}"'
}

quoteString() {
   typeset string="${1}"

   typeset pattern="'"
   typeset replacement="'"'"'"'"'"'"'"
   string="${string//${pattern}/${replacement}}"
   echo "'${string}'"
}

initialDirectory=`pwd`
programName=`basename "${0}"`
programDirectory=`dirname "${0}"`
programDirectory=`realpath "${programDirectory}"`

programMessage() {
   typeset message="${1}"

   echo >&2 "${programName}: ${message}"
}

syntaxError() {
   typeset message="${1}"

   [ "${#}" -eq 0 ] || programMessage "${message}"
   exit 2
}

semanticError() {
   typeset message="${1}"

   [ "${#}" -eq 0 ] || programMessage "${message}"
   exit 3
}

verifyProgram() {
   typeset path="${1}"

   [ -e "${path}" ] || semanticError "program not found: ${path}"
   [ -f "${path}" ] || semanticError "not a file: ${path}"
   [ -x "${path}" ] || semanticError "not executable: ${path}"
}

testDirectory() {
   typeset path="${1}"

   [ -e "${path}" ] || return 1
   [ -d "${path}" ] || semanticError "not a directory: ${path}"
   return 0
}

verifyInputDirectory() {
   typeset path="${1}"

   testDirectory "${path}" || semanticError "directory not found: ${path}"
}

verifyOutputDirectory() {
   typeset path="${1}"

   if testDirectory "${path}"
   then
      [ -w "${path}" ] || semanticError "directory not writable: ${path}"
      rm -f -r -- "${path}/"*
   else
      mkdir -p "${path}"
   fi
}

resolveDirectory() {
   typeset path="${1}"

   (cd "${path}" && pwd)
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
programOptionDefault_counter=0
programOptionDefault_flag=false
programOptionDefault_list=""
programOptionDefault_string=""

addProgramOption() {
   typeset letter="${1}"
   typeset type="${2}"
   typeset variable="${3}"
   typeset usage="${4}"

   programOptionsString="${programOptionsString}${letter}"
   eval 'programOptionType_'"${letter}"'="'"${type}"'"'
   eval 'programOptionVariable_'"${letter}"'="'"${variable}"'"'
   eval 'programOptionUsage_'"${letter}"'="'"${usage}"'"'
   eval "${variable}"'="${programOptionDefault_'"${type}"'}"'
   eval [ -n '"${'"${variable}"'}"' ] || programOptionsString="${programOptionsString}:"
}

parseProgramOptions() {
   typeset letter

   while getopts ":${programOptionsString}" letter
   do
      case "${letter}"
      in
        \?) syntaxError "unrecognized option: -${OPTARG}";;
         :) syntaxError "missing operand: -${OPTARG}";;

         *) eval 'typeset variable="${programOptionVariable_'"${letter}"'}"'
            eval 'typeset type="${programOptionType_'"${letter}"'}"'

            case "${type}"
            in
               counter) let "${variable} += 1";;
               flag) setVariable "${variable}" true;;
               lst) eval "${variable}"'="${'"${variable}"'} '"'"'${OPTARG}'"'"'"';;
               list) setVariable "${variable}" "$(getVariable "${variable}") $(quoteString "${OPTARG}")";;
               string) setVariable "${variable}" "${OPTARG}";;
               *) semanticError "unimplemented program option type: ${type} (-${letter})";;
            esac
            ;;
      esac
   done
}

parseProgramOptions='
   parseProgramOptions "${@}"
   shift $((OPTIND - 1))
'

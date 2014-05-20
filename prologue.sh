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

function programMessage {
   typeset message="${1}"

   [ -z "${message}" ] || echo >&2 "${programName}: ${message}"
}

function syntaxError {
   typeset message="${1}"

   [ "${#}" -eq 0 ] || programMessage "${message}"
   exit 2
}

function semanticError {
   typeset message="${1}"

   programMessage "${message}"
   exit 3
}

function internalError {
   typeset message="${1}"

   programMessage "${message}"
   exit 4
}

if [ "${BASH+set}" = "set" ]
then
   function getVariable {
      typeset variable="${1}"

      echo "${!variable}"
   }
elif [ "${KSH_VERSION+set}" = "set" ]
then
   function getVariable {
      typeset -n variable="${1}"

      echo "${variable}"
   }
fi

function setVariable {
   typeset variable="${1}"
   typeset value="${2}"

   eval "${variable}"'="${value}"'
}

function quoteString {
   typeset string="${1}"

   typeset pattern="'"
   typeset replacement="'"'"'"'"'"'"'"
   string="${string//${pattern}/${replacement}}"
   echo "'${string}'"
}

function verifyProgram {
   typeset path="${1}"

   [ -e "${path}" ] || semanticError "program not found: ${path}"
   [ -f "${path}" ] || semanticError "not a file: ${path}"
   [ -x "${path}" ] || semanticError "not executable: ${path}"
}

function testDirectory {
   typeset path="${1}"

   [ -e "${path}" ] || return 1
   [ -d "${path}" ] || semanticError "not a directory: ${path}"
   return 0
}

function verifyInputDirectory {
   typeset path="${1}"

   testDirectory "${path}" || semanticError "directory not found: ${path}"
}

function verifyOutputDirectory {
   typeset path="${1}"

   if testDirectory "${path}"
   then
      [ -w "${path}" ] || semanticError "directory not writable: ${path}"
      rm -f -r -- "${path}/"*
   else
      mkdir -p "${path}"
   fi
}

function resolveDirectory {
   typeset path="${1}"

   (cd "${path}" && pwd)
}

function needTemporaryDirectory {
   function cleanup {
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

function addProgramOption {
   typeset letter="${1}"
   typeset type="${2}"
   typeset variable="${3}"
   typeset usage="${4}"

   setVariable "programOptionType_${letter}" "${type}"
   setVariable "programOptionVariable_${letter}" "${variable}"
   setVariable "programOptionUsage_${letter}" "${usage}"

   typeset default="$(getVariable "programOptionDefault_${type}")"
   setVariable "${variable}" "${default}"

   programOptionsString="${programOptionsString}${letter}"
   [ -n "${default}" ] || programOptionsString="${programOptionsString}:"
}

function parseProgramOptions {
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
               list) setVariable "${variable}" "$(getVariable "${variable}") $(quoteString "${OPTARG}")";;
               string) setVariable "${variable}" "${OPTARG}";;
               *) internalError "unimplemented program option type: ${type} (-${letter})";;
            esac
            ;;
      esac
   done
}

parseProgramOptions='
   parseProgramOptions "${@}"
   shift $((OPTIND - 1))
'

programDirectory=`dirname "${0}"`
programDirectory=`resolveDirectory "${programDirectory}"`

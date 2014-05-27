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
programOptionsOperandWidth=0

programOptionDefault_counter=0
programOptionDefault_flag=false
programOptionDefault_list=""
programOptionDefault_string=""

function addProgramOption {
   typeset letter="${1}"
   typeset type="${2}"
   typeset variable="${3}"
   typeset usage="${4}"

   [ "$(expr "${letter}" : '^[[:alnum:]]*$')" -eq 1 ] || internalError "invalid program option: ${letter}"
   [ -z "$(getVariable "programOptionType_${letter}")" ] || internalError "duplicate program option definition: -${letter}"

   typeset operand
   case "${type}"
   in
      flag | counter)
         operand=""
         ;;

      string.* | list.*)
         operand="${type#*.}"
         type="${type%%.*}"
         [ -n "${operand}" ] || internalError "missing program option operand type: -${letter}"
         ;;

      *) internalError "invalid program option type: ${type} (-${letter})";;
   esac

   setVariable "programOptionType_${letter}" "${type}"
   setVariable "programOptionVariable_${letter}" "${variable}"
   setVariable "programOptionOperand_${letter}" "${operand}"
   setVariable "programOptionUsage_${letter}" "${usage}"

   typeset default="$(getVariable "programOptionDefault_${type}")"
   setVariable "${variable}" "${default}"

   typeset length="${#operand}"
   [ "${length}" -le "${programOptionsOperandWidth}" ] || programOptionsOperandWidth="${length}"

   programOptionsString="${programOptionsString}${letter}"
   [ "${length}" -eq 0 ] || programOptionsString="${programOptionsString}:"
}

addProgramOption h flag showProgramUsageSummary "show usage summary (this output), and then exit"

function showProgramUsageSummary {
   typeset options="${programOptionsString//:/}"
   typeset count="${#options}"
   typeset index=0
   typeset indent=$((3 + programOptionsOperandWidth + 2))

   typeset line="usage: ${programName}"
   [ "${count}" -eq 0 ] || line="${line} [-option ...]"
   typeset lines=("${line}")

   while ((index < count))
   do
      typeset letter="${options:index:1}"
      typeset line="-${letter} $(getVariable "programOptionOperand_${letter}")"

      while [ "${#line}" -lt "${indent}" ]
      do
         line="${line} "
      done

      line="${line}$(getVariable "programOptionUsage_${letter}")"
      lines[${#lines[*]}]="${line}"
      ((index += 1))
   done

   for line in "${lines[@]}"
   do
      echo "${line}"
   done
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

   if "${showProgramUsageSummary}"
   then
      showProgramUsageSummary
      exit 0
   fi
'

programDirectory=`dirname "${0}"`
programDirectory=`resolveDirectory "${programDirectory}"`

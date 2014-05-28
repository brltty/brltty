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

function setElement {
   typeset array="${1}"
   typeset index="${2}"
   typeset value="${3}"

   setVariable "${array}[${index}]" "${value}"
}

function appendElement {
   typeset array="${1}"
   typeset value="${2}"

   setElement "${array}" "\${#${array}[*]}" "${value}"
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

programParameterCount=0
programParameterLabelWidth=0

function addProgramParameter {
   typeset label="${1}"
   typeset variable="${2}"
   typeset usage="${3}"

   setVariable "programParameterLabel_${programParameterCount}" "${label}"
   setVariable "programParameterVariable_${programParameterCount}" "${variable}"
   setVariable "programParameterUsage_${programParameterCount}" "${usage}"

   typeset length="${#label}"
   [ "${length}" -le "${programParameterLabelWidth}" ] || programParameterLabelWidth="${length}"

   setVariable "${variable}" ""
   ((programParameterCount += 1))
}

programOptionString=""
programOptionOperandWidth=0

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
   [ "${length}" -le "${programOptionOperandWidth}" ] || programOptionOperandWidth="${length}"

   programOptionString="${programOptionString}${letter}"
   [ "${length}" -eq 0 ] || programOptionString="${programOptionString}:"
}

function addProgramUsageText {
   typeset linesArray="${1}"
   typeset text="${2}"
   typeset width="${3}"
   typeset prefix="${4}"

   typeset indent="${#prefix}"
   typeset length=$((width - indent))

   ((length > 0)) || {
      appendElement "${linesArray}" "${prefix}"
      prefix="${prefix:-length+1}"
      prefix="${prefix//?/ }"
      length=1
   }

   while [ "${#text}" -gt "${length}" ]
   do
      typeset index=$((length + 1))

      while ((--index > 0))
      do
         [ "${text:index:1}" != " " ] || break
      done

      if ((index > 0))
      then
         typeset head="${text:0:index}"
      else
         typeset head="${text%% *}"
         [ "${head}" != "${text}" ] || break
         index="${#head}"
      fi

      appendElement "${linesArray}" "${prefix}${head}"
      text="${text:index+1}"
      prefix="${prefix//?/ }"
   done

   appendElement "${linesArray}" "${prefix}${text}"
}

function showProgramUsageSummary {
   programUsageLines=()
   typeset options="${programOptionString//:/}"
   typeset count="${#options}"
   typeset width="${COLUMNS:-72}"

   typeset line="usage: ${programName}"
   [ "${count}" -eq 0 ] || line="${line} [-option ...]"

   typeset index=0
   while ((index < programParameterCount))
   do
      line="${line} $(getVariable "programParameterLabel_${index}")"
      ((index += 1))
   done

   appendElement programUsageLines "${line}"

   [ "${programParameterCount}" -eq 0 ] || {
      appendElement programUsageLines "parameters:"

      typeset indent=$((programParameterLabelWidth + 2))
      typeset index=0

      while ((index < programParameterCount))
      do
         typeset line="$(getVariable "programParameterLabel_${index}")"

         while [ "${#line}" -lt "${indent}" ]
         do
            line="${line} "
         done

         addProgramUsageText programUsageLines "$(getVariable "programParameterUsage_${index}")" "${width}" "  ${line}"
         ((index += 1))
      done
   }

   [ "${count}" -eq 0 ] || {
      appendElement programUsageLines "options:"

      typeset indent=$((3 + programOptionOperandWidth + 2))
      typeset index=0

      while ((index < count))
      do
         typeset letter="${options:index:1}"
         typeset line="-${letter} $(getVariable "programOptionOperand_${letter}")"

         while [ "${#line}" -lt "${indent}" ]
         do
            line="${line} "
         done

         addProgramUsageText programUsageLines "$(getVariable "programOptionUsage_${letter}")" "${width}" "  ${line}"
         ((index += 1))
      done
   }

   for line in "${programUsageLines[@]}"
   do
      echo "${line}"
   done
}

addProgramOption h flag showProgramUsageSummary "show usage summary (this output), and then exit"

function parseProgramOptions {
   typeset letter

   while getopts ":${programOptionString}" letter
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

   programParameterIndex=0
   while ((programParameterIndex < programParameterCount))
   do
      setVariable "programParameterVariable_${programParameterIndex}" "${1}"
      shift 1
      ((programParameterIndex += 1))
   done
   unset programParameterIndex

   [ "${#}" -eq 0 ] || syntaxError "too many parameters"
'

programDirectory=`dirname "${0}"`
programDirectory=`resolveDirectory "${programDirectory}"`

###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2018 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any
# later version. Please see the file LICENSE-LGPL for details.
#
# Web Page: http://brltty.com/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

readonly initialDirectory="$(pwd)"
readonly programName="$(basename "${0}")"

getVariable() {
   local __variable__="${1}"

   eval 'echo "${'"${__variable__}"'}"'
}

setVariable() {
   local __variable__="${1}"
   local value="${2}"

   eval "${__variable__}"'="${value}"'
}

defineEnumeration() {
   local prefix="${1}"
   shift 1

   local name
   local value=1

   for name
   do
      local variable="${prefix}${name}"
      readonly "${variable}"="${value}"
      value=$((value + 1))
   done
}

stringHead() {
   local string="${1}"
   local length="${2}"

   expr substr "${string}" 1 "${length}"
}

stringTail() {
   local string="${1}"
   local start="${2}"

   expr substr "${string}" $((start + 1)) $((${#string} - start))
}

stringReplace() {
   local string="${1}"
   local pattern="${2}"
   local replacement="${3}"
   local flags="${4}"

   echo "${string}" | sed -e "s/${pattern}/${replacement}/${flags}"
}

stringReplaceAll() {
   local string="${1}"
   local pattern="${2}"
   local replacement="${3}"

   stringReplace "${string}" "${pattern}" "${replacement}" "g"
}

stringQuoted() {
   local string="${1}"

   local pattern="'"
   local replacement="'"'"'"'"'"'"'"
   string="$(stringReplaceAll "${string}" "${pattern}" "${replacement}")"
   echo "'${string}'"
}

stringWrapped() {
   local string="${1}"
   local width="${2}"

   local result=""

   while [ "${#string}" -gt "${width}" ]
   do
      local head="$(stringHead "${string}" $((width + 1)))"
      head="${head% *}"

      [ "${#head}" -le "${width}" ] || {
         head="${string%% *}"
         [ "${head}" != "${string}" ] || break
      }

      result="${result} $(stringQuoted "${head}")"
      string="$(stringTail "${string}" $((${#head} + 1)))"
   done

   result="${result} $(stringQuoted "${string}")"
   echo "${result}"
}

programMessage() {
   local message="${1}"

   [ -z "${message}" ] || echo >&2 "${programName}: ${message}"
}

defineEnumeration programLogLevel_ error warning task step detail
programLogLevel=$((${programLogLevel_task}))

logMessage() {
   local level="${1}"
   local message="${2}"

   local variable="programLogLevel_${level}"
   local value=$((${variable}))

   [ "${value}" -gt 0 ] || programMessage "unknown log level: ${level}"
   [ "${value}" -gt "${programLogLevel}" ] || programMessage "${message}"
}

syntaxError() {
   local message="${1}"

   logMessage error "${message}"
   exit 2
}

semanticError() {
   local message="${1}"

   logMessage error "${message}"
   exit 3
}

internalError() {
   local message="${1}"

   logMessage error "${message}"
   exit 4
}

verifyProgram() {
   local path="${1}"

   [ -e "${path}" ] || semanticError "program not found: ${path}"
   [ -f "${path}" ] || semanticError "not a file: ${path}"
   [ -x "${path}" ] || semanticError "not executable: ${path}"
}

testDirectory() {
   local path="${1}"

   [ -e "${path}" ] || return 1
   [ -d "${path}" ] || semanticError "not a directory: ${path}"
   return 0
}

verifyInputDirectory() {
   local path="${1}"

   testDirectory "${path}" || semanticError "directory not found: ${path}"
}

verifyOutputDirectory() {
   local path="${1}"

   if testDirectory "${path}"
   then
      [ -w "${path}" ] || semanticError "directory not writable: ${path}"
      rm -f -r -- "${path}/"*
   else
      mkdir -p "${path}"
   fi
}

resolveDirectory() {
   local path="${1}"

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
   [ -n "${TMPDIR}" -a -d "${TMPDIR}" -a -r "${TMPDIR}" -a -w "${TMPDIR}" -a -x "${TMPDIR}" ] || export TMPDIR="/tmp"
   temporaryDirectory="$(mktemp -d "${TMPDIR}/${programName}.XXXXXX")" && cd "${temporaryDirectory}" || exit "${?}"
}

programParameterCount=0
programParameterCountMinimum=-1
programParameterLabelWidth=0

addProgramParameter() {
   local label="${1}"
   local variable="${2}"
   local usage="${3}"
   local default="${4}"

   setVariable "programParameterLabel_${programParameterCount}" "${label}"
   setVariable "programParameterVariable_${programParameterCount}" "${variable}"
   setVariable "programParameterUsage_${programParameterCount}" "${usage}"
   setVariable "programParameterDefault_${programParameterCount}" "${default}"

   local length="${#label}"
   [ "${length}" -le "${programParameterLabelWidth}" ] || programParameterLabelWidth="${length}"

   setVariable "${variable}" ""
   programParameterCount=$((programParameterCount + 1))
}

optionalProgramParameters() {
   if [ "${programParameterCountMinimum}" -lt 0 ]
   then
      programParameterCountMinimum="${programParameterCount}"
   else
      logMessage warning "program parameters are already optional"
   fi
}

programOptionLetters=""
programOptionString=""
programOptionOperandWidth=0

programOptionValue_counter=0
programOptionValue_flag=false
programOptionValue_list=""
programOptionValue_string=""

addProgramOption() {
   local letter="${1}"
   local type="${2}"
   local variable="${3}"
   local usage="${4}"
   local default="${5}"

   [ "$(expr "${letter}" : '[[:alnum:]]*$')" -eq 1 ] || internalError "invalid program option: -${letter}"
   [ -z "$(getVariable "programOptionType_${letter}")" ] || internalError "duplicate program option definition: -${letter}"

   local operand
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
   setVariable "programOptionDefault_${letter}" "${default}"

   local value="$(getVariable "programOptionValue_${type}")"
   setVariable "${variable}" "${value}"

   local length="${#operand}"
   [ "${length}" -le "${programOptionOperandWidth}" ] || programOptionOperandWidth="${length}"

   programOptionLetters="${programOptionLetters} ${letter}"
   programOptionString="${programOptionString}${letter}"
   [ "${length}" -eq 0 ] || programOptionString="${programOptionString}:"
}

addProgramUsageLine() {
   local line="${1}"

   setVariable "programUsageLine_${programUsageLineCount}" "${line}"
   programUsageLineCount=$((programUsageLineCount + 1))
}

addProgramUsageText() {
   local text="${1}"
   local width="${2}"
   local prefix="${3}"

   width=$((width - ${#prefix}))

   while [ "${width}" -lt 1 ]
   do
      [ "${prefix%  }" != "${prefix}" ] || break
      prefix="${prefix%?}"
      width=$((width + 1))
   done

   local indent="$(stringReplaceAll "${prefix}" '.' ' ')"

   [ "${width}" -gt 0 ] || {
      addProgramUsageLine "${prefix}"
      indent="$(stringTail "${indent}" $((-width + 1)))"
      prefix="${indent}"
      width=1
   }

   eval set -- $(stringWrapped "${text}" "${width}")
   for line
   do
      addProgramUsageLine "${prefix}${line}"
      prefix="${indent}"
   done
}

showProgramUsageSummary() {
   programUsageLineCount=0
   set ${programOptionLetters}
   local width="${COLUMNS:-72}"

   local line="Usage: ${programName}"
   [ "${#}" -eq 0 ] || line="${line} [-option ...]"

   local index=0
   local suffix=""

   while [ "${index}" -lt "${programParameterCount}" ]
   do
      line="${line} "

      [ "${index}" -lt "${programParameterCountMinimum}" ] || {
         line="${line}["
         suffix="${suffix}]"
      }

      line="${line}$(getVariable "programParameterLabel_${index}")"
      index=$((index + 1))
   done

   line="${line}${suffix}"
   addProgramUsageLine "${line}"

   [ "${programParameterCount}" -eq 0 ] || {
      addProgramUsageLine "Parameters:"

      local indent=$((programParameterLabelWidth + 2))
      local index=0

      while [ "${index}" -lt "${programParameterCount}" ]
      do
         local line="$(getVariable "programParameterLabel_${index}")"

         while [ "${#line}" -lt "${indent}" ]
         do
            line="${line} "
         done

         local usage="$(getVariable "programParameterUsage_${index}")"
         local default="$(getVariable "programParameterDefault_${index}")"
         [ -z "${default}" ] || usage="${usage} (default is ${default})"
         addProgramUsageText "${usage}" "${width}" "  ${line}"

         index=$((index + 1))
      done
   }

   [ "${#}" -eq 0 ] || {
      addProgramUsageLine "Options:"

      local indent=$((3 + programOptionOperandWidth + 2))
      local letter

      for letter
      do
         local line="-${letter} $(getVariable "programOptionOperand_${letter}")"

         while [ "${#line}" -lt "${indent}" ]
         do
            line="${line} "
         done

         usage="$(getVariable "programOptionUsage_${letter}")"
         local default="$(getVariable "programOptionDefault_${letter}")"
         [ -z "${default}" ] || usage="${usage} (default is ${default})"
         addProgramUsageText "${usage}" "${width}" "  ${line}"
      done
   }

   local index=0
   while [ "${index}" -lt "${programUsageLineCount}" ]
   do
      getVariable "programUsageLine_${index}"
      index=$((index + 1))
   done
}

addProgramOption h flag programOption_showUsageSummary "show usage summary (this output), and then exit"
addProgramOption q counter programOption_quietCount "decrease output verbosity"
addProgramOption v counter programOption_verboseCount "increase output verbosity"

parseProgramOptions() {
   local letter

   while getopts ":${programOptionString}" letter
   do
      case "${letter}"
      in
        \?) syntaxError "unrecognized option: -${OPTARG}";;
         :) syntaxError "missing operand: -${OPTARG}";;

         *) local variable type
            setVariable variable "$(getVariable "programOptionVariable_${letter}")"
            setVariable type "$(getVariable "programOptionType_${letter}")"

            case "${type}"
            in
               counter) setVariable "${variable}" $((${variable} + 1));;
               flag) setVariable "${variable}" true;;
               list) setVariable "${variable}" "$(getVariable "${variable}") $(stringQuoted "${OPTARG}")";;
               string) setVariable "${variable}" "${OPTARG}";;
               *) internalError "unimplemented program option type: ${type} (-${letter})";;
            esac
            ;;
      esac
   done
}

parseProgramArguments() {
   [ "${programParameterCountMinimum}" -ge 0 ] || programParameterCountMinimum="${programParameterCount}"

   parseProgramOptions "${@}"
   shift $((OPTIND - 1))

   if "${programOption_showUsageSummary}"
   then
      showProgramUsageSummary
      exit 0
   fi

   local programParameterIndex=0
   while [ "${#}" -gt 0 ]
   do
      [ "${programParameterIndex}" -lt "${programParameterCount}" ] || syntaxError "too many parameters"
      setVariable "$(getVariable "programParameterVariable_${programParameterIndex}")" "${1}"
      shift 1
      programParameterIndex=$((programParameterIndex + 1))
   done

   [ "${programParameterIndex}" -ge "${programParameterCountMinimum}" ] || syntaxError "$(getVariable "programParameterLabel_${programParameterIndex}") not specified"
   readonly programLogLevel=$((programLogLevel + programOption_verboseCount - programOption_quietCount))
}

programDirectory="$(dirname "${0}")"
readonly programDirectory="$(resolveDirectory "${programDirectory}")"

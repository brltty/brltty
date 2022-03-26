###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2022 by The BRLTTY Developers.
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

testMode=false

readonly initialDirectory="$(pwd)"
readonly programName="$(basename "${0}")"

programMessage() {
   local message="${1}"

   [ -z "${message}" ] || echo >&2 "${programName}: ${message}"
}

setVariable() {
   eval "${1}"'="${2}"'
}

getVariable() {
   if [ -n "${2}" ]
   then
      eval "${2}"'="${'"${1}"'}"'
   else
      eval 'echo "${'"${1}"'}"'
   fi
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

defineEnumeration programLogLevel_ error warning notice task note detail
programLogLevel=$((${programLogLevel_task}))

logMessage() {
   local level="${1}"
   local message="${2}"

   local variable="programLogLevel_${level}"
   local value=$((${variable}))

   [ "${value}" -gt 0 ] || programMessage "unknown log level: ${level}"
   [ "${value}" -gt "${programLogLevel}" ] || programMessage "${message}"
}

logError() {
   logMessage error "${@}"
}

logWarning() {
   logMessage warning "${@}"
}

logNotice() {
   logMessage notice "${@}"
}

logTask() {
   logMessage task "${@}"
}

logNote() {
   logMessage note "${@}"
}

logDetail() {
   logMessage detail "${@}"
}

programTerminationCommandCount=0

runProgramTerminationCommands() {
   set +e

   while [ "${programTerminationCommandCount}" -gt 0 ]
   do
      set -- $(getVariable "programTerminationCommand${programTerminationCommandCount}")
      let "programTerminationCommandCount -= 1"

      local process="${1}"
      local directory="${2}"
      shift 2

      [ "${process}" = "${$}" ] && {
         cd "${directory}"
         "${@}"
      }
   done
}

pushProgramTerminationCommand() {
   [ "${programTerminationCommandCount}" -gt 0 ] || trap runProgramTerminationCommands exit
   setVariable "programTerminationCommand$((programTerminationCommandCount += 1))" "${$} $(pwd) ${*}"
}

needTemporaryDirectory() {
   [ -n "${temporaryDirectory}" ] || {
      umask 022
      [ -n "${TMPDIR}" -a -d "${TMPDIR}" -a -r "${TMPDIR}" -a -w "${TMPDIR}" -a -x "${TMPDIR}" ] || export TMPDIR="/tmp"
      temporaryDirectory="$(mktemp -d "${TMPDIR}/${programName}.$(date +"%Y%m%d-%H%M%S").XXXXXX")" && cd "${temporaryDirectory}" || exit "${?}"
      pushProgramTerminationCommand rm -f -r -- "${temporaryDirectory}"
   }
}

resolveDirectory() {
   local path="${1}"
   local variable="${2}"
   local absolute="$(cd "${path}" && pwd)"

   if [ -n "${variable}" ]
   then
      setVariable "${variable}" "${absolute}"
   else
      echo "${absolute}"
   fi
}

programDirectory="$(dirname "${0}")"
readonly programDirectory="$(resolveDirectory "${programDirectory}")"

toRelativePath() {
   local toPath="${1}"
   local variable="${2}"

   [ -n "${toPath}" ] || toPath="."
   resolveDirectory "${toPath}" toPath
   local fromPath="$(pwd)"

   [ "${fromPath%/}" = "${fromPath}" ] && fromPath+="/"
   [ "${toPath%/}" = "${toPath}" ] && toPath+="/"

   local fromLength="${#fromPath}"
   local toLength="${#toPath}"

   local limit=$(( (fromLength < toLength)? fromLength: toLength ))
   local index=0
   local start=0

   while (( index < limit ))
   do
      local fromChar="${fromPath:index:1}"
      local toChar="${toPath:index:1}"

      [ "${fromChar}" = "${toChar}" ] || break
      [ "${fromChar}" = "/" ] && start="$((index + 1))"
      let "index += 1"
   done

   fromPath="${fromPath:start}"
   toPath="${toPath:start}"

   while [ "${#fromPath}" -gt 0 ]
   do
      toPath="../${toPath}"
      fromPath="${fromPath#*/}"
   done

   [ -n "${toPath}" ] || toPath="."

   if [ -n "${variable}" ]
   then
      setVariable "${variable}" "${toPath}"
   else
      echo "${toPath}"
   fi
}

parseParameterString() {
   local valuesArray="${1}"
   local parameters="${2}"
   local code="${3}"

   set -- ${parameters//,/ }
   local parameter

   for parameter
   do
      local name="${parameter%%=*}"
      [ "${name}" = "${parameter}" ] && continue
      [ -n "${name}" ] || continue
      local value="${parameter#*=}"

      local qualifier="${name%%:*}"
      [ "${qualifier}" = "${name}" ] || {
         [ -n "${qualifier}" ] || continue
         [ "${qualifier}" = "${code}" ] || continue
         name="${name#*:}"
      }

      setVariable "${valuesArray}[${name^^*}]" "${value}"
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

syntaxError() {
   local message="${1}"

   logError "${message}"
   exit 2
}

semanticError() {
   local message="${1}"

   logError "${message}"
   exit 3
}

internalError() {
   local message="${1}"

   logError "${message}"
   exit 4
}

findHostCommand() {
   local pathVariable="${1}"
   local command="${2}"

   local path="$(which "${command}")"
   [ -n "${path}" ] || return 1

   setVariable "${pathVariable}" "${path}"
   return 0
}

verifyHostCommand() {
   local pathVariable="${1}"
   local command="${2}"

   findHostCommand "${pathVariable}" "${command}" || {
      semanticError "host command not found: ${command}"
   }
}

executeHostCommand() {
   logDetail "executing host command: ${*}"

   "${testMode}" || "${@}" || {
      local status="${?}"
      logWarning "host command failed with exit status ${status}: ${*}"
      return "${status}"
   }
}

verifyActionFlags() {
   local allFlag="${1}"
   shift 1

   local allRequested
   getVariable "${allFlag}" allRequested
   local actionFlag

   for actionFlag in "${@}"
   do
      local actionRequested
      getVariable "${actionFlag}" actionRequested

      "${actionRequested}" && {
         "${allRequested}" && syntaxError "conflicting actions"
         return
      }
   done

   "${allRequested}" || syntaxError "no actions"

   for actionFlag in "${@}"
   do
      setVariable "${actionFlag}" true
   done
}

testInteger() {
   local value="${1}"

   [ "${value}" = "0" ] || {
      value="${value#-}"
      [ -n "${value}" ] || return 1
      [ "$(expr "${value}" : '^[1-9][0-9]*$')" -eq "${#value}" ] || return 1
   }

   return 0
}

verifyInteger() {
   local label="${1}"
   local value="${2}"
   local minimum="${3}"
   local maximum="${4}"

   testInteger "${value}" || {
      semanticError "${label} not an integer: ${value}"
   }

   [ -n "${minimum}" ] && {
      [ "${value}" -lt "${minimum}" ] && {
         semanticError "${label} out of range: ${value} < ${minimum}"
      }
   }

   [ -n "${maximum}" ] && {
      [ "${value}" -gt "${maximum}" ] && {
         semanticError "${label} out of range: ${value} > ${maximum}"
      }
   }
}

testContainingDirectory() {
   local directory="${1}"
   shift 1

   local path
   for path
   do
      [ -e "${directory}/${path}" ] || return 1
   done

   return 0
}

findContainingDirectory() {
   local variable="${1}"
   local directory="${2}"
   shift 2

   local value
   getVariable "${variable}" value
   [ -n "${value}" ] && return 0

   while :
   do
      testContainingDirectory "${directory}" "${@}" && break
      local parent="$(dirname "${directory}")"
      [ "${parent}" = "${directory}" ] && return 1
      directory="${parent}"
   done

   export "${variable}"="${directory}"
}

testDirectory() {
   local path="${1}"

   [ -e "${path}" ] || return 1
   [ -d "${path}" ] || semanticError "not a directory: ${path}"
   return 0
}

verifyWritableDirectory() {
   local path="${1}"

   testDirectory "${path}" || semanticError "directory not found: ${path}"
   [ -w "${path}" ] || semanticError "directory not writable: ${path}"
}

testFile() {
   local path="${1}"

   [ -e "${path}" ] || return 1
   [ -f "${path}" ] || semanticError "not a file: ${path}"
   return 0
}

verifyInputFile() {
   local path="${1}"

   testFile "${path}" || semanticError "file not found: ${path}"
   [ -r "${path}" ] || semanticError "file not readable: ${path}"
}

verifyOutputFile() {
   local path="${1}"

   if testFile "${path}"
   then
      [ -w "${path}" ] || semanticError "file not writable: ${path}"
   else
      verifyWritableDirectory "$(dirname "${path}")"
   fi
}

verifyExecutableFile() {
   local path="${1}"

   testFile "${path}" || semanticError "file not found: ${path}"
   [ -x "${path}" ] || semanticError "file not executable: ${path}"
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
      optionalProgramParameterLabel="${1}"
      optionalProgramParameterUsage="${2}"
   else
      logWarning "program parameters are already optional"
   fi
}

tooManyProgramParameters() {
   syntaxError "too many parameters"
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

addTestModeOption() {
   addProgramOption t flag testMode "test mode - log (but don't execute) the host commands"
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

   [ -z "${optionalProgramParameterLabel}" ] || {
      line="${line} [${optionalProgramParameterLabel} ...]"

      [ -z "${optionalProgramParameterUsage}" ] || {
         addProgramParameter "${optionalProgramParameterLabel} ..." optionalProgramParameterVariable "${optionalProgramParameterUsage}"
      }
   }

   line="${line}${suffix}"
   addProgramUsageLine "${line}"

   [ "${programParameterCount}" -eq 0 ] || {
      addProgramUsageLine ""
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
         [ -z "${default}" ] || usage="${usage} - the default is ${default}"
         addProgramUsageText "${usage}" "${width}" "  ${line}"

         index=$((index + 1))
      done
   }

   [ "${#}" -eq 0 ] || {
      addProgramUsageLine ""
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
         [ -z "${default}" ] || usage="${usage} - the default is ${default}"
         addProgramUsageText "${usage}" "${width}" "  ${line}"
      done
   }

   local index=0
   while [ "${index}" -lt "${programUsageLineCount}" ]
   do
      getVariable "programUsageLine_${index}"
      index=$((index + 1))
   done

   showProgramUsageNotes
}

addProgramOption h flag programOption_showUsageSummary "show (this) usage summary, and then exit"
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
      [ "${programParameterIndex}" -lt "${programParameterCount}" ] || break
      setVariable "$(getVariable "programParameterVariable_${programParameterIndex}")" "${1}"
      shift 1
      programParameterIndex=$((programParameterIndex + 1))
   done

   [ "${programParameterIndex}" -ge "${programParameterCountMinimum}" ] || {
      syntaxError "$(getVariable "programParameterLabel_${programParameterIndex}") not specified"
   }

   readonly programLogLevel=$((programLogLevel + programOption_verboseCount - programOption_quietCount))
   processExtraProgramParameters "${@}"
}

####################################################################
# The following functions are stubs that may be copied into the    #
# main script and augmented. They need to be defined after this    #
# prologue is embeded and before the program arguments are parsed. #
####################################################################

showProgramUsageNotes() {
cat <<END_OF_PROGRAM_USAGE_NOTES
END_OF_PROGRAM_USAGE_NOTES
}

processExtraProgramParameters() {
   [ "${#}" -eq 0 ] || tooManyProgramParameters
}


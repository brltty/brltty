#!/bin/bash
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

. "$(dirname "${BASH_SOURCE[0]}")/brltty-prologue.sh"

getElement() {
   eval setVariable "\${1}" "\"\${${2}[\"${3}\"]}\""
}

setElement() {
   eval setVariable "\${1}[\"${2}\"]" "\"\${3}\""
}

setElements() {
   local _array="${1}"
   shift 1
   eval "${_array}=(\"\${@}\")"
}

appendElements() {
   local _array="${1}"
   shift 1
   eval "${_array}+=(\"\${@}\")"
}

prependElements() {
   local _array="${1}"
   shift 1
   eval setElements "\${_array}" "\"\${@}\" \"\${${_array}[@]}\""
}

shiftElements() {
   local _array="${1}"
   local _count="${2}"

   eval set -- "\"\${${_array}[@]}\""
   shift "${_count}"
   setElements "${_array}" "${@}"
}

getElementCount() {
   eval setVariable "\${1}" "\${#${2}[*]}"
}

getElementNames() {
   eval "${1}=(\"\${!${2}[@]}\")"
}

forElements() {
   local _array="${1}"
   shift 1

   local _names=()
   getElementNames _names "${_array}"

   local name
   for name in "${_names[@]}"
   do
      local value
      getElement value "${_array}" "${name}"
      "${@}" "${name}" "${value}"
   done
}

getElements() {
   local _toArray="${1}"
   local _fromArray="${2}"
   forElements "${_fromArray}" setElement "${_toArray}"
}

listElement() {
   local array="${1}"
   local name="${2}"
   local value="${3}"
   echo "${array}[${name}]: ${value}"
}

listElements() {
   local _array="${1}"
   forElements "${_array}" listElement "${_array}" | sort
}

writeElement() {
   local name="${1}"
   local value="${2}"
   echo "${name} ${value}"
}

writeElements() {
   local _array="${1}"
   forElements "${_array}" writeElement | sort
}

readElements() {
   local _array="${1}"

   local name value
   while read name value
   do
      setElement "${_array}" "${name}" "${value}"
   done
}

programConfigurationFilePrefixArray=()
makeProgramConfigurationFilePrefixArray() {
   [ "${#programConfigurationFilePrefixArray[*]}" -gt 0 ] || {
      programConfigurationFilePrefixArray+=("${programDirectory}/.")

      [ -n "${HOME}" ] && {
         programConfigurationFilePrefixArray+=("${HOME}/.config/${programName}/")
         programConfigurationFilePrefixArray+=("${HOME}/.")
      }

      programConfigurationFilePrefixArray+=("/etc/${programName}/")
      programConfigurationFilePrefixArray+=("/etc/xdg/${programName}/")
      programConfigurationFilePrefixArray+=("/etc/")
   }
}

findProgramConfigurationFile() {
   local fileVariable="${1}"
   shift 1

   [ "${#}" -gt 0 ] || set -- conf cfg
   local fileExtensions=("${@}")

   makeProgramConfigurationFilePrefixArray
   local prefix

   for prefix in "${programConfigurationFilePrefixArray[@]}"
   do
      local extension

      for extension in "${fileExtensions[@]}"
      do
         local _file="${prefix}${programName}.${extension}"
         [ -f "${_file}" ] || continue
         [ -r "${_file}" ] || continue

         setVariable "${fileVariable}" "${_file}"
         return 0
      done
   done

   return 1
}

programComponentDirectoryArray=()
makeProgramComponentDirectoryArray() {
   [ "${#programComponentDirectoryArray[*]}" -gt 0 ] || {
      local subdirectory="libexec"

      programComponentDirectoryArray+=("${PWD}")
      programComponentDirectoryArray+=("${PWD}/../${subdirectory}")

      programComponentDirectoryArray+=("${programDirectory}")
      programComponentDirectoryArray+=("${programDirectory}/../${subdirectory}")

      [ -n "${HOME}" ] && {
         programComponentDirectoryArray+=("${HOME}/.config/${programName}")
         programComponentDirectoryArray+=("${HOME}/${subdirectory}")
      }

      programComponentDirectoryArray+=("/usr/${subdirectory}")
      programComponentDirectoryArray+=("/${subdirectory}")

      programComponentDirectoryArray+=("/etc/${programName}")
      programComponentDirectoryArray+=("/etc/xdg/${programName}")
   }
}

findProgramComponent() {
   local fileVariable="${1}"
   local name="${2}"
   shift 2
   local fileExtensions=("${@}")

   if [ "${#fileExtensions[*]}" -eq 0 ]
   then
      fileExtensions=(bash sh)
   else
      name="${programName}.${name}"
   fi

   makeProgramComponentDirectoryArray
   local directory

   for directory in "${programComponentDirectoryArray[@]}"
   do
      local extension

      for extension in "${fileExtensions[@]}"
      do
         local _file="${directory}/${name}.${extension}"
         [ -f "${_file}" ] || continue
         [ -r "${_file}" ] || continue

         setVariable "${fileVariable}" "${_file}"
         return 0
      done
   done

   return 1
}

includeProgramComponent() {
   local name="${1}"

   local component
   findProgramComponent component "${name}" || {
      logWarning "program component not found: ${name}"
      return 1
   }

   . "${component}" || {
      logWarning "problem including program component: ${component}"
      return 2
   }

   logNote "program component included: ${component}"
   return 0
}

declare -A persistentProgramSettingsArray=()
persistentProgramSettingsChanged=false
readonly persistentProgramSettingsExtension="conf"

restorePersistentProgramSettins() {
   local settingsFile="${1}"
   local found=false

   if [ -n "${settingsFile}" ]
   then
      [ -e "${settingsFile}" ] && found=true
   elif findProgramConfigurationFile settingsFile "${persistentProgramSettingsExtension}"
   then
      found=true
   fi

   "${found}" && {
      logNote "restoring persistent program settings: ${settingsFile}"
      persistentProgramSettingsArray=()
      readElements persistentProgramSettingsArray <"${settingsFile}"
      persistentProgramSettingsChanged=false
   }
}

savePersistentProgramSettins() {
   local settingsFile="${1}"

   "${persistentProgramSettingsChanged}" && {
      [ -n "${settingsFile}" ] || {
         findProgramConfigurationFile settingsFile "${persistentProgramSettingsExtension}" && [ -f "${settingsFile}" -a -w "${settingsFile}" ] || {
            settingsFile=""
            local prefix

            for prefix in "${programConfigurationFilePrefixArray[@]}"
            do
               local directory="${prefix%/}"
               [ "${directory}" = "${prefix}" ] && continue

               if [ -e "${directory}" ]
               then
                  [ -d "${directory}" ] || continue
               else
                  mkdir --parents -- "${directory}" || continue
                  logNote "program configuration directory created: ${directory}"
               fi

               [ -w "${directory}" ] || continue
               settingsFile="${directory}/${programName}.${persistentProgramSettingsExtension}"
               break
            done

            [ -n "${settingsFile}" ] || {
               logWarning "no eligible program configuration directory"
               return 1
            }
         }
      }

      logNote "saving persistent program settings: ${settingsFile}"
      writeElements persistentProgramSettingsArray >"${settingsFile}"
      persistentProgramSettingsChanged=false
   }

   return 0
}

getPersistentProgramSetting() {
   local variable="${1}"
   local name="${2}"

   setVariable "${variable}" "${persistentProgramSettingsArray["${name}"]}"
}

changePersistentProgramSetting() {
   local name="${1}"
   local value="${2}"
   local -n setting="persistentProgramSettingsArray[\"${name}\"]"

   [ "${value}" = "${setting}" ] || {
      setting="${value}"
      persistentProgramSettingsChanged=true
   }
}

evaluateExpression() {
   local resultVariable="${1}"
   local expression="${2}"

   local _result
   _result="$(bc --quiet <<<"${expression}")" || return 1
   [ -n "${_result}" ] || return 1

   setVariable "${resultVariable}" "${_result}"
   return 0
}

if [ -n "${SRANDOM}" ]
then
   declare -n bashRandomNumberVariable="SRANDOM"
else
   declare -n bashRandomNumberVariable="RANDOM"
fi

getRandomInteger() {
   local resultVariable="${1}"
   local maximumValue="${2}"
   local minimumValue="${3:-1}"

   setVariable "${resultVariable}" $(( (bashRandomNumberVariable % (maximumValue - minimumValue + 1)) + minimumValue ))
}

convertUnit() {
   local resultVariable="${1}"
   local from="${2}"
   local to="${3}"
   local precision="${4}"

   local toValue
   toValue="$(units --terse --output-format "%.${precision:-0}f" "${from}" "${to}")" || return 1

   [ "${toValue}" = "${toValue%.*}" ] || {
      toValue="${toValue%%*(0)}"
      toValue="${toValue%.}"
   }

   setVariable "${resultVariable}" "${toValue}"
   return 0
}

convertSimpleUnit() {
   local resultVariable="${1}"
   local fromValue="${2}"
   local fromUnit="${3}"
   local toUnit="${4}"
   local precision="${5}"

   convertUnit "${resultVariable}" "${fromValue}${fromUnit}" "${toUnit}" "${precision}" || return 1
   return 0
}

formatSimpleUnit() {
   local variable="${1}"
   local fromUnit="${2}"
   local toUnit="${3}"
   local precision="${4}"

   local value="${!variable}"
   convertSimpleUnit value "${value}" "${fromUnit}" "${toUnit}" "${precision}" || return 1

   setVariable "${variable}" "${value}${toUnit}"
   return 0
}

convertComplexUnit() {
   local resultVariable="${1}"
   local fromValue="${2}"
   local fromUnit="${3}"
   local toUnit="${4}"
   local unitType="${5}"
   local precision="${6}"

   convertUnit "${resultVariable}" "${unitType}${fromUnit}(${fromValue})" "${unitType}${toUnit}" "${precision}" || return 1
   return 0
}

formatComplexUnit() {
   local variable="${1}"
   local fromUnit="${2}"
   local toUnit="${3}"
   local unitType="${4}"
   local precision="${5}"

   local value="${!variable}"
   convertComplexUnit value "${value}" "${fromUnit}" "${toUnit}" "${unitType}" "${precision}" || return 1

   setVariable "${variable}" "${value}${toUnit}"
   return 0
}

isAbbreviation() {
   local abbreviation="${1}"
   shift 1

   local length="${#abbreviation}"
   local word

   for word
   do
      [ "${length}" -le "${#word}" ] || continue
      [ "${abbreviation}" = "${word:0:length}" ] && return 0
   done

   return 1
}

verifyChoice() {
   local label="${1}"
   local valueVariable="${2}"
   shift 2

   local value
   getVariable "${valueVariable}" value

   local candidates=()
   local choice

   for choice
   do
      [ "${value}" = "${choice}" ] && return 0
      isAbbreviation "${value}" "${choice}" || continue
      candidates+=("${choice}")
   done

   local count="${#candidates[*]}"
   [ "${count}" -gt 0 ] || syntaxError "invalid ${label}: ${value}"

   [ "${count}" -eq 1 ] || {
      local message="ambiguous ${label}: ${value}"
      local delimiter=" ("

      for choice in "${candidates[@]}"
      do
         message+="${delimiter}${choice}"
         delimiter=", "
      done

      message+=")"
      syntaxError "${message}"
   }

   setVariable "${valueVariable}" "${candidates[0]}"
}


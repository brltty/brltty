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

programConfigurationFilePrefixes=()
findProgramConfigurationFile() {
   local fileVariable="${1}"
   shift 1

   [ "${#}" -gt 0 ] || set -- conf cfg
   local fileExtensions=("${@}")

   [ "${#programConfigurationFilePrefixes[*]}" -gt 0 ] || {
      programConfigurationFilePrefixes+=("${PWD}/.")

      [ -n "${HOME}" ] && {
         programConfigurationFilePrefixes+=("${HOME}/.config/${programName}/")
         programConfigurationFilePrefixes+=("${HOME}/.")
      }

      programConfigurationFilePrefixes+=("/etc/${programName}/")
      programConfigurationFilePrefixes+=("/etc/xdg/${programName}/")
      programConfigurationFilePrefixes+=("/etc/")
   }

   local prefix
   for prefix in "${programConfigurationFilePrefixes[@]}"
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

programComponentDirectories=()
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

   [ "${#programComponentDirectories[*]}" -gt 0 ] || {
      local subdirectory="libexec"

      programComponentDirectories+=("${PWD}")
      programComponentDirectories+=("${PWD}/../${subdirectory}")

      [ -n "${HOME}" ] && {
         programComponentDirectories+=("${HOME}/.config/${programName}")
         programComponentDirectories+=("${HOME}/${subdirectory}")
      }

      programComponentDirectories+=("/usr/${subdirectory}")
      programComponentDirectories+=("/${subdirectory}")

      programComponentDirectories+=("/etc/${programName}")
      programComponentDirectories+=("/etc/xdg/${programName}")
   }

   local directory
   for directory in "${programComponentDirectories[@]}"
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
   local fileVariable="${1}"
   local name="${2}"

   findProgramComponent "${fileVariable}" "${name}" || {
      logWarning "program component not found: ${name}"
      return 1
   }

   . "${!fileVariable}" || {
      logWarning "failure including program component: ${name}"
      return 2
   }

   logNote "program component included: ${name}"
   return 0
}

verifyChoice() {
   local label="${1}"
   local valueVariable="${2}"
   shift 2

   local value
   getVariable "${valueVariable}" value

   local candidates=()
   local length="${#value}"
   local choice

   for choice
   do
      [ "${value}" = "${choice}" ] && return 0
      [ "${length}" -le "${#choice}" ] || continue
      [ "${value}" = "${choice:0:length}" ] || continue
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


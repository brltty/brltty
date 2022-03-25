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


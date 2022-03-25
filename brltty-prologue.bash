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
   eval setVariable "${1}" "\${${2}[\"${3}\"]}"
}

setElement() {
   eval "${1}[\"${2}\"]=\"${3}\""
}

setElements() {
   local _array="${1}"
   shift 1
   eval "${_array}=("${@}")"
}

appendElements() {
   local _array="${1}"
   shift 1
   eval "${_array}+=("${@}")"
}

getElements() {
   eval "${1}=(\"\${!${2}[@]}\")"
}

listElements() {
   local _array="${1}"

   local _elements=()
   getElements _elements "${_array}"

   local element
   for element in "${_elements[@]}"
   do
      local value
      getElement value "${_array}" "${element}"
      echo "${_array}[${element}]: ${value}"
   done | sort
}


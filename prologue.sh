#!/bin/bash
. "$(dirname "${BASH_SOURCE[0]}")/Source/brltty-prologue.sh"

setSourceRoot() {
   findContainingDirectory BRLTTY_SOURCE_ROOT "${programDirectory}" brltty.pc.in || {
      semanticError "source tree not found"
   }

   readonly sourceRoot="${BRLTTY_SOURCE_ROOT}"
}

setBuildRoot() {
   local variable=BRLTTY_BUILD_ROOT
   set -- brltty.pc

   findContainingDirectory "${variable}" "${initialDirectory}" "${@}" || {
      findContainingDirectory "${variable}" "${programDirectory}" "${@}" || {
         semanticError "build tree not found"
      }
   }

   readonly buildRoot="${BRLTTY_BUILD_ROOT}"
}

readonly documentsSubdirectory="Documents"
readonly driversSubdirectory="Drivers"
readonly programsSubdirectory="Programs"
readonly tablesSubdirectory="Tables"
readonly toolsSubdirectory="Tools"


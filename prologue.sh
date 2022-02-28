#!/bin/bash
. "$(dirname "${BASH_SOURCE[0]}")/Source/brltty-prologue.sh"

addSourceTreeOption() {
   addProgramOption s string.directory sourceTree "the path to the source tree"
}

verifySourceTree() {
   [ -n "${sourceTree}" ] || syntaxError "source tree not specified"
   [ -e "${sourceTree}" ] || semanticError "source tree not found: ${sourceTree}"
   [ -d "${sourceTree}" ] || semanticError "source tree not a directory: ${sourceTree}"
   testContainingDirectory "${sourceTree}" brltty.pc.in || semanticError "not a BRLTTY source tree: ${sourceTree}"

   [ "${sourceTree#/}" = "${sourceTree}" ] && sourceTree="${initialDirectory}/${sourceTree}"
}

addBuildTreeOption() {
   addProgramOption b string.directory buildTree "the path to the build tree"
}

verifyBuildTree() {
   [ -n "${buildTree}" ] || syntaxError "build tree not specified"
   [ -e "${buildTree}" ] || semanticError "build tree not found: ${buildTree}"
   [ -d "${buildTree}" ] || semanticError "build tree not a directory: ${buildTree}"
   testContainingDirectory "${buildTree}" brltty.pc || semanticError "not a BRLTTY build tree: ${buildTree}"

   [ "${buildTree#/}" = "${buildTree}" ] && buildTree="${initialDirectory}/${buildTree}"
}

addInstallLocationOption() {
   addProgramOption i string.directory installLocation "the path to the install location"
}

verifyInstallLocation() {
   [ -n "${installLocation}" ] || syntaxError "install location not specified"
   [ -e "${installLocation}" ] || semanticError "install location not found: ${installLocation}"
   [ -d "${installLocation}" ] || semanticError "install location not a directory: ${installLocation}"
   [ "${installLocation#/}" = "${installLocation}" ] && installLocation="${initialDirectory}/${installLocation}"
}

getInstallTree() {
   local resultVariable="${1}"
   setVariable "${resultVariable}" "$("${sourceTree}/Tools/pkgvars" -b "${buildTree}" -- execute_root)"
}


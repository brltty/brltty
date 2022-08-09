#!/bin/bash

setPackageManager() {
   local -r commands=(
      /sbin/apk
      /usr/bin/dnf
      /usr/bin/dpkg
      /usr/sbin/pkg
      /usr/bin/zypper
   )

   local command
   for command in "${commands[@]}"
   do
      [ -x "${command}" ] && {
         declare -g -r packageManager="${command##*/}"
         return 0
      }
   done

   semanticError "unknown package manager"
}

normalizePackageList() {
   sort | uniq
}

unsupportedPackageAction() {
   local action="${1}"
   semanticError "unsupported package action: ${action}"
}

setPackageManager
. "${programDirectory}/packages.d/${packageManager}.sh"

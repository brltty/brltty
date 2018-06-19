#!/bin/bash

export BRLTTY_START_MESSAGE="initramfs starting"
export BRLTTY_STOP_MESSAGE="initramfs finished"

BRLTTY_OVERRIDE_PREFERENCE="braille-keyboard-enabled=yes"
BRLTTY_OVERRIDE_PREFERENCE+=",braille-input-mode=text"
export BRLTTY_OVERRIDE_PREFERENCE

export BRLTTY_SCREEN_DRIVER="lx"
export BRLTTY_SPEECH_DRIVER="no"

export BRLTTY_WRITABLE_DIRECTORY="/run"
export BRLTTY_PID_FILE="${BRLTTY_WRITABLE_DIRECTORY}/brltty.pid"
export BRLTTY_LOG_FILE="${BRLTTY_WRITABLE_DIRECTORY}/initramfs/brltty.log"

export BRLTTY_UPDATABLE_DIRECTORY="/etc"
export BRLTTY_PREFERENCES_FILE="${BRLTTY_UPDATABLE_DIRECTORY}/brltty.prefs"

brlttySetOption() {
   local option="${1}"
   local name="${option%%=*}"

   if [ "${name}" = "${option}" ]
   then
      local value="yes"
   else
      local value="${option#*=}"
   fi

   [ -z "${name}" ] || {
      name="${name^^?}"
      export "BRLTTY_${name}=${value}"
   }
}

brlttySetConfiguredOptions() {
   local file="/etc/brltty/Initramfs/cmdline"

   [ -f "${file}" ] && [ -r "${file}" ] && {
      local line

      while read line
      do
         set -- ${line%%#*}
         local option

         for option
         do
            brlttySetOption "${option}"
         done
      done <"${file}"
   }
}

brlttySetExplicitOptions() {
   local option

   for option
   do
      [[ "${option}" =~ ^"rd.brltty."(.*) ]] && {
         brlttySetOption "${BASH_REMATCH[1]}"
      }
   done
}

brlttySetConfiguredOptions
brlttySetExplicitOptions $(getcmdline)
getargbool 1 rd.brltty.sound || export BRLTTY_SPEECH_DRIVER="no"

getargbool 1 rd.brltty && (
   # Give the kernel a bit of time to finish creating the /dev/input/ devices
   # (e.g. so that brltty can perform keyboard discovery for keyboard tables)
   # without delaying the boot.

   sleep 1
   brltty -E +n
) &

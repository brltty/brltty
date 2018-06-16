#!/bin/bash

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

brlttyParseOptions() {
   local option

   for option
   do
      if [[ "${option}" =~ ^"rd."("brltty."[[:alpha:]_]+)"="(.+) ]]
      then
         local name="${BASH_REMATCH[1]}"
         local value="${BASH_REMATCH[2]}"

         name="${name^^?}" # convert to uppercase
         name="${name/./_}" # translate . to _

         export "${name}=${value}"
      fi
   done
}

brlttyParseOptions $(getcmdline)
brltty -E +n

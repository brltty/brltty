#!/bin/bash

brlttyParseOptions() {
	local option

	for option
	do
		if [[ "${option}" =~ ^("brltty."[[:alpha:]_]+)"="(.+) ]]
		then
                        local name="${BASH_REMATCH[1]}"
                        local value="${BASH_REMATCH[2]}"

			name="${name^^?}" # convert to uppercase
			name="${name/./_}" # translate . to _

			export "${name}=${value}"
		fi
	done
}

export BRLTTY_WRITABLE_DIRECTORY="/run/initramfs"
export BRLTTY_LOG_FILE="$BRLTTY_WRITABLE_DIRECTORY/brltty.log"

export BRLTTY_UPDATABLE_DIRECTORY="/etc"
export BRLTTY_PREFERENCES_FILE="$BRLTTY_UPDATABLE_DIRECTORY/brltty.prefs"

brlttyParseOptions $(getcmdline)

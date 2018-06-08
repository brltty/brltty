#/bin/bash

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

brlttyParseOptions $(getcmdline)

#!/bin/bash

# called by dracut
check() {
	require_binaries brltty || return 1
	return 0
}

# called by dracut
depends() {
	return 0
}

# called by dracut
install() {
	local word

	# get the shared libraries that the brltty executable requires
	local required_libraries="$(ldd /usr/bin/brltty | awk '{print $1}' | sed 's/\..*/.so\*/g')"
	for word in ${required_libraries}
	do
		if [ -e $word ]
		then
			inst_libdir_file "$word"
		fi
	done

	# install the Linux screen driver
	inst_libdir_file "brltty/libbrlttyxlx.so*"

	export BRLTTY_CONFIGURATION_FILE=/etc/brltty.conf
	inst_simple "$BRLTTY_CONFIGURATION_FILE"

	local BRLTTY_EXECUTABLE_PATH="/usr/bin/brltty"
	inst_simple "$BRLTTY_EXECUTABLE_PATH"
	local brltty_report="$(LC_ALL="${BRLTTY_DRACUT_LOCALE:-${LANG}}" "$BRLTTY_EXECUTABLE_PATH" -E -v -e -ldebug 2>&1)"
	
	local required_braille_drivers=$(echo "$brltty_report" | awk '/checking for braille driver:/ {print $NF}')
	for word in $required_braille_drivers
	do
		brlttyIncludeBrailleDriver "$word"
	done

	local required_data_files=$(echo "$brltty_report" | awk '/including data file:/ {print $NF}')
	for word in $required_data_files
	do
		inst "$word"
	done	

	if [ -n "$BRLTTY_DRACUT_BRAILLE_DRIVERS" ]
	then
		for word in $BRLTTY_DRACUT_BRAILLE_DRIVERS
		do
			brlttyIncludeBrailleDriver "$word"
		done
	fi
		
	brlttyIncludeTables Text ttb $BRLTTY_DRACUT_TEXT_TABLES
	brlttyIncludeTables Attributes atb $BRLTTY_DRACUT_ATTRIBUTES_TABLES
	brlttyIncludeTables Contraction ctb $BRLTTY_DRACUT_CONTRACTION_TABLES

	inst_hook cmdline 99 "$moddir/brltty-parse-options.sh"
	inst_hook initqueue 99 "$moddir/brltty-start.sh"
	inst_hook cleanup 99 "$moddir/brltty-cleanup.sh"

	dracut_need_initqueue
}

brlttyIncludeTables() {
	local subdirectory="$1"
	local extension="$2"
	shift 2
	local name

	for name
	do
		brlttyIncludeDataFile "/etc/brltty/$subdirectory/$name.$extension"
	done
}

brlttyIncludeBrailleDriver() {
	inst_libdir_file "brltty/libbrlttyb$1.so*"
	brlttyIncludeInputTables "$1"
}

brlttyIncludeInputTables() {
	brlttyIncludeDataFile "/etc/brltty/Input/$1/"*.ktb
}

brlttyIncludeDataFile() {
	local file

	while read -r file
	do
		inst "$file"
	done < <( brltty-lsinc "$@" )
}


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

#called by dracut
install() {
	local required_libs="$(ldd /usr/bin/brltty | awk '{print $1}' | sed 's/\..*/.so\*/g')"

	for word in $required_libs; do
		if [ -e $word ]; then
			inst_libdir_file "$word"
		fi
	done

	inst_libdir_file "brltty/libbrlttyxlx.so*"

	local brltty_report="$(LC_ALL="$BRLTTY_LOCALE" brltty -Evel7 2>&1)"
	
	local checked_braille_drivers=$(echo "$brltty_report" | grep "checking for braille driver:" | awk '{print $NF}')

	for word in $checked_braille_drivers; do
		inst_libdir_file "brltty/libbrlttyb$word.so*"
	done

	local text_tables=$(echo "$brltty_report" | grep -E "compiling text table|including data file"| awk '{print $NF}')
		
	for word in $text_tables; do
		inst "$word"
	done	

	local attributes=$(echo "$brltty_report" | grep "Attributes Table" | awk '{print $NF}')	

	for word in $attributes; do
		inst "/etc/brltty/Attributes/$word.atb"
	done

	if [ "$BRLTTY_DRACUT_INCLUDE_DRIVERS" ]; then
		for word in $BRLTTY_DRACUT_INCLUDE_DRIVERS; do
			inst_libdir_file "brltty/libbrltty$BRLTTY_DRACUT_INCLUDE_DRIVERS.so*"
		done
	fi
		
	if [ "$BRLTTY_DRACUT_INCLUDE_TEXT_FILES" ]; then
		for word in $BRLTTY_DRACUT_INCLUDE_TEXT_FILES; do
			inst "/etc/brltty/Text/$BRLTTY_DRACUT_INCLUDE_DATA_FILES"
		done
	fi

	inst_hook cmdline 99 "$moddir/parse-brltty-opts.sh"
	inst_hook initqueue 99 "$moddir/brltty-start.sh"
	inst_hook cleanup 99 "$moddir/brltty-cleanup.sh"

	inst_simple "/etc/brltty.conf"
	inst_simple "/usr/bin/brltty"

	dracut_need_initqueue
}

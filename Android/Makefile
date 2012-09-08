all: local.properties
	ant debug

local.properties:
	android update project --path .

clean:
	-rm -f local.properties
	-rm -f -r gen
	-rm -f -r bin/*

install: all
	adb install bin/BRLTTY-debug.apk

uninstall:
	adb uninstall org.a11y.BRLTTY.Android


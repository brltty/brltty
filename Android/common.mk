ANDROID_ASSETS_DIRECTORY = assets
ANDROID_BINARIES_DIRECTORY = bin
ANDROID_LIBRARIES_DIRECTORY = libs

ANDROID_PACKAGE_FILE = $(ANDROID_BINARIES_DIRECTORY)/$(ANDROID_PACKAGE_NAME)-debug.apk

ANDROID_LOCAL_FILES = local.properties

all:: $(ANDROID_LOCAL_FILES)
	ant debug

$(ANDROID_LOCAL_FILES):
	android update project --path .

clean::
	-rm -f $(ANDROID_LOCAL_FILES)
	-rm -f -r $(ANDROID_ASSETS_DIRECTORY)
	-rm -f -r $(ANDROID_BINARIES_DIRECTORY)
	-rm -f -r $(ANDROID_LIBRARIES_DIRECTORY)
	-rm -f -r gen

install: all
	adb install $(ANDROID_PACKAGE_FILE)

reinstall: all
	adb install -r $(ANDROID_PACKAGE_FILE)

uninstall:
	adb uninstall $(ANDROID_PACKAGE_PATH)


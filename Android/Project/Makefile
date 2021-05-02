ANDROID_DEBUG_VARIANT = debug
ANDROID_RELEASE_VARIANT = release

ANDROID_PACKAGE_EXTENSION = apk
ANDROID_BUNDLE_EXTENSION = aab

ANDROID_OUTPUT_SUBDIRECTORY = app/build/outputs
ANDROID_PACKAGE_SUBDIRECTORY = $(ANDROID_OUTPUT_SUBDIRECTORY)/apk
ANDROID_BUNDLE_SUBDIRECTORY = $(ANDROID_OUTPUT_SUBDIRECTORY)/bundle

ANDROID_DEBUG_PACKAGE = $(ANDROID_PACKAGE_SUBDIRECTORY)/$(ANDROID_DEBUG_VARIANT)/app-$(ANDROID_DEBUG_VARIANT).$(ANDROID_PACKAGE_EXTENSION)
ANDROID_RELEASE_PACKAGE = $(ANDROID_PACKAGE_SUBDIRECTORY)/$(ANDROID_RELEASE_VARIANT)/app-$(ANDROID_RELEASE_VARIANT).$(ANDROID_PACKAGE_EXTENSION)

ANDROID_DEBUG_BUNDLE = $(ANDROID_BUNDLE_SUBDIRECTORY)/$(ANDROID_DEBUG_VARIANT)/app-$(ANDROID_DEBUG_VARIANT).$(ANDROID_BUNDLE_EXTENSION)
ANDROID_RELEASE_BUNDLE = $(ANDROID_BUNDLE_SUBDIRECTORY)/$(ANDROID_RELEASE_VARIANT)/app-$(ANDROID_RELEASE_VARIANT).$(ANDROID_BUNDLE_EXTENSION)

all: debug
debug: app-debug
release: app-release
install: install-debug

core-debug:
	./gradlew -q :core:assembleDebug

core-release:
	./gradlew -q :core:assembleRelease

app-debug:
	./gradlew -q :app:assembleDebug

app-release:
	./gradlew -q :app:assembleRelease

bundle:
	./gradlew -q :app:bundleRelease

install-debug:
	adb install -r -d $(ANDROID_DEBUG_PACKAGE)

install-release:
	adb install -r $(ANDROID_RELEASE_PACKAGE)

lint:
	./gradlew -q lintDebug

clean:
	./gradlew -q clean

purge: clean
	-rm -f -r .gradle


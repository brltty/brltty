all: debug
debug: app-debug
release: app-release
bundle: bundle-release
lint: lint-debug
install: install-debug
include gradle.mk

core-debug:
	$(GRADLE_COMMAND) :$(GRADLE_CORE_NAME):assembleDebug

core-release:
	$(GRADLE_COMMAND) :$(GRADLE_CORE_NAME):assembleRelease

app-debug:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):assembleDebug

app-release:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):assembleRelease

bundle-debug:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):bundleDebug

bundle-release:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):bundleRelease

lint-debug:
	$(GRADLE_COMMAND) lintDebug

lint-release:
	$(GRADLE_COMMAND) lintRelease

install-debug:
	adb install -r -d $(GRADLE_DEBUG_PACKAGE)

install-release:
	adb install -r $(GRADLE_RELEASE_PACKAGE)

useful-links:
	ln -s -f $(GRADLE_DEBUG_PACKAGE) $(GRADLE_ROOT_NAME)-$(GRADLE_DEBUG_VARIANT).$(GRADLE_PACKAGE_EXTENSION)
	ln -s -f $(GRADLE_RELEASE_PACKAGE) $(GRADLE_ROOT_NAME)-$(GRADLE_RELEASE_VARIANT).$(GRADLE_PACKAGE_EXTENSION)
	ln -s -f $(GRADLE_DEBUG_BUNDLE) $(GRADLE_ROOT_NAME)-$(GRADLE_DEBUG_VARIANT).$(GRADLE_BUNDLE_EXTENSION)
	ln -s -f $(GRADLE_RELEASE_BUNDLE) $(GRADLE_ROOT_NAME)-$(GRADLE_RELEASE_VARIANT).$(GRADLE_BUNDLE_EXTENSION)
	ln -s -f $(GRADLE_DEBUG_REPORT) $(GRADLE_REPORT_NAME)-$(GRADLE_DEBUG_VARIANT).$(GRADLE_REPORT_EXTENSION)
	ln -s -f $(GRADLE_RELEASE_REPORT) $(GRADLE_REPORT_NAME)-$(GRADLE_RELEASE_VARIANT).$(GRADLE_REPORT_EXTENSION)

clean:
	$(GRADLE_COMMAND) clean

uninstall-assets: uninstall-drivers uninstall-messages uninstall-tables

uninstall-drivers:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):uninstallDrivers

uninstall-messages:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):uninstallMessages

uninstall-tables:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):uninstallTables

purge: clean
	-rm -f -r .gradle


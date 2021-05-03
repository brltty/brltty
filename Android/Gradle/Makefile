all: debug
core: core-debug
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

assets: drivers messages tables

drivers:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):addDrivers

messages:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):addMessages

tables:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):addTables

remove-assets: remove-drivers remove-messages remove-tables

remove-drivers:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):removeDrivers

remove-messages:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):removeMessages

remove-tables:
	$(GRADLE_COMMAND) :$(GRADLE_APP_NAME):removeTables

clean:
	$(GRADLE_COMMAND) clean

clean-more: clean
	-rm -f -r .gradle

badging-dump configurations-dump permissions-dump resources-dump strings-dump xmlstrings-dump xmltree-dump: app-debug
	$(GRADLE_DUMP_COMMAND) --values $(@:-dump=) $(GRADLE_DEBUG_PACKAGE) >$(GRADLE_ROOT_NAME)-$(@:-dump=).$(GRADLE_DUMP_EXTENSION)

clean-dumps:
	-rm -f $(GRADLE_ROOT_NAME)-*.$(GRADLE_DUMP_EXTENSION)

useful-links:
	ln -s -f $(GRADLE_DEBUG_PACKAGE) $(GRADLE_ROOT_NAME)-$(GRADLE_DEBUG_VARIANT).$(GRADLE_PACKAGE_EXTENSION)
	ln -s -f $(GRADLE_RELEASE_PACKAGE) $(GRADLE_ROOT_NAME)-$(GRADLE_RELEASE_VARIANT).$(GRADLE_PACKAGE_EXTENSION)
	ln -s -f $(GRADLE_DEBUG_BUNDLE) $(GRADLE_ROOT_NAME)-$(GRADLE_DEBUG_VARIANT).$(GRADLE_BUNDLE_EXTENSION)
	ln -s -f $(GRADLE_RELEASE_BUNDLE) $(GRADLE_ROOT_NAME)-$(GRADLE_RELEASE_VARIANT).$(GRADLE_BUNDLE_EXTENSION)
	ln -s -f $(GRADLE_DEBUG_REPORT) $(GRADLE_REPORT_NAME)-$(GRADLE_DEBUG_VARIANT).$(GRADLE_REPORT_EXTENSION)
	ln -s -f $(GRADLE_RELEASE_REPORT) $(GRADLE_REPORT_NAME)-$(GRADLE_RELEASE_VARIANT).$(GRADLE_REPORT_EXTENSION)


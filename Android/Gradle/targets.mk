core-debug:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_CORE_NAME):assembleDebug

core-release:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_CORE_NAME):assembleRelease

api-debug:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_API_NAME):assembleDebug

api-release:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_API_NAME):assembleRelease

api-publish:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_API_NAME):publishReleasePublicationToMavenLocal

apitest-debug:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_APITEST_NAME):assembleDebug

apitest-release:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_APITEST_NAME):assembleRelease

app-debug:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_APP_NAME):assembleDebug

app-release:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_APP_NAME):assembleRelease

bundle-debug:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_APP_NAME):bundleDebug

bundle-release:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_APP_NAME):bundleRelease

lint-debug:
	$(GRADLE_WRAPPER_COMMAND) lintDebug

lint-release:
	$(GRADLE_WRAPPER_COMMAND) lintRelease

clean-build:
	$(GRADLE_WRAPPER_COMMAND) clean

install-debug: app-debug
	adb install -r -d $(GRADLE_DEBUG_PACKAGE)

install-release: app-release
	adb install -r $(GRADLE_RELEASE_PACKAGE)

assets: messages tables

messages:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_APP_NAME):brlttyAddMessages

tables:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_APP_NAME):brlttyAddTables

remove-assets: remove-messages remove-tables

remove-messages:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_APP_NAME):brlttyRemoveMessages

remove-tables:
	$(GRADLE_WRAPPER_COMMAND) :$(GRADLE_APP_NAME):brlttyRemoveTables

tasks-dump:
	$(GRADLE_WRAPPER_COMMAND) $(GRADLE_DUMP_NAME) tasks --all >$(GRADLE_DUMP_FILE)

badging-dump configurations-dump permissions-dump resources-dump strings-dump xmlstrings-dump xmltree-dump: app-debug
	$(GRADLE_DUMP_COMMAND) $(GRADLE_DUMP_NAME) $(GRADLE_DEBUG_PACKAGE) >$(GRADLE_DUMP_FILE)

symlinks: package-symlinks bundle-symlinks lint-symlinks

package-symlinks:
	$(GRADLE_SYMLINK_COMMAND) $(GRADLE_DEBUG_PACKAGE) $(GRADLE_ROOT_NAME)-$(GRADLE_DEBUG_VARIANT).$(GRADLE_PACKAGE_EXTENSION)
	$(GRADLE_SYMLINK_COMMAND) $(GRADLE_RELEASE_PACKAGE) $(GRADLE_ROOT_NAME)-$(GRADLE_RELEASE_VARIANT).$(GRADLE_PACKAGE_EXTENSION)

bundle-symlinks:
	$(GRADLE_SYMLINK_COMMAND) $(GRADLE_DEBUG_BUNDLE) $(GRADLE_ROOT_NAME)-$(GRADLE_DEBUG_VARIANT).$(GRADLE_BUNDLE_EXTENSION)
	$(GRADLE_SYMLINK_COMMAND) $(GRADLE_RELEASE_BUNDLE) $(GRADLE_ROOT_NAME)-$(GRADLE_RELEASE_VARIANT).$(GRADLE_BUNDLE_EXTENSION)

lint-symlinks:
	$(GRADLE_SYMLINK_COMMAND) $(GRADLE_DEBUG_REPORT) $(GRADLE_REPORT_NAME)-$(GRADLE_DEBUG_VARIANT).$(GRADLE_REPORT_EXTENSION)
	$(GRADLE_SYMLINK_COMMAND) $(GRADLE_RELEASE_REPORT) $(GRADLE_REPORT_NAME)-$(GRADLE_RELEASE_VARIANT).$(GRADLE_REPORT_EXTENSION)

get-screen-log:
	adb pull /sdcard/Android/data/org.a11y.brltty.android/files/screen.log

clean::
	-rm -f $(GRADLE_ROOT_NAME)-*.$(GRADLE_PACKAGE_EXTENSION)
	-rm -f $(GRADLE_ROOT_NAME)-*.$(GRADLE_BUNDLE_EXTENSION)
	-rm -f $(GRADLE_ROOT_NAME)-*.$(GRADLE_REPORT_EXTENSION)
	-rm -f $(GRADLE_ROOT_NAME)-*.$(GRADLE_DUMP_EXTENSION)

distclean::
	-rm -f config.properties
	-rm -f -r ../ABI


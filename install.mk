install: build
	sudo $(MAKE) --silent -C $(BUILD_TREE)/Programs -- install-pkgconfig-file install-commands install-tools install-manpages install-drivers install-tables install-core-headers install-api-libraries install-api-commands install-xbrlapi install-api-headers install-api-bindings
	sudo $(MAKE) --silent -C $(BUILD_TREE) -- install-systemd install-udev
	Make/add-files -s $(SOURCE_TREE) -b $(BUILD_TREE)

uninstall:
	sudo $(MAKE) --silent -C $(BUILD_TREE) -- $@

symlinks:
	Make/symlink-current -s $(SOURCE_TREE) -b $(BUILD_TREE)
	Make/symlink-bindings


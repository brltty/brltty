CLONE_TARGET = $(SOURCE_TREE)/configure.ac
clone: $(CLONE_TARGET)
$(CLONE_TARGET):
	git clone --quiet --branch master -- https://github.com/brltty/brltty $(SOURCE_TREE)
	sudo $(SOURCE_TREE)/Tools/reqpkgs -q -i

AUTOGEN_TARGET = $(SOURCE_TREE)/configure
autogen: $(AUTOGEN_TARGET)
$(AUTOGEN_TARGET): $(CLONE_TARGET)
	$(SOURCE_TREE)/autogen

build-tree: $(BUILD_TREE)
$(BUILD_TREE):
	install --directory $(BUILD_TREE)

CONFIGURE_TARGET = $(BUILD_TREE)/Makefile
configure: $(CONFIGURE_TARGET)
$(CONFIGURE_TARGET): $(AUTOGEN_TARGET) | build-tree
	Make/configure-brltty -s $(SOURCE_TREE) -b $(BUILD_TREE) -i $(INSTALL_LOCATION)

build: $(CONFIGURE_TARGET)
	$(MAKE) --silent -C $(BUILD_TREE)/Programs -- everything

unconfigure:
	-rm -f -r -- $(BUILD_TREE)
	-rm -f -- $(AUTOGEN_TARGET)


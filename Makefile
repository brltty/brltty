all: build

SOURCE_TREE = Source
BUILD_TREE = Build
INSTALL_LOCATION = /brltty

CLONE_TARGET = $(SOURCE_TREE)/configure.ac
clone: $(CLONE_TARGET)
$(CLONE_TARGET):
	git clone --quiet -- https://github.com/brltty/brltty $(SOURCE_TREE)

AUTOGEN_TARGET = $(SOURCE_TREE)/configure
autogen: $(AUTOGEN_TARGET)
$(AUTOGEN_TARGET): $(CLONE_TARGET)
	$(SOURCE_TREE)/autogen
	install --directory $(BUILD_TREE)

CONFIGURE_TARGET = $(BUILD_TREE)/Makefile
configure: $(CONFIGURE_TARGET)
$(CONFIGURE_TARGET): $(AUTOGEN_TARGET)
	./configure-brltty -s $(SOURCE_TREE) -b $(BUILD_TREE) -i $(INSTALL_LOCATION)

build: $(CONFIGURE_TARGET)
	$(MAKE) --silent -C $(BUILD_TREE)/Programs -- everything

install: build
	sudo $(MAKE) --silent -C $(BUILD_TREE)/Programs -- install

uninstall:
	sudo $(MAKE) --silent -C $(BUILD_TREE) -- $@

clean distclean:
	$(MAKE) --silent -C $(BUILD_TREE) -- $@

unconfigure:
	-rm -f -r -- $(BUILD_TREE)
	-rm -f -- $(AUTOGEN_TARGET)

install-required-packages: $(CLONE_TARGET)
	$(SOURCE_TREE)/Tools/reqpkgs -q -i

set-tty-size: $(CLONE_TARGET)
	$(SOURCE_TREE)/Tools/ttysize -c 40 -l 9


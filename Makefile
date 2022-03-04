all: build

SOURCE_TREE = Source
BUILD_TREE = Build
INSTALL_LOCATION = /brltty
SYSTEMD_UNIT = brltty@canute.path

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

unconfigure:
	-rm -f -r -- $(BUILD_TREE)
	-rm -f -- $(AUTOGEN_TARGET)

install: build
	sudo $(MAKE) --silent -C $(BUILD_TREE)/Programs -- install-commands install-tools install-manpages install-drivers install-tables install-core-headers install-api-libraries install-api-commands install-xbrlapi install-api-headers install-api-bindings
	sudo $(MAKE) --silent -C $(BUILD_TREE) -- install-systemd install-udev
	./add-files -s $(SOURCE_TREE) -b $(BUILD_TREE)

uninstall:
	sudo $(MAKE) --silent -C $(BUILD_TREE) -- $@

current:
	./symlink-current -s $(SOURCE_TREE) -b $(BUILD_TREE)

INSTALL_CURRENT = $(INSTALL_LOCATION)/current
INSTALL_TREE = $(shell readlink -n -e -- $(INSTALL_CURRENT))

TAR_ARCHIVE = canute-brltty.tar
tar-archive: $(TAR_ARCHIVE)
$(TAR_ARCHIVE):
	tar --file $(TAR_ARCHIVE) --create --absolute-names -- $(INSTALL_TREE) $(INSTALL_CURRENT)

GZIP_ARCHIVE = $(TAR_ARCHIVE).gz
gzip-archive: $(GZIP_ARCHIVE)
$(GZIP_ARCHIVE): $(TAR_ARCHIVE)
	gzip -9 -c $(TAR_ARCHIVE) >$@

BZIP2_ARCHIVE = $(TAR_ARCHIVE).bz2
bzip2-archive: $(BZIP2_ARCHIVE)
$(BZIP2_ARCHIVE): $(TAR_ARCHIVE)
	bzip2 -9 -c $(TAR_ARCHIVE) >$@

XZ_ARCHIVE = $(TAR_ARCHIVE).xz
xz-archive: $(XZ_ARCHIVE)
$(XZ_ARCHIVE): $(TAR_ARCHIVE)
	xz -9 -c $(TAR_ARCHIVE) >$@

clean-archives:
	-rm -f -- *.tar *.tar.*

status:
	systemctl $@ $(SYSTEMD_UNIT)

enable disable start stop:
	sudo systemctl $@ $(SYSTEMD_UNIT)

restart:
	sudo systemctl stop $(SYSTEMD_UNIT)
	sudo systemctl start $(SYSTEMD_UNIT)

install-required-packages: $(CLONE_TARGET)
	$(SOURCE_TREE)/Tools/reqpkgs -q -i

set-tty-size: $(CLONE_TARGET)
	$(SOURCE_TREE)/Tools/ttysize -c 40 -l 9


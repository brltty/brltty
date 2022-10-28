all: build

SOURCE_TREE = Source
BUILD_TREE = Build
INSTALL_LOCATION = /brltty

include build.mk
include install.mk
include archives.mk
include systemd.mk

install-required-packages: $(CLONE_TARGET)
	sudo $(SOURCE_TREE)/Tools/reqpkgs -q -i

set-tty-size: $(CLONE_TARGET)
	$(SOURCE_TREE)/Tools/ttysize -c 40 -l 9


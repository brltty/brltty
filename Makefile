all: build

SOURCE_TREE = Source
BUILD_TREE = Build
INSTALL_LOCATION = /brltty

include build.mk
include install.mk
include archives.mk
include systemd.mk

update-package-metadata:
	sudo apt --quiet --quiet --quiet -- update

install-required-packages: | clone
	sudo $(SOURCE_TREE)/Tools/reqpkgs -q -i

set-screen-size: $(CLONE_TARGET)
	$(SOURCE_TREE)/Tools/ttysize -c 40 -l 9

archives:
	$(MAKE) --directory Archives

document:
	$(MAKE) --directory Document

download: archives document


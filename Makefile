all: build

SOURCE_TREE = Source
BUILD_TREE = Build
INSTALL_LOCATION = /brltty

ARCHIVES_DIRECTORY = Archives
DOCUMENTS_DIRECTORY = Documents
RELEASE_DIRECTORY = Release

include build.mk
include install.mk
include archives.mk
include documents.mk
include systemd.mk

update-package-metadata:
	sudo apt --quiet --quiet --quiet -- update

install-required-packages: | clone
	sudo $(SOURCE_TREE)/Tools/reqpkgs -q -i

set-screen-size: $(CLONE_TARGET)
	$(SOURCE_TREE)/Tools/ttysize -c 40 -l 9

release: archives documents
	rm -f -- $(RELEASE_DIRECTORY)/*
	cp --archive -- $(DOCUMENTS_DIRECTORY)/software.html $(RELEASE_DIRECTORY)/index.html
	cp --archive -- $(ARCHIVES_DIRECTORY)/*.tar.* $(RELEASE_DIRECTORY)/
	cp --archive -- Files/libexec/canute-install $(RELEASE_DIRECTORY)/
	cp --archive -- README.txt $(RELEASE_DIRECTORY)/


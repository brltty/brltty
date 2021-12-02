###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2021 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any
# later version. Please see the file LICENSE-LGPL for details.
#
# Web Page: http://brltty.app/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

SOURCE_DIRECTORY = Source
BUILD_DIRECTORY = Build

OBJECTS_DIRECTORY = $(BUILD_DIRECTORY)/liblouis
OBJECT_FILES = compileTranslationTable.c commonTranslationFunctions.c lou_translateString.c lou_backTranslateString.c pattern.c metadata.c logging.c

CHECKOUT_TARGET = $(SOURCE_DIRECTORY)/configure.ac
AUTOGEN_TARGET = $(SOURCE_DIRECTORY)/configure
CONFIGURE_TARGET = $(BUILD_DIRECTORY)/Makefile

all: build
checkout: $(CHECKOUT_TARGET)
autogen: $(AUTOGEN_TARGET)
configure: $(CONFIGURE_TARGET)

build: $(CONFIGURE_TARGET)
	$(MAKE) --silent -C $(OBJECTS_DIRECTORY) -- $(OBJECT_FILES)

$(CONFIGURE_TARGET): $(AUTOGEN_TARGET)
	cd $(BUILD_DIRECTORY) && ../Source//configure --quiet

$(AUTOGEN_TARGET): $(CHECKOUT_TARGET)
	cd $(SOURCE_DIRECTORY) && ./autogen.sh
	install --directory $(BUILD_DIRECTORY)

$(CHECKOUT_TARGET):
	git submodule update --init -- $(SOURCE_DIRECTORY)

clean distclean:
	$(MAKE) -C $(BUILD_DIRECTORY) -- $@


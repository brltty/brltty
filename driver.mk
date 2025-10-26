###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2025 by The BRLTTY Developers.
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

DRIVER_OBJECTS ?= $(DRIVER_TYPE).$O

DRIVER_DEFINES = '-DDRIVER_NAME=$(DRIVER_NAME)' '-DDRIVER_CODE=$(DRIVER_CODE)' '-DDRIVER_COMMENT="$(DRIVER_COMMENT)"' '-DDRIVER_VERSION="$(DRIVER_VERSION)"' '-DDRIVER_DEVELOPERS="$(DRIVER_DEVELOPERS)"'
DRIVER_CFLAGS = $(LIBCFLAGS) $(DRIVER_DEFINES)
DRIVER_CXXFLAGS = $(LIBCXXFLAGS) $(DRIVER_DEFINES)

DRIVER_ARCHIVE = $(DRIVER_TYPE).$(ARC_EXT)
driver-archive: $(DRIVER_ARCHIVE)
$(DRIVER_ARCHIVE): $(DRIVER_OBJECTS)
	$(AR) rcs $@ $(DRIVER_OBJECTS)

DRIVER_MODULE_NAME = $(BLD_TOP)$(DRV_DIR)/$(MOD_NAME)$(DRIVER_LETTER)$(DRIVER_CODE)
DRIVER_MODULE = $(DRIVER_MODULE_NAME).$(MOD_EXT)
driver-module: $(DRIVER_MODULE)
$(DRIVER_MODULE): $(DRIVER_OBJECTS)
	$(INSTALL_DIRECTORY) $(@D)
	$(MKSHR) $(@) $(DRIVER_OBJECTS) $(DRIVER_DEPENDENCIES)

%.$O: $(SRC_TOP)$(PGM_DIR)/%.c $(SRC_TOP)$(HDR_DIR)/%.h
	$(CC) $(DRIVER_CFLAGS) -c $<

clean::
	-rm -f $(DRIVER_MODULE_NAME).*
	-rm -f $(DRIVER_ARCHIVE)


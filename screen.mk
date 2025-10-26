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

SCR_OBJECTS ?= screen.$O

SCR_DEFS ='-DDRIVER_NAME=$(DRIVER_NAME)' '-DDRIVER_CODE=$(DRIVER_CODE)' '-DDRIVER_COMMENT="$(DRIVER_COMMENT)"' '-DDRIVER_VERSION="$(DRIVER_VERSION)"' '-DDRIVER_DEVELOPERS="$(DRIVER_DEVELOPERS)"'
SCR_CFLAGS = $(LIBCFLAGS) $(SCR_DEFS)
SCR_CXXFLAGS = $(LIBCXXFLAGS) $(SCR_DEFS)

SCR_ARCHIVE = screen.$(ARC_EXT)
screen-archive: $(SCR_ARCHIVE)
$(SCR_ARCHIVE): $(SCR_OBJECTS)
	$(AR) rcs $@ $(SCR_OBJECTS)

SCR_MOD_NAME = $(BLD_TOP)$(DRV_DIR)/$(MOD_NAME)x$(DRIVER_CODE)
SCR_MOD_FILE = $(SCR_MOD_NAME).$(MOD_EXT)
screen-driver: $(SCR_MOD_FILE)
$(SCR_MOD_FILE): $(SCR_ARCHIVE)
	$(INSTALL_DIRECTORY) $(@D)
	$(MKSHR) $(@) $(SCR_ARCHIVE) $(SCR_OBJS)

%.$O: $(SRC_TOP)$(PGM_DIR)/%.c $(SRC_TOP)$(HDR_DIR)/%.h
	$(CC) $(SCR_CFLAGS) -c $<

install::

uninstall::

clean::
	-rm -f $(SCR_MOD_NAME).*
	-rm -f $(SCR_ARCHIVE)


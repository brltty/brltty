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

SPK_OBJECTS ?= speech.$O

SPK_DEFS ='-DDRIVER_NAME=$(DRIVER_NAME)' '-DDRIVER_CODE=$(DRIVER_CODE)' '-DDRIVER_COMMENT="$(DRIVER_COMMENT)"' '-DDRIVER_VERSION="$(DRIVER_VERSION)"' '-DDRIVER_DEVELOPERS="$(DRIVER_DEVELOPERS)"'
SPK_CFLAGS = $(LIBCFLAGS) $(SPK_DEFS)
SPK_CXXFLAGS = $(LIBCXXFLAGS) $(SPK_DEFS)

SPK_ARCHIVE = speech.$(ARC_EXT)
speech-archive: $(SPK_ARCHIVE)
$(SPK_ARCHIVE): $(SPK_OBJECTS)
	$(AR) rcs $@ $(SPK_OBJECTS)

SPK_MOD_NAME = $(BLD_TOP)$(DRV_DIR)/$(MOD_NAME)s$(DRIVER_CODE)
SPK_MOD_FILE = $(SPK_MOD_NAME).$(MOD_EXT)
speech-driver: $(SPK_MOD_FILE)
$(SPK_MOD_FILE): $(spk_ARCHIVE)
	$(INSTALL_DIRECTORY) $(@D)
	$(MKSHR) $(@) $(spk_ARCHIVE) $(SPK_OBJS)

%.$O: $(SRC_TOP)$(PGM_DIR)/%.c $(SRC_TOP)$(HDR_DIR)/%.h
	$(CC) $(spk_CFLAGS) -c $<

install::

uninstall::

clean::
	-rm -f $(SPK_MOD_NAME).*
	-rm -f $(SPK_ARCHIVE)


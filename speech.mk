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

SPK_DEFS ='-DDRIVER_NAME=$(DRIVER_NAME)' '-DDRIVER_CODE=$(DRIVER_CODE)' '-DDRIVER_COMMENT="$(DRIVER_COMMENT)"' '-DDRIVER_VERSION="$(DRIVER_VERSION)"' '-DDRIVER_DEVELOPERS="$(DRIVER_DEVELOPERS)"'
SPK_CFLAGS = $(LIBCFLAGS) $(SPK_DEFS)
SPK_CXXFLAGS = $(LIBCXXFLAGS) $(SPK_DEFS)
SPK_MOD_NAME = $(BLD_TOP)$(DRV_DIR)/$(MOD_NAME)s$(DRIVER_CODE)
SPK_MOD_FILE = $(SPK_MOD_NAME).$(MOD_EXT)
$(SPK_MOD_FILE): speech.$O
	$(INSTALL_DIRECTORY) $(@D)
	$(MKSHR) $(@) speech.$O $(SPK_OBJS)
speech-driver: $(SPK_MOD_FILE)

install::

uninstall::

clean::
	-rm -f $(SPK_MOD_NAME).*

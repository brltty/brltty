###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2010 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any
# later version. Please see the file LICENSE-GPL for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

SCR_DEFS ='-DDRIVER_NAME=$(DRIVER_NAME)' '-DDRIVER_CODE=$(DRIVER_CODE)' '-DDRIVER_COMMENT="$(DRIVER_COMMENT)"' '-DDRIVER_VERSION="$(DRIVER_VERSION)"' '-DDRIVER_DEVELOPERS="$(DRIVER_DEVELOPERS)"'
SCR_CFLAGS = $(LIBCFLAGS) $(SCR_DEFS)
SCR_CXXFLAGS = $(LIBCXXFLAGS) $(SCR_DEFS)
SCR_MOD_NAME = $(BLD_TOP)$(DRV_DIR)/$(MOD_NAME)x$(DRIVER_CODE)
SCR_MOD_FILE = $(SCR_MOD_NAME).$(MOD_EXT)
$(SCR_MOD_FILE): screen.$O
	$(INSTALL_DIRECTORY) $(@D)
	$(MKMOD) $(@) screen.$O $(SCR_OBJS)
screen-driver: $(SCR_MOD_FILE)

install::

uninstall::

clean::
	-rm -f $(SCR_MOD_NAME).*

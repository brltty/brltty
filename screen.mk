###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

SCRNAMES = $(BLD_TOP)$(DRV_DIR)/brltty-scr.lst
SCR_DEFS ='-DSCRNAME=$(DRIVER_NAME)' '-DSCRCODE=$(DRIVER_CODE)' '-DSCRCOMMENT="$(DRIVER_COMMENT)"'
SCR_CFLAGS = $(LIBCFLAGS) $(SCR_DEFS)
SCR_CXXFLAGS = $(LIBCXXFLAGS) $(SCR_DEFS)
SCR_SO_NAME = $(MOD_NAME)x
SCR_NAME = $(SCR_SO_NAME)$(DRIVER_CODE).$(MOD_EXT)
SCR_FILE = $(BLD_TOP)$(DRV_DIR)/$(SCR_NAME)
$(SCR_FILE): screen.$O
	$(INSTALL_DIRECTORY) $(@D)
	$(MKMOD) $(@) screen.$O $(SCR_OBJS)
screen-driver: $(SCR_FILE)

install::

uninstall::

clean::
	-rm -f $(BLD_TOP)$(DRV_DIR)/$(SCR_SO_NAME)$(DRIVER_CODE).*

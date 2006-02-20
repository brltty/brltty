###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2006 by The BRLTTY Team. All rights reserved.
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

SPK_DEFS ='-DSPKNAME=$(DRIVER_NAME)' '-DSPKCODE=$(DRIVER_CODE)' '-DSPKCOMMENT="$(DRIVER_COMMENT)"' '-DSPKVERSION="$(DRIVER_VERSION)"' '-DSPKCOPYRIGHT="$(DRIVER_COPYRIGHT)"'
SPK_CFLAGS = $(LIBCFLAGS) $(SPK_DEFS)
SPK_CXXFLAGS = $(LIBCXXFLAGS) $(SPK_DEFS)
SPK_MOD_NAME = $(BLD_TOP)$(DRV_DIR)/$(MOD_NAME)s$(DRIVER_CODE)
SPK_MOD_FILE = $(SPK_MOD_NAME).$(MOD_EXT)
$(SPK_MOD_FILE): speech.$O
	$(INSTALL_DIRECTORY) $(@D)
	$(MKMOD) $(@) speech.$O $(SPK_OBJS)
speech-driver: $(SPK_MOD_FILE)

install::

uninstall::

clean::
	-rm -f $(SPK_MOD_NAME).*

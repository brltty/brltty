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

SPKNAMES = $(BLD_TOP)$(DRV_DIR)/brltty-spk.lst
SPK_DEFS ='-DSPKNAME=$(DRIVER_NAME)' '-DSPKCODE=$(DRIVER_CODE)'
SPK_CFLAGS = $(LIBCFLAGS) $(SPK_DEFS)
SPK_CXXFLAGS = $(LIBCXXFLAGS) $(SPK_DEFS)
SPK_SO_NAME = $(MOD_NAME)s
SPK_NAME = $(SPK_SO_NAME)$(DRIVER_CODE).$(MOD_EXT)
SPK_FILE = $(BLD_TOP)$(DRV_DIR)/$(SPK_NAME)
$(SPK_FILE): speech.$O
	$(INSTALL_DIRECTORY) $(@D)
	$(MKMOD) $(@) speech.$O $(SPK_OBJS)
speech-driver: $(SPK_FILE)

spk-lib-name:
	echo "$(DRIVER_CODE)  $(DRIVER_NAME) [$(SPEECH_MODELS)]" >>$(SPKNAMES)

install::

uninstall::

clean::
	-rm -f $(BLD_TOP)$(DRV_DIR)/$(SPK_SO_NAME)$(DRIVER_CODE).*

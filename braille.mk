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

DRIVER_TYPE = braille
DRIVER_LETTER = b
include $(SRC_TOP)driver.mk

install-api::
	$(INSTALL_DIRECTORY) $(INSTALL_ROOT)$(INCLUDE_DIRECTORY)
	for file in $(SRC_DIR)/*-$(DRIVER_CODE).h; do test -f $$file && $(INSTALL_DATA) $$file $(INSTALL_ROOT)$(INCLUDE_DIRECTORY); done || :

install:: $(INSTALL_API)

uninstall::
	rm -f $(INSTALL_ROOT)$(INCLUDE_DIRECTORY)/*-$(DRIVER_CODE).h


###############################################################################
# BRLTTY - A background process providing access to the Linux console (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

default: all

include $(SRC_DIR)/reldeps.mk
include $(SRC_TOP)absdeps.mk

$(BLD_TOP)Drivers/VarioHT/braille.$O: $(BLD_TOP)Drivers/VarioHT/vario.$O
$(BLD_TOP)Drivers/VarioHT/braille.$O: $(BLD_TOP)Drivers/VarioHT/variolow.$O
	cd $(@D) && $(MAKE) $(@F)

$(SRC_TOP)Drivers/VideoBraille/braille.$O: $(BLD_TOP)Drivers/VideoBraille/vb.$O
$(BLD_TOP)Drivers/VideoBraille/braille.$O: $(BLD_TOP)Drivers/VideoBraille/vblow.$O
	cd $(@D) && $(MAKE) $(@F)

clean::
	-rm -f *.$O *.auto.h core

distclean::
	-rm -f *~ *orig \#*\# *.rej ? a.out
	-rm -f Makefile


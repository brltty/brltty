###############################################################################
# BRLTTY - A background process providing access to the Linux console (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

include $(SRC_TOP)absdeps.mk
include $(SRC_DIR)/reldeps.mk

$(BLD_TOP)Drivers/VarioHT/braille.o: $(BLD_TOP)Drivers/VarioHT/vario.o
$(BLD_TOP)Drivers/VarioHT/braille.o: $(BLD_TOP)Drivers/VarioHT/variolow.o
	cd $(@D) && $(MAKE) $(@F)

$(SRC_TOP)Drivers/VideoBraille/braille.o: $(BLD_TOP)Drivers/VideoBraille/vb.o
$(BLD_TOP)Drivers/VideoBraille/braille.o: $(BLD_TOP)Drivers/VideoBraille/vblow.o
	cd $(@D) && $(MAKE) $(@F)

$(BLD_TOP)Drivers/Voyager/braille.o: $(BLD_TOP)Drivers/Voyager/brlvger.auto.h
$(BLD_TOP)Drivers/Voyager/brlvger.auto.h:
	cd $(@D) && $(MAKE) $(@F)

clean::
	-rm -f *.o *.auto.h core

distclean::
	-rm -f *~ *orig \#*\# *.rej ? a.out
	-rm -f Makefile


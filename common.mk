###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2006 by The BRLTTY Developers.
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

$(BLD_TOP)$(BRL_DIR)/VarioHT/braille.$O: $(BLD_TOP)$(BRL_DIR)/VarioHT/vario.$O
$(BLD_TOP)$(BRL_DIR)/VarioHT/braille.$O: $(BLD_TOP)$(BRL_DIR)/VarioHT/variolow.$O
	cd $(@D) && $(MAKE) $(@F)

$(BLD_TOP)$(BRL_DIR)/VideoBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/VideoBraille/vb.$O
$(BLD_TOP)$(BRL_DIR)/VideoBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/VideoBraille/vblow.$O
	cd $(@D) && $(MAKE) $(@F)

clean::
	-rm -f *.$O *.auto.h core implib.a

distclean::
	-rm -f *~ *orig \#*\# *.rej ? a.out
	-rm -f Makefile

.DELETE_ON_ERROR:


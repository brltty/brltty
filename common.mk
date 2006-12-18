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

brlapi:
	cd $(BLD_TOP)$(PGM_DIR) && $(MAKE) api

$(BLD_TOP)$(BRL_DIR)/VideoBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/VideoBraille/vb.$O
$(BLD_TOP)$(BRL_DIR)/VideoBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/VideoBraille/vblow.$O
	cd $(@D) && $(MAKE) $(@F)

INSTALL_PROGRAM_DIRECTORY = $(INSTALL_ROOT)$(PROGRAM_DIRECTORY)
install-program-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_PROGRAM_DIRECTORY)

INSTALL_LIBRARY_DIRECTORY = $(INSTALL_ROOT)$(LIBRARY_DIRECTORY)
install-library-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_LIBRARY_DIRECTORY)

INSTALL_DATA_DIRECTORY = $(INSTALL_ROOT)$(DATA_DIRECTORY)
install-data-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_DATA_DIRECTORY)

INSTALL_MAN1_DIRECTORY = $(INSTALL_ROOT)$(MANPAGE_DIRECTORY)/man1
install-man1-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_MAN1_DIRECTORY)

INSTALL_MAN3_DIRECTORY = $(INSTALL_ROOT)$(MANPAGE_DIRECTORY)/man3
install-man3-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_MAN3_DIRECTORY)

INSTALL_INCLUDE_DIRECTORY = $(INSTALL_ROOT)$(INCLUDE_DIRECTORY)
install-include-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_INCLUDE_DIRECTORY)

INSTALL_APILIB_DIRECTORY = $(INSTALL_ROOT)$(libdir)
install-apilib-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_APILIB_DIRECTORY)

INSTALL_APIHDR_DIRECTORY = $(INSTALL_ROOT)$(includedir)
install-apihdr-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_APIHDR_DIRECTORY)

clean::
	-rm -f *.$O *.auto.h *.auto.c core implib.a

distclean::
	-rm -f *~ *orig \#*\# *.rej ? a.out
	-rm -f Makefile

.DELETE_ON_ERROR:


###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2009 by The BRLTTY Developers.
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

FORCE:

brlapi:
	cd $(BLD_TOP)$(PGM_DIR) && $(MAKE) api

$(BLD_TOP)$(BRL_DIR)/EuroBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/EuroBraille/eu_braille.$O
$(BLD_TOP)$(BRL_DIR)/EuroBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/EuroBraille/eu_io.$O
$(BLD_TOP)$(BRL_DIR)/EuroBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/EuroBraille/eu_serial.$O
$(BLD_TOP)$(BRL_DIR)/EuroBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/EuroBraille/eu_usb.$O
$(BLD_TOP)$(BRL_DIR)/EuroBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/EuroBraille/eu_bluetooth.$O
$(BLD_TOP)$(BRL_DIR)/EuroBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/EuroBraille/eu_net.$O
$(BLD_TOP)$(BRL_DIR)/EuroBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/EuroBraille/eu_protocol.$O
$(BLD_TOP)$(BRL_DIR)/EuroBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/EuroBraille/eu_clio.$O
$(BLD_TOP)$(BRL_DIR)/EuroBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/EuroBraille/eu_esysiris.$O
	cd $(@D) && $(MAKE) $(@F)

$(BLD_TOP)$(BRL_DIR)/VideoBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/VideoBraille/vb.$O
$(BLD_TOP)$(BRL_DIR)/VideoBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/VideoBraille/vblow.$O
	cd $(@D) && $(MAKE) $(@F)

INSTALL_PROGRAM_DIRECTORY = $(INSTALL_ROOT)$(PROGRAM_DIRECTORY)
install-program-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_PROGRAM_DIRECTORY)

INSTALL_WRITABLE_DIRECTORY = $(INSTALL_ROOT)$(WRITABLE_DIRECTORY)
install-writable-directory:
	test -z "${WRITABLE_DIRECTORY}" || $(INSTALL_DIRECTORY) $(INSTALL_WRITABLE_DIRECTORY)

INSTALL_LIBRARY_DIRECTORY = $(INSTALL_ROOT)$(LIBRARY_DIRECTORY)
install-library-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_LIBRARY_DIRECTORY)

INSTALL_DATA_DIRECTORY = $(INSTALL_ROOT)$(DATA_DIRECTORY)
install-data-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_DATA_DIRECTORY)

INSTALL_MAN_DIRECTORY = $(INSTALL_ROOT)$(MANPAGE_DIRECTORY)

INSTALL_MAN1_DIRECTORY = $(INSTALL_MAN_DIRECTORY)/man1
install-man1-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_MAN1_DIRECTORY)

INSTALL_MAN3_DIRECTORY = $(INSTALL_MAN_DIRECTORY)/man3
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

INSTALL_APISOC_DIRECTORY = $(INSTALL_ROOT)$(API_SOCKET_DIRECTORY)
install-apisoc-directory:
	-$(INSTALL_DIRECTORY) -m 1777 $(INSTALL_APISOC_DIRECTORY)

clean::
	-rm -f *.$O *.auto.h *.auto.c core implib.a

distclean::
	-rm -f *~ *orig \#*\# *.rej ? a.out
	-rm -f Makefile

.DELETE_ON_ERROR:


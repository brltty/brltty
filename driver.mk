###############################################################################
# BRLTTY - A background process providing access to the Linux console (when in
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

TXT2HLP = $(BLD_TOP)$(PGM_DIR)/txt2hlp$X
$(TXT2HLP)$X:
	cd $(@D) && $(MAKE) $(@F)

HELP_NAME = brltty-$(DRIVER_CODE).hlp
HELP_FILE = $(BLD_TOP)$(DAT_DIR)/$(HELP_NAME)
HELP_TEXT = $(SRC_DIR)/help*.txt
$(HELP_FILE): $(HELP_DEPS) $(TXT2HLP)
	$(INSTALL_DIRECTORY) $(@D)
	$(TXT2HLP) $(HELP_FILE) $(HELP_TEXT)
braille-help:: $(HELP_FILE)

BRLNAMES = $(BLD_TOP)$(DRV_DIR)/brltty-brl.lst
BRL_DEFS = '-DBRLNAME=$(DRIVER_NAME)' '-DBRLCODE=$(DRIVER_CODE)' '-DBRLHELP="$(HELP_NAME)"'
BRL_CFLAGS = $(LIBCFLAGS) $(BRL_DEFS)
BRL_CXXFLAGS = $(LIBCXXFLAGS) $(BRL_DEFS)
BRL_SO_NAME = $(LIB_NAME)b
BRL_NAME = $(BRL_SO_NAME)$(DRIVER_CODE).$(LIB_EXT)
BRL_FILE = $(BLD_TOP)$(DRV_DIR)/$(BRL_NAME)
$(BRL_FILE): braille.$O
	$(INSTALL_DIRECTORY) $(@D)
	$(MKLIB:<soname>=${BRL_NAME}) $(@) braille.$O $(BRL_OBJS)
braille-driver: $(BRL_FILE)

brl-lib-name:
	echo "$(DRIVER_CODE)  $(DRIVER_NAME) [$(BRAILLE_MODELS)]" >>$(BRLNAMES)

SPKNAMES = $(BLD_TOP)$(DRV_DIR)/brltty-spk.lst
SPK_DEFS ='-DSPKNAME=$(DRIVER_NAME)' '-DSPKCODE=$(DRIVER_CODE)'
SPK_CFLAGS = $(LIBCFLAGS) $(SPK_DEFS)
SPK_CXXFLAGS = $(LIBCXXFLAGS) $(SPK_DEFS)
SPK_SO_NAME = $(LIB_NAME)s
SPK_NAME = $(SPK_SO_NAME)$(DRIVER_CODE).$(LIB_EXT)
SPK_FILE = $(BLD_TOP)$(DRV_DIR)/$(SPK_NAME)
$(SPK_FILE): speech.$O
	$(INSTALL_DIRECTORY) $(@D)
	$(MKLIB:<soname>=${SPK_NAME}) $(@) speech.$O $(SPK_OBJS)
speech-driver: $(SPK_FILE)

spk-lib-name:
	echo "$(DRIVER_CODE)  $(DRIVER_NAME) [$(SPEECH_MODELS)]" >>$(SPKNAMES)

install-api:
	$(INSTALL_DIRECTORY) $(INSTALL_ROOT)$(INCLUDE_DIRECTORY)
	for file in *-$(DRIVER_CODE).h; do test -f $$file && $(INSTALL_DATA) $$file $(INSTALL_ROOT)$(INCLUDE_DIRECTORY); done || :

install:: $(INSTALL_API)

uninstall::
	rm -f $(INSTALL_ROOT)$(INCLUDE_DIRECTORY)/*-$(DRIVER_CODE).h

clean::
	-rm -f $(BLD_TOP)$(DRV_DIR)/$(LIB_NAME)?$(DRIVER_CODE).*
	-rm -f $(BLD_TOP)$(DAT_DIR)/brltty-$(DRIVER_CODE)[-.]*

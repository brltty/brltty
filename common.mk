###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2018 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any
# later version. Please see the file LICENSE-LGPL for details.
#
# Web Page: http://brltty.com/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

default: all

FORCE:

include $(BLD_TOP)config.mk
include $(BLD_TOP)forbuild.mk

include $(SRC_TOP)absdeps.mk
include $(SRC_DIR)/reldeps.mk

%.$B: $(SRC_DIR)/%.c
	$(CC_FOR_BUILD) -DFOR_BUILD $(CFLAGS_FOR_BUILD) -o $@ -c $<

API_NAME = brlapi
API_ARC = $(ARC_PFX)$(API_NAME).$(ARC_EXT)
API_LIB = $(LIB_PFX)$(API_NAME).$(LIB_EXT)
API_LIB_VERSIONED = $(API_LIB).$(API_VERSION)

API_DLL = $(LIB_PFX)$(API_NAME)-$(API_VERSION).$(LIB_EXT)
API_IMPLIB = $(ARC_PFX)$(API_NAME).$(LIB_EXT).$(ARC_EXT)
API_IMPLIB_VERSIONED = $(ARC_PFX)$(API_NAME)-$(API_RELEASE).$(LIB_EXT).$(ARC_EXT)
API_DEF = $(API_NAME).def

COMMANDS_OPTIONS = -f $(SRC_TOP)$(PGM_DIR)/brl_cmds.awk
COMMANDS_SCRIPTS = $(COMMANDS_OPTIONS:-f=)
COMMANDS_SOURCES = $(SRC_TOP)$(HDR_DIR)/brl_cmds.h $(SRC_TOP)$(HDR_DIR)/brl_custom.h
COMMANDS_DEPENDENCIES = $(COMMANDS_SCRIPTS) $(COMMANDS_SOURCES)
COMMANDS_ARGUMENTS = $(COMMANDS_OPTIONS) $(COMMANDS_SOURCES)

brlapi:
	cd $(BLD_TOP)$(PGM_DIR) && $(MAKE) api

$(BLD_TOP)$(BRL_DIR)/EuroBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/EuroBraille/eu_braille.$O
$(BLD_TOP)$(BRL_DIR)/EuroBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/EuroBraille/eu_clio.$O
$(BLD_TOP)$(BRL_DIR)/EuroBraille/braille.$O: $(BLD_TOP)$(BRL_DIR)/EuroBraille/eu_esysiris.$O
	cd $(@D) && $(MAKE) $(@F)

INSTALL_PROGRAM_DIRECTORY = $(INSTALL_ROOT)$(PROGRAM_DIRECTORY)
install-program-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_PROGRAM_DIRECTORY)

INSTALL_WRITABLE_DIRECTORY = $(INSTALL_ROOT)$(WRITABLE_DIRECTORY)
install-writable-directory:
	test -z "${WRITABLE_DIRECTORY}" || $(INSTALL_DIRECTORY) $(INSTALL_WRITABLE_DIRECTORY)

INSTALL_DRIVERS_DIRECTORY = $(INSTALL_ROOT)$(DRIVERS_DIRECTORY)
install-drivers-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_DRIVERS_DIRECTORY)

INSTALL_TABLES_DIRECTORY = $(INSTALL_ROOT)$(TABLES_DIRECTORY)
install-tables-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_TABLES_DIRECTORY)

INSTALL_TEXT_TABLES_DIRECTORY = $(INSTALL_TABLES_DIRECTORY)/$(TEXT_TABLES_SUBDIRECTORY)
install-text-tables-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_TEXT_TABLES_DIRECTORY)

INSTALL_ATTRIBUTES_TABLES_DIRECTORY = $(INSTALL_TABLES_DIRECTORY)/$(ATTRIBUTES_TABLES_SUBDIRECTORY)
install-attributes-tables-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_ATTRIBUTES_TABLES_DIRECTORY)

INSTALL_CONTRACTION_TABLES_DIRECTORY = $(INSTALL_TABLES_DIRECTORY)/$(CONTRACTION_TABLES_SUBDIRECTORY)
install-contraction-tables-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_CONTRACTION_TABLES_DIRECTORY)

INSTALL_KEYBOARD_TABLES_DIRECTORY = $(INSTALL_TABLES_DIRECTORY)/$(KEYBOARD_TABLES_SUBDIRECTORY)
install-keyboard-tables-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_KEYBOARD_TABLES_DIRECTORY)

INSTALL_INPUT_TABLES_DIRECTORY = $(INSTALL_TABLES_DIRECTORY)/$(INPUT_TABLES_SUBDIRECTORY)
install-input-tables-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_INPUT_TABLES_DIRECTORY)

INSTALL_LOCALE_DIRECTORY = $(INSTALL_ROOT)$(localedir)
install-locale-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_LOCALE_DIRECTORY)

INSTALL_MAN_DIRECTORY = $(INSTALL_ROOT)$(MANPAGE_DIRECTORY)

INSTALL_MAN1_DIRECTORY = $(INSTALL_MAN_DIRECTORY)/man1
install-man1-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_MAN1_DIRECTORY)

INSTALL_MAN3_DIRECTORY = $(INSTALL_MAN_DIRECTORY)/man3
install-man3-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_MAN3_DIRECTORY)

INSTALL_DOCUMENT_DIRECTORY = $(INSTALL_ROOT)$(docdir)
install-document-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_DOCUMENT_DIRECTORY)

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

INSTALL_APIPOL_DIRECTORY = $(INSTALL_ROOT)$(datadir)/polkit-1/actions
install-apipol-directory:
	-$(INSTALL_DIRECTORY) $(INSTALL_APIPOL_DIRECTORY)

INSTALL_X11_AUTOSTART_DIRECTORY = $(INSTALL_ROOT)$(sysconfdir)/X11/Xsession.d
install-x11-autostart-directory:
	-$(INSTALL_DIRECTORY) $(INSTALL_X11_AUTOSTART_DIRECTORY)

INSTALL_GDM_AUTOSTART_DIRECTORY = $(INSTALL_ROOT)$(datadir)/gdm/greeter/autostart
install-gdm-autostart-directory:
	$(INSTALL_DIRECTORY) $(INSTALL_GDM_AUTOSTART_DIRECTORY)

clean::
	-rm -f *.$O *.auto.h *.auto.c core implib.a

distclean::
	-rm -f *~ *orig \#*\# *.rej ? a.out
	-rm -f Makefile

.DELETE_ON_ERROR:


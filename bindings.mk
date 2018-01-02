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

API_NAME = brlapi
API_DIR = $(BLD_TOP)$(PGM_DIR)
API_LIB = $(API_DIR)/$(LIB_PFX)$(API_NAME).$(LIB_EXT)
API_LDFLAGS = -L$(API_DIR) -l$(API_NAME)

API_HDR = $(API_DIR)/$(API_NAME).h
API_AWK = $(SRC_TOP)$(PGM_DIR)/$(API_NAME).awk

API_KEYCODES = $(SRC_TOP)$(PGM_DIR)/$(API_NAME)_keycodes.h
API_CONSTANTS = $(API_DIR)/$(API_NAME)_constants.h
API_HDRS = $(API_HDR) $(API_KEYCODES) $(API_CONSTANTS)

CONSTANTS_OPTIONS = -f $(API_AWK) -f $(SRC_DIR)/constants.awk
CONSTANTS_SCRIPTS = $(CONSTANTS_OPTIONS:-f=)
CONSTANTS_SOURCES = $(API_HDR) $(API_KEYCODES)
CONSTANTS_DEPENDENCIES = $(CONSTANTS_SCRIPTS) $(CONSTANTS_SOURCES) $(COMMANDS_DEPENDENCIES)
CONSTANTS_ARGUMENTS = $(CONSTANTS_OPTIONS) $(COMMANDS_OPTIONS) $(CONSTANTS_SOURCES) $(COMMANDS_SOURCES)


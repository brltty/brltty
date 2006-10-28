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

BRLDEFS_HDR = $(SRC_TOP)$(PGM_DIR)/brldefs.h
BRLDEFS_AWK = $(SRC_TOP)$(PGM_DIR)/brldefs.awk
APIDEFS_AWK = $(SRC_TOP)$(PGM_DIR)/apidefs.awk

API_DIR = $(BLD_TOP)$(PGM_DIR)
API_LDFLAGS = -L$(API_DIR) -lbrlapi

API_HEADER = $(API_DIR)/api.h
API_constants = $(API_DIR)/api_constants.h
API_HEADERS = $(API_HEADER) $(API_constants)


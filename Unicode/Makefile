###############################################################################
# BRLTTY - A background process providing access to the Linux console (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

PROGRAMS = trcp
all: $(PROGRAMS)

include setup.mk

TRCP_OBJECTS = trcp.o unicode.o $(PGM_DIR)/options.o $(PGM_DIR)/misc.o
trcp: $(TRCP_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(TRCP_OBJECTS) $(LDLIBS)
trcp.o:
	$(CC) $(CFLAGS) -c trcp.c
unicode.o:
	$(CC) $(CFLAGS) -c unicode.c

clean::
	rm -f $(PROGRAMS)


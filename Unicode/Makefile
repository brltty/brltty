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


CC = gcc
CFLAGS = -O2 -Wall
LD = $(CC)
LDFLAGS = -s


TOOLS = trcp

all: $(TOOLS)

trcp: trcp.o unicode.o

trcp.o: trcp.c unicode.h

unicode.o: unicode.c unicode.h unicode-table.h $(wildcard iso-*.h) $(wildcard cp-*.h)

clean:
	rm -f *.o *~

DISTCLEAN: clean
	rm -f $(TOOLS)


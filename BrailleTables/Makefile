###############################################################################
# BRLTTY - A background process providing access to the Linux console (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

# This Makefile produces some tools to manipulate braille definition tables
# used by BRLTTY.
###############################################################################


TOOLS = tbl2txt txt2tbl convtable ibm437toiso ibm850toiso

CC = gcc
CFLAGS = -O2 -Wall
LD = $(CC)
LDFLAGS = -s


all: $(TOOLS)

tbl2txt: tbl2txt.c

txt2tbl: txt2tbl.c

convtable: convtable.c

ibm437toiso: ibm437toiso.c

ibm850toiso: ibm850toiso.c

clean:
	rm -f *.o *~

distclean: clean
	rm -f $(TOOLS)


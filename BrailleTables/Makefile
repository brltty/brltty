###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-2001 by The BRLTTY Team, All rights reserved.
#
# Web Page: http://www.cam.org/~nico/brltty
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# This software is maintained by Nicolas Pitre <nico@cam.org>.
#
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


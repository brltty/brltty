###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
#
# Nicolas Pitre <nico@cam.org>
# Stephane Doyon <s.doyon@videotron.ca>
# Nikhil Nair <nn201@cus.cam.ac.uk>
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

TOOLS = tbl2txt txt2tbl convtable ibm437toiso

CC = gcc
CFLAGS = -O2 -g -Wall -D_POSIX_C_SOURCE=2
LD = $(CC)
LDFLAGS = -s


all: $(TOOLS)

tbl2txt: tbl2txt.o
	$(LD) $(LDFLAGS) -o $@ tbl2txt.o $(LDLIBS)

tbl2txt.o: tbl2txt.c
	$(CC) $(CFLAGS) -c tbl2txt.c

txt2tbl: txt2tbl.o
	$(LD) $(LDFLAGS) -o $@ txt2tbl.o $(LDLIBS)

txt2tbl.o: txt2tbl.c
	$(CC) $(CFLAGS) -c txt2tbl.c

convtable: convtable.o
	$(LD) $(LDFLAGS) -o $@ convtable.o $(LDLIBS)

convtable.o: convtable.c
	$(CC) $(CFLAGS) -c convtable.c

ibm437toiso: ibm437toiso.o
	$(LD) $(LDFLAGS) -o $@ ibm437toiso.o $(LDLIBS)

ibm437toiso.o: ibm437toiso.c
	$(CC) $(CFLAGS) -c ibm437toiso.c

clean:
	rm -f *.o *~

distclean: clean
	rm -f $(TOOLS)


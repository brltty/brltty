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

###############################################################################
# This Makefile produces some tools to manipulate braille definition tables
# used by BRLTTY.
###############################################################################

PROGRAMS = txt2tbl tbl2txt tbl2tbl tbl2jbt isotoibm437 ibm437toiso ibm850toiso
all: $(PROGRAMS)

include setup.mk

TXT2TBL_OBJECTS = txt2tbl.o $(PGM_DIR)/options.o $(PGM_DIR)/misc.o
txt2tbl: $(TXT2TBL_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(TXT2TBL_OBJECTS) $(LDLIBS)
txt2tbl.o:
	$(CC) $(CFLAGS) -c txt2tbl.c

TBL2TXT_OBJECTS = tbl2txt.o $(PGM_DIR)/options.o $(PGM_DIR)/misc.o $(UCD_DIR)/unicode.o
tbl2txt: $(TBL2TXT_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(TBL2TXT_OBJECTS) $(LDLIBS)
tbl2txt.o:
	$(CC) $(CFLAGS) -c tbl2txt.c

TBL2TBL_OBJECTS = tbl2tbl.o $(PGM_DIR)/options.o $(PGM_DIR)/misc.o
tbl2tbl: $(TBL2TBL_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(TBL2TBL_OBJECTS) $(LDLIBS)
tbl2tbl.o:
	$(CC) $(CFLAGS) -c tbl2tbl.c

TBL2JBT_OBJECTS = tbl2jbt.o $(PGM_DIR)/options.o $(PGM_DIR)/misc.o
tbl2jbt: $(TBL2JBT_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(TBL2JBT_OBJECTS) $(LDLIBS)
tbl2jbt.o:
	$(CC) $(CFLAGS) -c tbl2jbt.c

ISOTOIBM437_OBJECTS = isotoibm437.o
isotoibm437: $(ISOTOIBM437_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(ISOTOIBM437_OBJECTS) $(LDLIBS)
isotoibm437.o:
	$(CC) $(CFLAGS) -c isotoibm437.c

IBM437TOISO_OBJECTS = ibm437toiso.o
ibm437toiso: $(IBM437TOISO_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(IBM437TOISO_OBJECTS) $(LDLIBS)
ibm437toiso.o:
	$(CC) $(CFLAGS) -c ibm437toiso.c

IBM850TOISO_OBJECTS = ibm850toiso.o
ibm850toiso: $(IBM850TOISO_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(IBM850TOISO_OBJECTS) $(LDLIBS)
ibm850toiso.o:
	$(CC) $(CFLAGS) -c ibm850toiso.c

clean::
	-rm -f $(PROGRAMS)


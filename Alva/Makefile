###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-1999 by The BRLTTY Team, All rights reserved.
#
# Nicolas Pitre <nico@cam.org>
# Stéphane Doyon <s.doyon@videotron.ca>
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

###############################################################################
#
# Makefile for BRLTTY Alva driver
# All variables are defined in the main Makefile...
#
###############################################################################


# Uncomment next line if you want to use the parallel port interface
# and you configured USE_PARALLEL_PORT to 1 in brlconf.h
# Comment it out if you are compiling on a non-i386 architecture.
# Note that this option uses an extra library.  See README for details.
ALVALIB = alva_api.library/alva_api.a
#ALVALIB =

HELP = al
HELPNAME = brltty-$(HELP).hlp

help:
	../txt2hlp $(HELPNAME) brlttyh*.txt

LD = ld

brlmain.o: brlmain.cc brlconf.h ../brl.h ../misc.h ../scr.h ../config.h
	$(COMPCPP) $(CFLAGS) '-DBRLDEV="$(BRLDEV)"' \
	'-DHELPNAME="$(PREFIX)$(DATA_DIR)/$(HELPNAME)"'\
	'-DUSE_PARALLEL_PORT=$(UPP)' -c brlmain.cc

brl.o: brlmain.o $(ALVALIB)
	$(LD) -r -o brl.o brlmain.o $(ALVALIB)

speech.o: speech.c ../speech.h brlconf.h speech.h
	$(CC) $(CFLAGS) -c speech.c

# shared library
# name of library - serial (as) and parallel (ap)
#LIB_DESC = al
LIB_NAME = $(LIB_SO_NAME)$(LIB_DESC).so.$(LIB_VER)

lib: brlmain.cc brlconf.h ../brl.h ../misc.h ../scr.h ../config.h
	rm -f brl.o brlmain.o
	$(MAKE) lib1 LIB_DESC=as UPP=0
	rm -f brl.o brlmain.o
	$(MAKE) lib1 LIB_DESC=ap UPP=1
	touch lib

lib1: brl.o
	$(CC) -g -shared -Wl,-soname,$(LIB_NAME) -o $(LIB_NAME) brl.o -lc


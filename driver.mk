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
# Sub-makefile included by makefile for each BRLTTY device driver.
# It defines the "help" and "lib" targets.
# All variables are defined in the main Makefile...
#
###############################################################################

HELPNAME = brltty-$(DRIVER).hlp
HELPDIR = ../help
HELPFILE = $(HELPDIR)/$(HELPNAME)
DRIVER_CFLAGS = $(LIB_CFLAGS) '-DHELPNAME="$(PREFIX)$(DATA_DIR)/$(HELPNAME)"'

$(HELPFILE): brlttyh*.txt
	../txt2hlp $(HELPFILE) brlttyh*.txt

help: $(HELPFILE)

LIBNAME = $(LIB_SO_NAME)$(DRIVER).so.$(LIB_VER)
LIBDIR = ../lib
LIBFILE = $(LIBDIR)/$(LIBNAME)

$(LIBFILE): brl.o
	$(CC) -shared -Wl,-soname,$(LIBNAME) -o $(LIBFILE) brl.o -lc

ifeq ($(LIB_TARGETS),)
   LIB_TARGETS = $(LIBFILE)
endif
lib: $(LIB_TARGETS)


###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
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

###############################################################################
#
# Sub-makefile included by makefile for each BRLTTY device driver.
# It defines the "help" and "lib" targets.
# All variables are defined in the main Makefile...
#
###############################################################################

LD = ld

HELPDIR = ../help
LIBDIR = ../lib
LIBNAMES = $(LIBDIR)/brl_drivers.lst

HELPNAME = brltty-$(DRIVER).hlp
HELPFILE = $(HELPDIR)/$(HELPNAME)
$(HELPFILE): brlttyh*.txt
	../txt2hlp $(HELPFILE) brlttyh*.txt

braille-help: $(HELPFILE)

BRL_CFLAGS = $(LIB_CFLAGS) '-DHELPNAME="$(PREFIX)$(DATA_DIR)/$(HELPNAME)"'
BRL_LFLAGS = -shared
BRL_SO_NAME = $(LIB_SO_NAME)b
BRL_NAME = $(BRL_SO_NAME)$(DRIVER).so.$(LIB_VER)
BRL_FILE = $(LIBDIR)/$(BRL_NAME)
$(BRL_FILE): brl.o
	$(LD) $(BRL_LFLAGS) -soname $(BRL_NAME) -o $(BRL_FILE) brl.o -lc

ifeq ($(BRL_FILES),)
   BRL_FILES = $(BRL_FILE)
endif
braille-driver: $(BRL_FILES)

SPK_CFLAGS = $(LIB_CFLAGS)
SPK_LFLAGS = -shared
SPK_SO_NAME = $(LIB_SO_NAME)s
SPK_NAME = $(SPK_SO_NAME)$(DRIVER).so.$(LIB_VER)
SPK_FILE = $(LIBDIR)/$(SPK_NAME)
$(SPK_FILE): speech.o
	$(LD) $(SPK_LFLAGS) -soname $(SPK_NAME) -o $(SPK_FILE) speech.o -lc

ifeq ($(SPK_FILES),)
   SPK_FILES = $(SPK_FILE)
endif
speech-driver: $(SPK_FILES)


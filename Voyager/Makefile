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

###############################################################################
#
# Makefile for BRLTTY Voyager driver
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS, and LDLIBS
###############################################################################

DRIVER_CODE = vo
DRIVER_NAME = Voyager
BRAILLE_MODELS = 44/70 (USB)
include ../driver.mk

MINOR := $(shell grep VOYAGER_DEFAULT_MINOR kernel/voyager.h |cut -f3 -d' ')
#MINOR=144
VOYAGERDEV=/dev/voyager

brl.o: brl.c brlconf.h kernel/voyager.h ../brl.h ../misc.h \
	../scr.h ../brl_driver.h
	$(CC) $(BRL_CFLAGS) -c brl.c

#Let's not rebuild the help unless the user wants to as it requires python.
remakehelp: brl.c brltty_helpmodel.txt mkhlp hlptrans.py
	./mkhlp

install-device:
	@if [ -c ${INSTALL_ROOT}${PREFIX}${VOYAGERDEV} ]; \
        then \
          echo "Voyager device ${INSTALL_ROOT}${PREFIX}${VOYAGERDEV}" \
             "already exists." ; \
        else \
          echo "Creating character device" \
            "${INSTALL_ROOT}${PREFIX}${VOYAGERDEV}" \
            "with major 180 and minor ${MINOR}" ; \
          mknod -m 0660 ${INSTALL_ROOT}${PREFIX}${VOYAGERDEV} c 180 ${MINOR}; \
        fi

distclean:
	$(MAKE) -C kernel distclean

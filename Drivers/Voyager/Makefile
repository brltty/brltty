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
# Makefile for BRLTTY Voyager driver
###############################################################################

DRIVER_CODE = vo
DRIVER_NAME = Voyager
BRAILLE_MODELS = 44/70 (USB)
HELP_DEPS = $(HELP_TEXT)

include setup.mk
include $(TOP_DIR)/driver.mk

KERNEL_INCLUDES = /usr/include/linux
ifneq (,$(wildcard $(KERNEL_INCLUDES)/brlvger.h))
# The USB driver is in the kernel.
VGERHDR = $(KERNEL_INCLUDES)/brlvger.h
else
# The USB driver is not in the kernel: we use the module in kernel/.
VGERHDR = kernel/linux/brlvger.h
endif

MINOR := $(shell grep BRLVGER_MINOR $(VGERHDR) | ${AWK} '{print $$3;}')
VOYAGERDEV=/dev/brlvger

.PHONY : brlvger.h
brlvger.auto.h:
	ln -sf $(VGERHDR) $@

braille.o: brlvger.auto.h
	$(CC) $(BRL_CFLAGS) $(KDRIVER) -c braille.c

#Let's not rebuild the help unless the user wants to as it requires python.
remakehelp: braille.c brltty_helpmodel.txt mkhlp hlptrans.py
	./mkhlp

install-device:
	@if [ -c ${INSTALL_ROOT}${VOYAGERDEV} ]; \
        then \
          echo "Voyager device ${INSTALL_ROOT}${VOYAGERDEV}" \
             "already exists." ; \
        else \
          echo "Creating character device" \
            "${INSTALL_ROOT}${VOYAGERDEV}" \
            "with major 180 and minor ${MINOR}" ; \
          mknod -m 0660 ${INSTALL_ROOT}${VOYAGERDEV} c 180 ${MINOR}; \
        fi

clean::
	-rm -f brlvger.auto.h

distclean::
	-cd kernel && $(MAKE) clean distclean

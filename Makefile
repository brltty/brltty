###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Version 1.9.0, 06 April 1998
#
# Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
#
# Nikhil Nair <nn201@cus.cam.ac.uk>
# Nicolas Pitre <nico@cam.org>
# Stephane Doyon <s.doyon@videotron.ca>
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


# Specify your Braille display by uncommenting one and ONLY one of these
# definitions of BRL_TARGET:
#BRL_TARGET = Alva_ABT3
#BRL_TARGET = CombiBraille
#BRL_TARGET = EuroBraille
#BRL_TARGET = TSI

# Specify your speech support option.
# Uncomment one of these lines and comment out the NoSpeech line 
# if you have speech.
# Note: All this is still experimental.  It is safe to leave it as NoSpeech.
SPK_TARGET = NoSpeech
#SPK_TARGET = CombiBraille
#SPK_TARGET = Generic_say
#SPK_TARGET = Televox

# Specify the default, compiled-in text translation table.  This can be
# overridden at run-time by giving brltty the -t option.
# See the content of the BrailleTable directory for available tables.
#TEXTTRANS = text.us.tbl
TEXTTRANS = nico.tbl

# Specify the device name for the serial port your Braille display will
# normally be connected to.  For port com(n), use /dev/ttyS(n-1) -
# e.g. for com1 use /dev/ttyS0
# [Note that this port can be overridden from the brltty command-line.
# See the documentation for further details.]
BRLDEV = /dev/ttyS0

# NOTE: if you intend to have brltty start up automatically at
# boot-time, the executable and all data files should be in your root
# filesystem.

# Specify the full path of the `brltty' executable:
EXEC_PATH = /sbin/brltty

# Specify the full path of the script used for reinstalling from a working
# system:
REINSTALL_PATH = /sbin/install-brltty

# Specify the directory where BRLTTY keeps its translation tables and its
# help file.  If this directory doesn't exist it will be created.
# (You may prefer somewhere under /usr/lib or /usr/local/lib ...)
DATA_DIR = /etc/brltty

# Edit this definition (options for install(1)) if you want to install
# setuid or setgid to allow ordinary users to start it.  For instance,
# to install setuid and executable by anyone, you could use permissions
# 4755 (rwsr-xr-x).
INSTALL_EXEC = --owner=root --group=root --mode=0744


# Screen access methods (OS dependent).  Uncomment only one of the
# two following sets of options:

# 1. This is the name of the character device BRLTTY uses to read the screen
# under Linux.
# It is created during installation if it does not already exist.
# Don't modify this unless you know what you're doing!
VCSADEV = /dev/vcsa0
SCR_O = scr_vcsa.o
INSKEY_O = inskey_lnx.o

# 2. You may define this as an alternative to Linux's vcsa device by using
# the "screen" package along with BRLTTY through shared memory.  It is 
# made available for any UNIX platform that supports IPC (like SVR4).
# See Patches/screen-x.x.x.txt for more information.
# NOTE: This is completely experimental.
#SCR_O = scr_shm.o
#INSKEY_O = *** still under progress ***


# The following are compiler settings.  If you don't know what they mean,
# LEAVE THEM ALONE!
CC = gcc
# To compile in a.out (if you use ELF by default), you may be able to use
# `-b i486-linuxaout'; however, you may also need to use the -V flag, or
# possibly a different gcc executable, depending on your setup.
CFLAGS = -O2 -g -Wall -D_POSIX_C_SOURCE=2 -D$(BRL_TARGET) -D$(SPK_TARGET)
UTILS_CFLAGS = -O2 -g -Wall -D_POSIX_C_SOURCE=2
LD = $(CC)
LDFLAGS = -s
#LDFLAGS = -s -static
LDLIBS =
PREFIX =

# ------------------------ DO NOT EDIT BELOW THIS LINE ------------------------


all:
ifndef BRL_TARGET
	@echo BRL_TARGET not defined in the Makefile
else
ifndef SPK_TARGET
	@echo SPK_TARGET not defined in the Makefile
else
	make do_it
endif
endif

do_it: brltty install-brltty txt2hlp brltest scrtest

install: brltty txt2hlp install-brltty
	install $(INSTALL_EXEC) --strip brltty $(PREFIX)$(EXEC_PATH) 
	install $(INSTALL_EXEC) install-brltty $(PREFIX)$(REINSTALL_PATH) 
	install --directory $(PREFIX)$(DATA_DIR) 
	install -m 644 BrailleTables/*.tbl $(PREFIX)$(DATA_DIR)
	find $(BRL_TARGET) -name '*.dat' -exec install -m 644 {} \
	  $(PREFIX)$(DATA_DIR) \;
	./txt2hlp $(PREFIX)$(DATA_DIR)/brlttydev.hlp \
	  $(BRL_TARGET)/brlttyh*.txt
	if [ "$(VCSADEV)" ]; \
	then \
	  if [ ! -c $(PREFIX)$(VCSADEV) ]; \
	  then \
	    mknod -m o= $(PREFIX)$(VCSADEV) c 7 128; \
	    chown root.tty $(PREFIX)$(VCSADEV); \
	    chmod 660 $(PREFIX)$(VCSADEV); \
	  fi; \
	fi

uninstall:
	rm -f $(PREFIX)$(EXEC_PATH) $(PREFIX)$(REINSTALL_PATH)
	rm -rf $(PREFIX)$(DATA_DIR)

install-brltty: install.template
	sed -e 's%=E%$(EXEC_PATH)%g' -e 's%=I%$(REINSTALL_PATH)%g' \
	  -e 's%=D%$(DATA_DIR)%g' -e 's%=V%$(VCSADEV)%g' install.template > $@

brltty: brltty.o brl.o speech.o scr.o scrdev.o $(SCR_O) $(INSKEY_O) \
	misc.o beeps.o cut-n-paste.o
	$(LD) $(LDFLAGS) -o $@ brltty.o $(BRL_TARGET)/brl.o \
	  $(SPK_TARGET)/speech.o $(INSKEY_O) \
	  scr.o scrdev.o $(SCR_O) \
	  misc.o beeps.o cut-n-paste.o $(LDLIBS)

brltest: brltest.o brl.o misc.o
	$(LD) $(LDFLAGS) -o $@ brltest.o $(BRL_TARGET)/brl.o misc.o $(LDLIBS)

scrtest: scrtest.o scr.o scrdev.o $(SCR_O)
	$(LD) $(LDFLAGS) -o $@ scrtest.o scr.o scrdev.o $(SCR_O) $(LDLIBS)

brl.o:
	cd $(BRL_TARGET); \
	if [ -f Makefile ]; \
	then \
	  $(MAKE) brl.o CC='$(CC)' CFLAGS='$(CFLAGS)' LD='$(LD)' \
	    LDFLAGS='$(LDFLAGS)' LDLIBS='$(LDLIBS)' BRLDEV='$(BRLDEV)'; \
	else \
	  $(MAKE) -f ../Driver.Makefile brl.o CC='$(CC)' CFLAGS='$(CFLAGS)' \
	    LD='$(LD)' LDFLAGS='$(LDFLAGS)' LDLIBS='$(LDLIBS)' \
	    BRLDEV='$(BRLDEV)'; \
	fi

speech.o:
	cd $(SPK_TARGET); \
	if [ -f Makefile ]; \
	then \
	  $(MAKE) speech.o CC='$(CC)' CFLAGS='$(CFLAGS)' LD='$(LD)' \
	    LDFLAGS='$(LDFLAGS)' LDLIBS='$(LDLIBS)' BRLDEV='$(BRLDEV)'; \
	else \
	  $(MAKE) -f ../Driver.Makefile speech.o CC='$(CC)' \
	    CFLAGS='$(CFLAGS)' LD='$(LD)' LDFLAGS='$(LDFLAGS)' \
	    LDLIBS='$(LDLIBS)' BRLDEV='$(BRLDEV)'; \
	fi

scr.o: scr.cc scr.h scrdev.h helphdr.h config.h
	$(CC) $(CFLAGS) -c scr.cc

scrdev.o: scrdev.cc scr.h scrdev.h helphdr.h config.h
	$(CC) $(CFLAGS) -c scrdev.cc

scr_vcsa.o: scr_vcsa.cc scr_vcsa.h scr.h scrdev.h config.h
	$(CC) $(CFLAGS) '-DVCSADEV="$(VCSADEV)"' -c scr_vcsa.cc

scr_shm.o: scr_shm.cc scr_shm.h scr.h scrdev.h config.h
	$(CC) $(CFLAGS) -c scr_shm.cc

inskey_lnx.o: inskey_lnx.c inskey.h
	$(CC) $(CFLAGS) -c inskey_lnx.c

misc.o: misc.c misc.h
	$(CC) $(CFLAGS) -c misc.c

brltty.o: brltty.c brl.h scr.h inskey.h misc.h config.h \
	  text.auto.h attrib.auto.h
	$(CC) $(CFLAGS) '-DHOME_DIR="$(DATA_DIR)"' -c brltty.c

beeps.o: beeps.c beeps.h beeps-songs.h
	$(CC) $(CFLAGS) -c beeps.c

cut-n-paste.o: cut-n-paste.c cut-n-paste.h beeps.h scr.h inskey.h
	$(CC) $(CFLAGS) -c cut-n-paste.c

scrtest.o: scrtest.c 
	$(CC) $(CFLAGS) -c scrtest.c

brltest.o: brltest.c brl.h config.h text.auto.h
	$(CC) $(CFLAGS) '-DHOME_DIR="$(DATA_DIR)"' -c brltest.c

text.auto.h: Makefile BrailleTables/$(TEXTTRANS) comptable
	./comptable < BrailleTables/$(TEXTTRANS) > text.auto.h

attrib.auto.h: Makefile BrailleTables/attrib.tbl comptable
	./comptable < BrailleTables/attrib.tbl > attrib.auto.h

txt2hlp: txt2hlp.o
	$(LD) $(LDFLAGS) -o $@ txt2hlp.o $(LDLIBS)

txt2hlp.o: txt2hlp.c helphdr.h
	$(CC) $(UTILS_CFLAGS) -c txt2hlp.c

comptable: comptable.o
	$(LD) $(LDFLAGS) -o $@ comptable.o $(LDLIBS)

comptable.o: comptable.c
	$(CC) $(UTILS_CFLAGS) -c comptable.c

clean:
	rm -f *.o */*.o install-brltty *.auto.h

distclean: clean
	rm -f brltty txt2hlp comptable *test
	rm -f *~ */*~ *orig */*orig \#*\# */\#*\#
	cd BrailleTables; make distclean


###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
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


# If you would like the driver for your Braille display built into the
# program, then specify it by uncommenting one, and only one, of these
# definitions of BRL_TARGET. If you do not specify your display here,
# then a set of shared libraries, one for each driver, will be made,
# and you will need to specify your display type at run-time via -m.
# Note to maintainer:
#    When adding a new target here, add it also to the "TARGETS =" line.
#BRL_TARGET = Alva
#BRL_TARGET = BrailleLite
#BRL_TARGET = CombiBraille
#BRL_TARGET = EcoBraille
#BRL_TARGET = EuroBraille
#BRL_TARGET = MDV
#BRL_TARGET = Papenmeier
#BRL_TARGET = TSI
#BRL_TARGET = Vario

# Specify your speech support option.
# Uncomment one of these lines and comment out the NoSpeech line 
# if you have speech.
# PS: Alva speech is available with Delphi models only.
# Note: All this is still experimental.  It is safe to leave it as NoSpeech.
SPK_TARGET = NoSpeech
#SPK_TARGET = Alva
#SPK_TARGET = BrailleLite
#SPK_TARGET = CombiBraille
#SPK_TARGET = Festival
#SPK_TARGET = Generic_say
#SPK_TARGET = Televox

# Specify the default, compiled-in text translation table.  This can be
# overridden at run-time by giving brltty the -t option.
# See the content of the BrailleTable directory for available tables.
TEXTTRANS = text.us.tbl

# Specify the device name for the serial port your Braille display will
# normally be connected to.  For port com(n), use /dev/ttyS(n-1) -
# e.g. for com1 use /dev/ttyS0
# [Note that this port can be overridden from the brltty command-line 
# parameter `-d'.  See the documentation for further details.]
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
# help files.  If this directory doesn't exist it will be created.
# (You may prefer somewhere under /usr/lib or /usr/local/lib ...)
DATA_DIR = /etc/brltty

# where to look for libs
LIB_PATH = /lib/brltty

# Edit this definition (options for install(1)) if you want to install
# setuid or setgid to allow ordinary users to start it.  For instance,
# to install setuid and executable by anyone, you could use permissions
# 4755 (rwsr-xr-x).
INSTALL_USER = root
INSTALL_GROUP = root
INSTALL_MODE = 0744
INSTALL_EXEC = --owner=$(INSTALL_USER) --group=$(INSTALL_GROUP) --mode=$(INSTALL_MODE)
INSTALL_LIB = --owner=$(INSTALL_USER) --group=$(INSTALL_GROUP) --mode=$(INSTALL_MODE)


# Screen access methods (OS dependent).  Uncomment only one of the
# two following sets of options (if not sure, don't touch anything):

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
MAKE = make

CC = gcc
# To compile in a.out (if you use ELF by default), you may be able to use
# `-b i486-linuxaout'; however, you may also need to use the -V flag, or
# possibly a different gcc executable, depending on your setup.
COMMONCFLAGS = -O2 -Wall -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE
CFLAGS = $(COMMONCFLAGS) '-DBRL_TARGET="$(BRL_TARGET)"' -D$(SPK_TARGET)
UTILS_CFLAGS = $(CFLAGS)

LIB_CFLAGS = $(CFLAGS) -fPIC
LIB_SO_NAME = libbrltty
LIB_VER = 1

COMPCPP = g++

#LD = gcc
LD = g++
LDFLAGS = -s
#LDFLAGS = -g
LDLIBS = -ldl -rdynamic

PREFIX =

TARGETS = Alva BrailleLite CombiBraille EcoBraille EuroBraille MDV Papenmeier TSI Vario
TRGLIBS = al b1 b4 cb ec eu md pm ts va
# ------------------------ DO NOT EDIT BELOW THIS LINE ------------------------


.EXPORT_ALL_VARIABLES:

all: brltty install-brltty txt2hlp brltest scrtest

install: install-programs install-help install-tables install-drivers install-devices

install-programs: brltty install-brltty txt2hlp
	install $(INSTALL_EXEC) --strip brltty $(PREFIX)$(EXEC_PATH) 
	install $(INSTALL_EXEC) install-brltty $(PREFIX)$(REINSTALL_PATH) 

install-help: brltty
	install --directory $(PREFIX)$(DATA_DIR) 
	install --mode=0644 help/* $(PREFIX)$(DATA_DIR) 

install-tables:
	install --directory $(PREFIX)$(DATA_DIR) 
	install -m 644 BrailleTables/*.tbl $(PREFIX)$(DATA_DIR)

install-drivers: brltty
	install --directory $(PREFIX)$(LIB_PATH)
	install $(INSTALL_LIB) lib/* $(PREFIX)$(LIB_PATH)

install-devices:
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
	rm -rf $(PREFIX)$(LIB_PATH)/$(LIB_SO_NAME)??.so.$(LIB_VER)
	rm -rf $(PREFIX)$(DATA_DIR)

install-brltty: install.template
	sed -e 's%=E%$(EXEC_PATH)%g' -e 's%=I%$(REINSTALL_PATH)%g' \
	  -e 's%=D%$(DATA_DIR)%g' -e 's%=V%$(VCSADEV)%g' install.template > $@

SCREEN_OBJECTS = scr.o scrdev.o $(SCR_O)

SPEECH_TARGETS = speech
SPEECH_OBJECTS = $(SPK_TARGET)/speech.o 

ifeq ($(BRL_TARGET),)
   DRIVER_TARGETS = brl_dynamic.o dynamic-drivers
   DRIVER_OBJECTS = brl_dynamic.o
else
   DRIVER_TARGETS = brl_static.o static-driver
   DRIVER_OBJECTS = brl_static.o $(BRL_TARGET)/brl.o
endif

BRLTTY_OBJECTS = brltty.o misc.o beeps.o cut-n-paste.o $(INSKEY_O)
brltty: $(BRLTTY_OBJECTS) $(SCREEN_OBJECTS) $(SPEECH_TARGETS) $(DRIVER_TARGETS)
	$(LD) $(LDFLAGS) -Wl,-rpath,$(LIB_PATH) -o $@ \
	 $(BRLTTY_OBJECTS) $(SCREEN_OBJECTS) $(SPEECH_OBJECTS) $(DRIVER_OBJECTS) $(LDLIBS)

brltty.o: brltty.c brl.h scr.h inskey.h misc.h message.h config.h \
	  text.auto.h attrib.auto.h
	$(CC) $(CFLAGS) '-DHOME_DIR="$(DATA_DIR)"' '-DTRGLIBS="$(TRGLIBS)"' '-DBRLDEV="$(BRLDEV)"' -c brltty.c

misc.o: misc.c misc.h
	$(CC) $(CFLAGS) -c misc.c

beeps.o: beeps.c beeps.h
	$(CC) $(CFLAGS) -c beeps.c

cut-n-paste.o: cut-n-paste.c cut-n-paste.h beeps.h scr.h inskey.h
	$(CC) $(CFLAGS) -c cut-n-paste.c

inskey_lnx.o: inskey_lnx.c inskey.h
	$(CC) $(CFLAGS) -c inskey_lnx.c

brl_dynamic.o: brl_dynamic.c brl.h scr.h
	$(CC) $(CFLAGS) '-DLIB_PATH="$(LIB_PATH)"' -c brl_dynamic.c 

dynamic-drivers: txt2hlp
	for target in $(TARGETS); \
	do $(MAKE) -C $$target lib help || exit 1; \
	done

brl_static.o: brl_static.c brl.h scr.h
	$(CC) $(CFLAGS) '-DLIB_PATH="$(LIB_PATH)"' -c brl_static.c 

static-driver: txt2hlp
	$(MAKE) -C $(BRL_TARGET) brl.o help

scr.o: scr.cc scr.h scrdev.h helphdr.h config.h
	$(COMPCPP) $(CFLAGS) -c scr.cc

scrdev.o: scrdev.cc scr.h scrdev.h helphdr.h config.h
	$(COMPCPP) $(CFLAGS) -c scrdev.cc

scr_vcsa.o: scr_vcsa.cc scr_vcsa.h scr.h scrdev.h config.h
	$(COMPCPP) $(CFLAGS) '-DVCSADEV="$(VCSADEV)"' -c scr_vcsa.cc

scr_shm.o: scr_shm.cc scr_shm.h scr.h scrdev.h config.h
	$(COMPCPP) $(CFLAGS) -c scr_shm.cc

speech:
	$(MAKE) -C $(SPK_TARGET) speech.o

BRLTEST_OBJECTS = brltest.o misc.o
brltest: $(BRLTEST_OBJECTS) $(DRIVER_TARGETS)
	$(LD) $(LDFLAGS) -o $@ $(BRLTEST_OBJECTS) $(DRIVER_OBJECTS) $(LDLIBS)

brltest.o: brltest.c brl.h config.h text.auto.h
	$(CC) $(CFLAGS) '-DHOME_DIR="$(DATA_DIR)"' '-DBRLDEV="$(BRLDEV)"' -c brltest.c

SCRTEST_OBJECTS = scrtest.o misc.o
scrtest: $(SCRTEST_OBJECTS) $(SCREEN_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(SCRTEST_OBJECTS) $(SCREEN_OBJECTS) $(LDLIBS)

scrtest.o: scrtest.c 
	$(CC) $(CFLAGS) -c scrtest.c

# quick hack to check all libs
# try to load all libs - start brltty with library
checklibs: brltty dynamic-drivers
	if [ $$UID = 0 ]; then exit 99; fi
	for lib in $(TRGLIBS); \
	do ls -al lib/*$$lib* || exit 1; \
	   LD_RUN_PATH="./lib" ./brltty -v -m $$lib 2>&1 || exit 1; \
	done

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
	rm -f *.o */*.o lib/* help/* install-brltty *.auto.h

distclean: clean
	rm -f brltty txt2hlp comptable *test
	rm -f *~ */*~ *orig */*orig \#*\# */\#*\#
	rm -f Papenmeier/serial
	$(MAKE) -C BrailleTables distclean


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


# If you would like the driver for your Braille display built into the
# program, then specify it by uncommenting one, and only one, of these
# definitions of BRL_TARGET. If you do not specify the display here,
# then a set of shared libraries, one for each driver, will be made,
# and you will need to specify the display type at run-time via -b.
# Note to maintainer:
#    When adding a new target here, add it also to the "BRL_TARGETS =" line.
#BRL_TARGET = Alva
#BRL_TARGET = BrailleLite
#BRL_TARGET = CombiBraille
#BRL_TARGET = EcoBraille
#BRL_TARGET = EuroBraille
#BRL_TARGET = MDV
#BRL_TARGET = Papenmeier
#BRL_TARGET = TSI
#BRL_TARGET = Vario

# If you would like the driver for your speech interface built into the
# program, then specify it by uncommenting one, and only one, of these
# definitions of SPK_TARGET. If you do not specify the interface here,
# then a set of shared libraries, one for each driver, will be made,
# and you will need to specify the interface type at run-time via -s.
# Note to maintainer:
#    When adding a new target here, add it also to the "SPK_TARGETS =" line.
# PS: Alva speech is available with Delphi models only.
# Note: All this is still experimental.  It is safe to leave all of
# these entries commented out.
#SPK_TARGET = Alva
#SPK_TARGET = BrailleLite
#SPK_TARGET = CombiBraille
#SPK_TARGET = Festival
#SPK_TARGET = GenericSay
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
COMMONCFLAGS = -Wall -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE
CFLAGS = $(COMMONCFLAGS) -O2
#CFLAGS = $(COMMONCFLAGS) -g
UTILS_CFLAGS = $(CFLAGS)

LIB_CFLAGS = $(CFLAGS) -fPIC
LIB_SO_NAME = libbrltty
LIB_VER = 1

COMPCPP = g++

#LD = gcc
LD = g++
COMMONLDFLAGS = -rdynamic
LDFLAGS = $(COMMONLDFLAGS) -s
#LDFLAGS = $(COMMONLDFLAGS) -g
LDLIBS = -ldl

PREFIX =

INSTALL_DRIVERS =

BRL_TARGETS = Alva BrailleLite CombiBraille EcoBraille EuroBraille MDV Papenmeier TSI Vario
BRL_LIBS = al b1 b4 cb ec eu md pm ts va

SPK_TARGETS = NoSpeech Alva BrailleLite CombiBraille Festival GenericSay Televox
SPK_LIBS = no al bl cb fv gs tv

# ------------------------ DO NOT EDIT BELOW THIS LINE ------------------------


.EXPORT_ALL_VARIABLES:

all: brltty install-brltty txt2hlp brltest scrtest

install-brltty: install.template
	sed -e 's%=E%$(EXEC_PATH)%g' -e 's%=I%$(REINSTALL_PATH)%g' \
	  -e 's%=D%$(DATA_DIR)%g' -e 's%=V%$(VCSADEV)%g' install.template > $@

SCREEN_OBJECTS = scr.o scrdev.o $(SCR_O)

ifeq ($(SPK_TARGET),)
   SPK_TARGET = NoSpeech
   INSTALL_DRIVERS = install-drivers
   SPEECH_TARGETS = dynamic-speech
else
   SPEECH_TARGETS = static-speech
endif
SPEECH_OBJECTS = $(SPK_TARGET)/speech.o 
BUILTIN_SPEECH = -DSPK_BUILTIN

ifeq ($(BRL_TARGET),)
   INSTALL_DRIVERS = install-drivers
   BRAILLE_TARGETS = dynamic-braille
   BRAILLE_OBJECTS =
   BUILTIN_BRAILLE =
else
   BRAILLE_TARGETS = static-braille
   BRAILLE_OBJECTS = $(BRL_TARGET)/brl.o
   BUILTIN_BRAILLE = -DBRL_BUILTIN
endif

BRLTTY_OBJECTS = brltty.o misc.o beeps.o cut-n-paste.o $(INSKEY_O) spk_load.o brl_load.o

brltty: $(BRLTTY_OBJECTS) $(SCREEN_OBJECTS) $(SPEECH_TARGETS) $(BRAILLE_TARGETS)
	$(LD) $(LDFLAGS) -Wl,-rpath,$(LIB_PATH) -o $@ \
	  $(BRLTTY_OBJECTS) $(SCREEN_OBJECTS) $(SPEECH_OBJECTS) $(BRAILLE_OBJECTS) $(LDLIBS)

brltty.o: brltty.c brl.h scr.h inskey.h misc.h message.h config.h \
	  text.auto.h attrib.auto.h
	$(CC) $(CFLAGS) '-DHOME_DIR="$(DATA_DIR)"' '-DBRLLIBS="$(BRL_LIBS)"' '-DSPKLIBS="$(SPK_LIBS)"' '-DBRLDEV="$(BRLDEV)"' -c brltty.c

misc.o: misc.c misc.h
	$(CC) $(CFLAGS) -c misc.c

beeps.o: beeps.c beeps.h
	$(CC) $(CFLAGS) -c beeps.c

cut-n-paste.o: cut-n-paste.c cut-n-paste.h beeps.h scr.h inskey.h
	$(CC) $(CFLAGS) -c cut-n-paste.c

inskey_lnx.o: inskey_lnx.c inskey.h
	$(CC) $(CFLAGS) -c inskey_lnx.c

spk_load.o: spk_load.c spk.h brl.h misc.h
	$(CC) $(CFLAGS) '-DLIB_PATH="$(LIB_PATH)"' $(BUILTIN_SPEECH) -c spk_load.c 

dynamic-speech:
	rm -f lib/brltty-spk.lst
	for target in $(SPK_TARGETS); \
	do $(MAKE) -C $$target speech-driver spk-lib-name || exit 1; \
	done

static-speech:
	$(MAKE) -C $(SPK_TARGET) speech.o

brl_load.o: brl_load.c brl.h misc.h
	$(CC) $(CFLAGS) '-DLIB_PATH="$(LIB_PATH)"' $(BUILTIN_BRAILLE) -c brl_load.c 

dynamic-braille: txt2hlp
	rm -f lib/brltty-brl.lst
	for target in $(BRL_TARGETS); \
	do $(MAKE) -C $$target braille-driver braille-help brl-lib-name || exit 1; \
	done

static-braille: txt2hlp
	$(MAKE) -C $(BRL_TARGET) brl.o braille-help

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

BRLTEST_OBJECTS = brltest.o misc.o brl_load.o
brltest: $(BRLTEST_OBJECTS) $(BRAILLE_TARGETS)
	$(LD) $(LDFLAGS) -o $@ $(BRLTEST_OBJECTS) $(BRAILLE_OBJECTS) $(LDLIBS)

brltest.o: brltest.c brl.h config.h text.auto.h
	$(CC) $(CFLAGS) '-DHOME_DIR="$(DATA_DIR)"' '-DBRLDEV="$(BRLDEV)"' -c brltest.c

SCRTEST_OBJECTS = scrtest.o misc.o
scrtest: $(SCRTEST_OBJECTS) $(SCREEN_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(SCRTEST_OBJECTS) $(SCREEN_OBJECTS) $(LDLIBS)

scrtest.o: scrtest.c 
	$(CC) $(CFLAGS) -c scrtest.c

# quick hack to check all libs
# try to load all libs - start brltty with library
check-braille: brltty dynamic-braille
	if [ $$UID = 0 ]; then exit 99; fi
	for lib in $(BRL_LIBS); \
	do ls -al lib/*$$lib* || exit 1; \
	   LD_RUN_PATH="./lib" ./brltty -v -b $$lib 2>&1 || exit 1; \
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

install: install-programs install-help install-tables $(INSTALL_DRIVERS) install-devices

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
	rm -f $(PREFIX)$(LIB_PATH)/$(LIB_SO_NAME)*
	rm -f $(PREFIX)$(LIB_PATH)/brltty-*.lst
	rmdir $(PREFIX)$(LIB_PATH)
	rm -f $(PREFIX)$(DATA_DIR)/*.tbl
	rm -f $(PREFIX)$(DATA_DIR)/brltty*
	rmdir $(PREFIX)$(DATA_DIR)

clean:
	rm -f *.o */*.o lib/* help/* install-brltty *.auto.h core

distclean: clean
	rm -f brltty txt2hlp comptable *test
	rm -f *~ */*~ *orig */*orig \#*\# */\#*\# *.rej */*.rej
	$(MAKE) -C Papenmeier distclean
	$(MAKE) -C BrailleTables distclean


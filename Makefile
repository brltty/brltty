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



# If you would like the driver for your Braille display built into the
# program, then specify it by uncommenting one, and only one, of these
# definitions of BRL_TARGET. If you do not specify the display here,
# then a set of shared libraries, one for each driver, will be made,
# and you will need to specify the display type at run-time via
# either the -b option or the braille-driver directive.
# Note to maintainer:
#    When adding a new target here, add it also to the "BRL_TARGETS =" line.
#BRL_TARGET = Alva
#BRL_TARGET = BrailleLite
#BRL_TARGET = BrailleNote
#BRL_TARGET = CombiBraille
#BRL_TARGET = EcoBraille
#BRL_TARGET = EuroBraille
#BRL_TARGET = HandyTech
#BRL_TARGET = LogText
#BRL_TARGET = MDV
#BRL_TARGET = MiniBraille
#BRL_TARGET = MultiBraille
#BRL_TARGET = Papenmeier
#BRL_TARGET = TSI
#BRL_TARGET = Vario
#BRL_TARGET = Vario-HT
#BRL_TARGET = VideoBraille
#BRL_TARGET = VisioBraille
#BRL_TARGET = Voyager

# Specify the device path for the port your Braille display will
# normally be connected to.  For port com(n), use /dev/ttyS(n-1) -
# e.g. for com1 use /dev/ttyS0
# This specification can be overridden by either the -d option or the
# braille-device directive.
BRLDEV = /dev/ttyS0

# If you would like the driver for your speech interface built into the
# program, then specify it by uncommenting one, and only one, of these
# definitions of SPK_TARGET. If you do not specify the interface here,
# then a set of shared libraries, one for each driver, will be made,
# and you will need to specify the interface type at run-time via either
# the -s option or the speech-driver directive.
# It is safe to leave all of these entries commented out.
# Note: Alva speech is available with Delphi models only.
# Note to maintainer:
#    When adding a new target here, add it also to the "SPK_TARGETS =" line.
#SPK_TARGET = Alva
#SPK_TARGET = BrailleLite
#SPK_TARGET = CombiBraille
#SPK_TARGET = ExternalSpeech
#SPK_TARGET = Festival
#SPK_TARGET = GenericSay
#SPK_TARGET = Televox

# Specify the default, compiled-in, text translation table.  This can be
# overridden at run-time by either the -t option or the text-table directive.
# Uncomment exactly one of the following lines.
# It is safe to leave all of them commented out.
# See the content of the BrailleTables directory for more tables.
#TEXTTRANS = text.danish.tbl      # Danish
#TEXTTRANS = text.es.tbl          # Spanish
#TEXTTRANS = text.french.tbl      # French
#TEXTTRANS = text.german.tbl      # German
#TEXTTRANS = text.it.tbl          # Italian
#TEXTTRANS = text.no-h.tbl        # Norwegian and German
#TEXTTRANS = text.no-p.tbl        # Norwegian
#TEXTTRANS = text.pl-iso02.tbl    # Polish (iso-8859-2)
#TEXTTRANS = text.simple.tbl      # American English
#TEXTTRANS = text.sweden.tbl      # Swedish
#TEXTTRANS = text.swedish.tbl     # Swedish
#TEXTTRANS = text.uk.tbl          # United Kingdom English
#TEXTTRANS = text.us.tbl          # American English

# Specify the default, compiled-in, attributes translation table.  This can be
# overridden at run-time by either the -a option or the attributes-table directive.
# Uncomment exactly one of the following lines.
# See the content of the BrailleTables directory for more tables.
ATTRTRANS = attributes.tbl
#ATTRTRANS = attrib.tbl

# Set INSTALL_ROOT if you need to install BRLTTY into a location which is
# different from the one where it will be run from.
INSTALL_ROOT = 

# Specify the anchor for the installed file hierarchy. Even though this setting
# only determines the location of the installed files, it's important to have
# it set consistently throughout the entire build process so that the various
# components of BRLTTY will know where to find each other.
# NOTE: if you intend to have brltty start up automatically at
# boot-time, the executable and all data files should be in your root
# filesystem.
PREFIX =

# Specify the directory where the executable programs are to be placed:
PROG_DIR = /sbin

# Specify the directory where BRLTTY keeps its translation tables and its
# help files.  If this directory doesn't exist it will be created.
DATA_DIR = /etc/brltty

# Specify the directory where BRLTTY keeps its shared libraries.
# If this directory doesn't exist it will be created.
LIB_DIR = /lib/brltty

# Edit this definition (options for install(1)) if you want to install
# setuid or setgid to allow ordinary users to start it.  For instance,
# to install setuid and executable by anyone, you could use permissions
# 4755 (rwsr-xr-x).
INSTALL_USER = root
INSTALL_GROUP = root
INSTALL_MODE = 0744

# Screen access methods (OS dependent).  Uncomment exactly one of the
# following screen driver definitions (if not sure, don't touch anything).

# The Linux screen driver.
SCR_O = scr_linux.o

# You may define this one as an alternative to the native screen driver for
# your operating system. It works in conjunction with the "screen" package
# to provide access for any UNIX platform which supports IPC (like SVR4).
# See Patches/screen-x.x.x.txt for more information.
# NOTE: This is completely experimental.
#SCR_O = scr_shm.o

# Parameter for the Linux screen driver:
# These are the names by which the character device which BRLTTY uses
# to read the screen under Linux is commonly known.
# If none of them exist during the install, then the first one is created.
# Don't modify this unless you know what you're doing!
VCSADEV = /dev/vcsa /dev/vcsa0 /dev/vcc/a

# The following are compiler settings.  If you don't know what they mean,
# LEAVE THEM ALONE!

# This is for the unlikely case where you would want to use a cross-compiler
# (to compile for another archtecture).
CROSSCOMP=
#CROSSCOMP=arm-linux-

HOSTCC = gcc
CC = $(CROSSCOMP)$(HOSTCC)
HOSTLD = ld
LD = $(CROSSCOMP)$(HOSTLD)

# To compile in a.out (if you use ELF by default), you may be able to use
# `-b i486-linuxaout'; however, you may also need to use the -V flag, or
# possibly a different gcc executable, depending on your setup.

COMMONCFLAGS = -Wall -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE
HOSTCFLAGS = $(COMMONCFLAGS) -O2
#HOSTCFLAGS = $(COMMONCFLAGS) -g
CFLAGS = $(HOSTCFLAGS)
UTILS_CFLAGS = $(CFLAGS)

LIB_CFLAGS = $(CFLAGS) -fPIC
LIB_SO_NAME = libbrltty
LIB_VER = 1

COMPCPP = $(CROSSCOMP)gcc

COMMONLDFLAGS = -rdynamic
LDFLAGS = $(COMMONLDFLAGS) -s
#LDFLAGS = $(COMMONLDFLAGS) -g
LDLIBS = -ldl -lm -lc

# Bootdisk hacking:
# Uncomment next line for BRLTTY to be able to act as /sbin/init
# replacement on a bootdisk.
#INIT_HACK=1
ifdef INIT_HACK
  INIT_PATH = "/sbin/init_real"
  INIT_PATH_FLAGS := '-DINIT_PATH=$(INIT_PATH)'
else
  INIT_PATH_FLAGS =
endif

# Uncomment next line to produce a statically linked executable.
#LINKSTATIC = 1
ifdef LINKSTATIC
  LDFLAGS += -static -s
endif

MAKE = make

###############################################################################

ifeq ($(TEXTTRANS),)
   TEXTTRANS = text.simple.tbl
endif

INSTALL_EXEC = --owner=$(INSTALL_USER) --group=$(INSTALL_GROUP) --mode=$(INSTALL_MODE)
INSTALL_LIB = --owner=$(INSTALL_USER) --group=$(INSTALL_GROUP) --mode=$(INSTALL_MODE)

INSTALL_DRIVERS =

BRL_TARGETS = Alva BrailleLite BrailleNote CombiBraille EcoBraille EuroBraille HandyTech LogText MDV MiniBraille MultiBraille Papenmeier TSI Vario Vario-HT VideoBraille VisioBraille Voyager
BRL_LIBS = al bl bn cb ec eu ht lt md mn mb pm ts va vh vd vs vo

SPK_TARGETS = Alva BrailleLite CombiBraille ExternalSpeech Festival GenericSay Televox
SPK_LIBS = al bl cb es fv gs tv

ifneq (,$(wildcard /usr/include/eci.h))
SPK_TARGETS += ViaVoice
SPK_LIBS += vv
endif

# ------------------------ DO NOT EDIT BELOW THIS LINE ------------------------


.EXPORT_ALL_VARIABLES:

all: brltty install-brltty txt2hlp brltest scrtest tunetest

install-brltty: install.template
	sed -e 's%=F%$(PREFIX)%g' \
	    -e 's%=P%$(PROG_DIR)%g' \
            -e 's%=L%$(LIB_DIR)%g' \
	    -e 's%=D%$(DATA_DIR)%g' \
            -e 's%=V%$(VCSADEV)%g' \
            $< >$@

SCREEN_OBJECTS = scr.o scr_base.o $(SCR_O)

ifeq ($(SPK_TARGET),)
   INSTALL_DRIVERS = install-drivers
   SPEECH_TARGETS = dynamic-speech
   SPEECH_OBJECTS =
   BUILTIN_SPEECH =
else
   SPEECH_TARGETS = static-speech
   SPEECH_OBJECTS = $(SPK_TARGET)/speech.o 
   BUILTIN_SPEECH = -DSPK_BUILTIN
endif

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

CONTRACT_OBJECTS = ctb_compile.o ctb_translate.o

ctb_compile.o: ctb_compile.c ctb_definitions.h contract.h misc.h brl.h
	$(CC) $(CFLAGS) -c ctb_compile.c

ctb_translate.o: ctb_translate.c ctb_definitions.h contract.h misc.h brl.h
	$(CC) $(CFLAGS) -c ctb_translate.c

TUNE_OBJECTS = tunes.o tones_speaker.o tones_soundcard.o tones_sequencer.o tones_adlib.o adlib.o

tunes.o: tunes.c tunes.h tones.h common.h misc.h message.h brl.h
	$(CC) $(CFLAGS) -c tunes.c

tones_speaker.o: tones_speaker.c tones.h
	$(CC) $(CFLAGS) -c tones_speaker.c

tones_soundcard.o: tones_soundcard.c tones.h
	$(CC) $(CFLAGS) -c tones_soundcard.c

tones_sequencer.o: tones_sequencer.c tones.h misc.h
	$(CC) $(CFLAGS) -c tones_sequencer.c

tones_adlib.o: tones_adlib.c tones.h adlib.h misc.h
	$(CC) $(CFLAGS) -c tones_adlib.c

adlib.o: adlib.c adlib.h misc.h
	$(CC) $(CFLAGS) -c adlib.c

BRLTTY_OBJECTS = main.o config.o route.o misc.o $(CONTRACT_OBJECTS) $(TUNE_OBJECTS) cut.o spk_load.o brl_load.o

brltty-static: $(BRLTTY_OBJECTS) $(SCREEN_OBJECTS) $(SPEECH_TARGETS) $(BRAILLE_TARGETS)
	$(CC) $(LDFLAGS) -static -Wl,--export-dynamic,-rpath,$(LIB_DIR) -o $@ \
	  $(BRLTTY_OBJECTS) $(SCREEN_OBJECTS) $(SPEECH_OBJECTS) $(BRAILLE_OBJECTS) $(LDLIBS)

brltty: $(BRLTTY_OBJECTS) $(SCREEN_OBJECTS) $(SPEECH_TARGETS) $(BRAILLE_TARGETS)
	$(CC) $(LDFLAGS) -Wl,-rpath,$(LIB_DIR) -o $@ \
	  $(BRLTTY_OBJECTS) $(SCREEN_OBJECTS) $(SPEECH_OBJECTS) $(BRAILLE_OBJECTS) $(LDLIBS)

main.o: main.c brl.h spk.h scr.h contract.h tunes.h cut.h route.h \
	misc.h message.h config.h common.h
	$(CC) $(CFLAGS) $(INIT_PATH_FLAGS) -c main.c

config.o: config.c config.h brl.h spk.h scr.h contract.h tunes.h message.h misc.h common.h
	$(CC) $(CFLAGS) \
		'-DHOME_DIR="$(PREFIX)$(DATA_DIR)"' \
		'-DBRLLIBS="$(BRL_LIBS)"' \
		'-DSPKLIBS="$(SPK_LIBS)"' \
		'-DBRLDEV="$(BRLDEV)"' \
		'-DTEXTTRANS="$(TEXTTRANS)"' \
		'-DATTRTRANS="$(ATTRTRANS)"' \
                -c config.c

route.o: route.c route.h scr.h misc.h
	$(CC) $(CFLAGS) -c route.c

misc.o: misc.c misc.h config.h brl.h common.h
	$(CC) $(CFLAGS) -c misc.c

cut.o: cut.c cut.h tunes.h scr.h
	$(CC) $(CFLAGS) -c cut.c

spk_load.o: spk_load.c spk.h brl.h misc.h
	$(CC) $(CFLAGS) '-DLIB_PATH="$(LIB_DIR)"' $(BUILTIN_SPEECH) -c spk_load.c 

dynamic-speech:
	rm -f lib/brltty-spk.lst
	for target in $(SPK_TARGETS); \
	do $(MAKE) -C $$target speech-driver spk-lib-name || exit 1; \
	done

static-speech:
	$(MAKE) -C $(SPK_TARGET) speech.o

brl_load.o: brl_load.c brl.h misc.h message.h text.auto.h attrib.auto.h cmds.auto.h
	$(CC) $(CFLAGS) '-DLIB_PATH="$(LIB_DIR)"' $(BUILTIN_BRAILLE) -c brl_load.c 

dynamic-braille: txt2hlp
	rm -f lib/brltty-brl.lst
	for target in $(BRL_TARGETS); \
	do $(MAKE) -C $$target braille-driver braille-help brl-lib-name || exit 1; \
	done

static-braille: txt2hlp
	$(MAKE) -C $(BRL_TARGET) brl.o braille-help

scr.o: scr.cc scr.h scr_base.h config.h
	$(COMPCPP) $(CFLAGS) -c scr.cc

scr_base.o: scr_base.cc scr.h scr_base.h route.h help.h misc.h config.h
	$(COMPCPP) $(CFLAGS) -c scr_base.cc

scr_linux.o: scr_linux.cc scr_linux.h scr.h scr_base.h
	$(COMPCPP) $(CFLAGS) '-DVCSADEV="$(VCSADEV)"' -c scr_linux.cc

scr_shm.o: scr_shm.cc scr_shm.h scr.h scr_base.h config.h
	$(COMPCPP) -D_XOPEN_SOURCE $(CFLAGS) -c scr_shm.cc

speech:
	$(MAKE) -C $(SPK_TARGET) speech.o

check-brltest:
	$(MAKE) distclean
	for target in $(BRL_TARGETS); \
	do $(MAKE) BRL_TARGET=$$target brltest || exit 99; \
	   $(MAKE) distclean; \
	done

BRLTEST_OBJECTS = brltest.o misc.o brl_load.o
brltest: $(BRLTEST_OBJECTS) $(BRAILLE_TARGETS)
	$(CC) $(LDFLAGS) -o $@ $(BRLTEST_OBJECTS) $(BRAILLE_OBJECTS) $(LDLIBS)

brltest.o: brltest.c brl.h config.h
	$(CC) $(CFLAGS) '-DHOME_DIR="$(PREFIX)$(DATA_DIR)"' '-DBRLDEV="$(BRLDEV)"' -c brltest.c

SCRTEST_OBJECTS = scrtest.o misc.o route.o
scrtest: $(SCRTEST_OBJECTS) $(SCREEN_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(SCRTEST_OBJECTS) $(SCREEN_OBJECTS) $(LDLIBS)

scrtest.o: scrtest.c 
	$(CC) $(CFLAGS) -c scrtest.c

TUNETEST_OBJECTS = tunetest.o misc.o
tunetest: $(TUNETEST_OBJECTS) $(TUNE_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(TUNETEST_OBJECTS) $(TUNE_OBJECTS) $(LDLIBS)

tunetest.o: tunetest.c tunes.h misc.h config.h common.h message.h brl.h
	$(CC) $(CFLAGS) -c tunetest.c

# quick hack to check all libs
# try to load all libs - start brltty with library
check-braille: brltty dynamic-braille
	if [ $$UID = 0 ]; then exit 99; fi
	for lib in $(BRL_LIBS); \
	do ls -al lib/libbrlttyb$$lib.* || exit 1; \
	   LD_RUN_PATH="./lib" ./brltty -v -q -N -e -f /dev/null -b $$lib 2>&1 || exit 1; \
	done

text.auto.h: BrailleTables/$(TEXTTRANS) tbl2hex
	./tbl2hex <$< >$@

attrib.auto.h: BrailleTables/$(ATTRTRANS) tbl2hex
	./tbl2hex <$< >$@

cmds.auto.h: brl.h cmds.awk
	awk -f cmds.awk $< >$@

txt2hlp: txt2hlp.c help.h
	$(HOSTCC) $(LDFLAGS) -o $@ txt2hlp.c $(LDLIBS)

tbl2hex: tbl2hex.c
	$(HOSTCC) $(LDFLAGS) -o $@ tbl2hex.c $(LDLIBS)

install: install-programs install-help install-tables $(INSTALL_DRIVERS) install-devices

install-programs: brltty install-brltty txt2hlp
	install --directory $(INSTALL_ROOT)$(PREFIX)$(PROG_DIR) 
	install $(INSTALL_EXEC) --strip brltty $(INSTALL_ROOT)$(PREFIX)$(PROG_DIR) 
	install $(INSTALL_EXEC) install-brltty $(INSTALL_ROOT)$(PREFIX)$(PROG_DIR) 

install-help: brltty
	install --directory $(INSTALL_ROOT)$(PREFIX)$(DATA_DIR) 
	install --mode=0644 help/* $(INSTALL_ROOT)$(PREFIX)$(DATA_DIR) 

install-tables:
	install --directory $(INSTALL_ROOT)$(PREFIX)$(DATA_DIR) 
	install -m 644 BrailleTables/*.tbl $(INSTALL_ROOT)$(PREFIX)$(DATA_DIR)
	install -m 644 ContractionTables/*.ctb $(INSTALL_ROOT)$(PREFIX)$(DATA_DIR)

install-drivers: brltty
	install --directory $(INSTALL_ROOT)$(PREFIX)$(LIB_DIR)
	install $(INSTALL_LIB) lib/* $(INSTALL_ROOT)$(PREFIX)$(LIB_DIR)

install-devices: install-screen-device

SCREEN_DEVICE := $(firstword $(VCSADEV))
install-screen-device:
ifneq (0,$(words $(VCSADEV)))
ifeq (,$(wildcard $(addprefix $(INSTALL_ROOT),$(VCSADEV))))
	install --directory $(dir $(INSTALL_ROOT)$(SCREEN_DEVICE))
	mknod -m o= $(INSTALL_ROOT)$(SCREEN_DEVICE) c 7 128
	chmod 620 $(INSTALL_ROOT)$(SCREEN_DEVICE)
	chown root.tty $(INSTALL_ROOT)$(SCREEN_DEVICE)
endif
endif

uninstall:
	rm -f $(INSTALL_ROOT)$(PREFIX)$(PROG_DIR)/brltty $(INSTALL_ROOT)$(PREFIX)$(PROG_DIR)/install-brltty
	rm -f $(INSTALL_ROOT)$(PREFIX)$(LIB_DIR)/$(LIB_SO_NAME)*
	rm -f $(INSTALL_ROOT)$(PREFIX)$(LIB_DIR)/brltty-*.lst
	rmdir $(INSTALL_ROOT)$(PREFIX)$(LIB_DIR)
	rm -f $(INSTALL_ROOT)$(PREFIX)$(DATA_DIR)/*.tbl
	rm -f $(INSTALL_ROOT)$(PREFIX)$(DATA_DIR)/*.ctb
	rm -f $(INSTALL_ROOT)$(PREFIX)$(DATA_DIR)/brltty*
	rmdir $(INSTALL_ROOT)$(PREFIX)$(DATA_DIR)

clean:
	rm -f brltty install-brltty txt2hlp tbl2hex
	rm -f *.o */*.o lib/* help/* *.auto.h core

distclean: clean
	rm -f *test *-static *~ */*~ *orig */*orig \#*\# */\#*\# *.rej */*.rej
	rm -f ? */? a.out */a.out
	$(MAKE) -C BrailleLite distclean
	$(MAKE) -C Papenmeier distclean
	$(MAKE) -C Voyager distclean
	$(MAKE) -C BrailleTables distclean


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

all: brltty install-brltty brltest scrtest tunetest

include setup.mk

###############################################################################

options.o:
	$(CC) $(CFLAGS) -c options.c

###############################################################################

ctb_compile.o:
	$(CC) $(CFLAGS) -c ctb_compile.c

ctb_translate.o:
	$(CC) $(CFLAGS) -c ctb_translate.c

###############################################################################

SCREEN_OBJECTS = scr.o scr_base.o $(SCR_OBJ)

scr.o:
	$(CXX) $(CXXFLAGS) -c scr.cc

scr_base.o:
	$(CXX) $(CXXFLAGS) -c scr_base.cc

scr_linux.o:
	$(CXX) $(CXXFLAGS) -c scr_linux.cc

scr_shm.o:
	$(CXX) $(CXXFLAGS) -c scr_shm.cc

###############################################################################

TUNE_OBJECTS = tunes.o $(SPKR_OBJS) $(DAC_OBJS) $(MIDI_OBJS) $(ADLIB_OBJS)

tunes.o:
	$(CC) $(CFLAGS) -c tunes.c

tones_speaker.o:
	$(CC) $(CFLAGS) -c tones_speaker.c

tones_dac.o:
	$(CC) $(CFLAGS) -c tones_dac.c

tones_midi.o:
	$(CC) $(CFLAGS) -c tones_midi.c

tones_adlib.o:
	$(CC) $(CFLAGS) -c tones_adlib.c

adlib.o:
	$(CC) $(CFLAGS) -c adlib.c

###############################################################################

BRLTTY_OBJECTS = main.o config.o options.o misc.o $(SYS_OBJ) route.o cut.o brl.o spk.o $(CTB_OBJS) $(SCREEN_OBJECTS) $(TUNE_OBJECTS)

brltty: $(BRLTTY_OBJECTS) $(BRAILLE_TARGETS) $(SPEECH_TARGETS)
	$(CXX) $(LDFLAGS) -o $@ $(BRLTTY_OBJECTS) $(BRAILLE_OBJECTS) $(SPEECH_OBJECTS) $(LDLIBS)

main.o:
	$(CC) $(CFLAGS) -c main.c

config.o:
	$(CC) $(CFLAGS) -c config.c

misc.o:
	$(CC) $(CFLAGS) -c misc.c

sys_linux.o:
	$(CC) $(CFLAGS) -c sys_linux.c

sys_solaris.o:
	$(CC) $(CFLAGS) -c sys_solaris.c

sys_hpux.o:
	$(CC) $(CFLAGS) -I/opt/audio/include -c sys_hpux.c

route.o:
	$(CC) $(CFLAGS) -c route.c

cut.o:
	$(CC) $(CFLAGS) -c cut.c

brl.o: text.auto.h attrib.auto.h cmds.auto.h
	$(CC) $(CFLAGS) -c brl.c

spk.o:
	$(CC) $(CFLAGS) -c spk.c

###############################################################################

BRLTEST_OBJECTS = brltest.o options.o misc.o $(SYS_OBJ) brl.o

brltest: $(BRLTEST_OBJECTS) $(BRAILLE_TARGETS)
	$(CXX) $(LDFLAGS) -o $@ $(BRLTEST_OBJECTS) $(BRAILLE_OBJECTS) $(LDLIBS)

brltest.o:
	$(CC) $(CFLAGS) -c brltest.c

###############################################################################

SCRTEST_OBJECTS = scrtest.o options.o misc.o route.o

scrtest: $(SCRTEST_OBJECTS) $(SCREEN_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(SCRTEST_OBJECTS) $(SCREEN_OBJECTS) $(LDLIBS)

scrtest.o:
	$(CC) $(CFLAGS) -c scrtest.c

###############################################################################

TUNETEST_OBJECTS = tunetest.o options.o misc.o $(SYS_OBJ)

tunetest: $(TUNETEST_OBJECTS) $(TUNE_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(TUNETEST_OBJECTS) $(TUNE_OBJECTS) $(LDLIBS)

tunetest.o:
	$(CC) $(CFLAGS) -c tunetest.c

###############################################################################

dynamic-braille: txt2hlp
	-rm -f $(DRV_DIR)/brltty-brl.lst
	for target in $(BRL_TARGETS); \
	do (cd ../Drivers/$$target && $(MAKE) braille-driver braille-help brl-lib-name) || exit 1; \
	done

static-braille: txt2hlp
	cd ../Drivers/$(BRL_TARGET) && $(MAKE) braille.o braille-help

###############################################################################

dynamic-speech:
	-rm -f $(DRV_DIR)/brltty-spk.lst
	for target in $(SPK_TARGETS); \
	do (cd ../Drivers/$$target && $(MAKE) speech-driver spk-lib-name) || exit 1; \
	done

static-speech:
	cd ../Drivers/$(SPK_TARGET) && $(MAKE) speech.o

###############################################################################

text.auto.h: $(TBL_DIR)/$(TEXTTRANS) tbl2hex
	./tbl2hex <$(TBL_DIR)/$(TEXTTRANS) >$@

attrib.auto.h: $(TBL_DIR)/$(ATTRTRANS) tbl2hex
	./tbl2hex <$(TBL_DIR)/$(ATTRTRANS) >$@

cmds.auto.h: brl.h cmds.awk
	$(AWK) -f cmds.awk brl.h >$@

###############################################################################

HOSTOPTS_OBJECTS = host_misc.o hostopts.o

hostopts.o:
	$(HOSTCC) $(HOSTCFLAGS) -o $@ -c options.c

host_misc.o:
	$(HOSTCC) $(HOSTCFLAGS) -o $@ -c misc.c

###############################################################################

TXT2HLP_OBJECTS = txt2hlp.o $(HOSTOPTS_OBJECTS)

txt2hlp: $(TXT2HLP_OBJECTS)
	$(HOSTCC) $(HOSTLDFLAGS) -o $@ $(TXT2HLP_OBJECTS) $(LDLIBS)

txt2hlp.o:
	$(HOSTCC) $(HOSTCFLAGS) -c txt2hlp.c

###############################################################################

TBL2HEX_OBJECTS = tbl2hex.o $(HOSTOPTS_OBJECTS)

tbl2hex: $(TBL2HEX_OBJECTS)
	$(HOSTCC) $(HOSTLDFLAGS) -o $@ $(TBL2HEX_OBJECTS) $(LDLIBS)

tbl2hex.o:
	$(HOSTCC) $(HOSTCFLAGS) -c tbl2hex.c

###############################################################################

install-brltty: install.template
	sed -e 's%=F%$(EXECUTE_ROOT)%g' \
	    -e 's%=P%$(PROG_DIR)%g' \
            -e 's%=L%$(LIB_DIR)%g' \
	    -e 's%=D%$(DATA_DIR)%g' \
            -e 's%=S%$(SCR_DEV)%g' \
            install.template >$@

###############################################################################

check-braille: brltty dynamic-braille
	for lib in $(BRL_LIBS); \
	do test $$lib = no && continue; \
	   ls -l $(DRV_DIR)/libbrlttyb$$lib.$(LIB_EXT) || exit 10; \
	   $(LIB_VAR)="$(DRV_DIR)" ./brltty -v -q -N -e -f /dev/null -b $$lib -s no 2>&1 || exit 11; \
	done

check-speech: brltty dynamic-speech
	for lib in $(SPK_LIBS); \
	do test $$lib = no && continue; \
	   ls -l $(DRV_DIR)/libbrlttys$$lib.$(LIB_EXT) || exit 10; \
	   $(LIB_VAR)="$(DRV_DIR)" ./brltty -v -q -N -e -f /dev/null -b no -s $$lib 2>&1 || exit 11; \
	done

###############################################################################

install: install-programs install-help install-tables $(INSTALL_DRIVERS) install-device-$(SCR_OBJ)

install-programs: brltty install-brltty
	$(INSTALL_DIRECTORY) $(INSTALL_ROOT)$(EXECUTE_ROOT)$(PROG_DIR) 
	$(INSTALL_PROGRAM) brltty $(INSTALL_ROOT)$(EXECUTE_ROOT)$(PROG_DIR) 
	$(INSTALL_SCRIPT) install-brltty $(INSTALL_ROOT)$(EXECUTE_ROOT)$(PROG_DIR) 

install-help: brltty
	$(INSTALL_DIRECTORY) $(INSTALL_ROOT)$(EXECUTE_ROOT)$(DATA_DIR) 
	$(INSTALL_DATA) $(HLP_DIR)/* $(INSTALL_ROOT)$(EXECUTE_ROOT)$(DATA_DIR) 

install-tables:
	$(INSTALL_DIRECTORY) $(INSTALL_ROOT)$(EXECUTE_ROOT)$(DATA_DIR) 
	$(INSTALL_DATA) $(TBL_DIR)/*.tbl $(INSTALL_ROOT)$(EXECUTE_ROOT)$(DATA_DIR)
	$(INSTALL_DATA) $(CTB_DIR)/*.ctb $(INSTALL_ROOT)$(EXECUTE_ROOT)$(DATA_DIR)

install-drivers: brltty
	$(INSTALL_DIRECTORY) $(INSTALL_ROOT)$(EXECUTE_ROOT)$(LIB_DIR)
	$(INSTALL) $(DRV_DIR)/* $(INSTALL_ROOT)$(EXECUTE_ROOT)$(LIB_DIR)

install-device-scr_linux.o:
	set -e; \
	for dev in $(SCR_DEV); do test -c $(INSTALL_ROOT)$$dev && exit 0; done; \
	set -- $(SCR_DEV); dev=$(INSTALL_ROOT)$$1; \
	$(INSTALL_DIRECTORY) $${dev%/*}; \
	mknod -m o= $$dev c 7 128; \
	chmod 620 $$dev; \
	chown root:tty $$dev

install-device-scr_shm.o:

uninstall:
	-rm -f $(INSTALL_ROOT)$(EXECUTE_ROOT)$(PROG_DIR)/brltty
	-rm -f $(INSTALL_ROOT)$(EXECUTE_ROOT)$(PROG_DIR)/install-brltty
	-rm -f $(INSTALL_ROOT)$(EXECUTE_ROOT)$(LIB_DIR)/$(LIB_NAME)*
	-rm -f $(INSTALL_ROOT)$(EXECUTE_ROOT)$(LIB_DIR)/brltty-*.lst
	-rmdir $(INSTALL_ROOT)$(EXECUTE_ROOT)$(LIB_DIR)
	-rm -f $(INSTALL_ROOT)$(EXECUTE_ROOT)$(DATA_DIR)/*.tbl
	-rm -f $(INSTALL_ROOT)$(EXECUTE_ROOT)$(DATA_DIR)/*.ctb
	-rm -f $(INSTALL_ROOT)$(EXECUTE_ROOT)$(DATA_DIR)/brltty*
	-rmdir $(INSTALL_ROOT)$(EXECUTE_ROOT)$(DATA_DIR)

###############################################################################

clean::
	-rm -f brltty install-brltty txt2hlp tbl2hex *test *-static
	-rm -f $(DRV_DIR)/* $(HLP_DIR)/*

distclean clean::
	-for driver in $(CONFIGURED_DRIVERS); do (cd ../Drivers/$$driver && $(MAKE) $@); done

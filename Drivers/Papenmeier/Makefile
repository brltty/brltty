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

###############################################################################
# Makefile for the Papenmeier Terminal
# This software is maintained by August Hörandl <august.hoerandl@gmx.at>
###############################################################################

DRIVER_CODE = pm
DRIVER_NAME = Papenmeier
BRAILLE_MODELS = Tiny, Compact, 2D Lite/Screen, EL2D 40/66/80, EL 40/80, Elba 20/32, IB 80
HELP_DEPS = $(HELP_TEXT)

include setup.mk
include $(TOP_DIR)/driver.mk

PM_CMD_SOURCES = $(PGM_DIR)/brl.h config.y
PM_CONFIG_FILE = brltty-$(DRIVER_CODE).conf

braille.o:
	$(CC) $(BRL_CFLAGS) $(YCFLAGS) -DREAD_CONFIG -c braille.c

read_config: read_config.o
	$(HOSTCC) $(HOSTLDFLAGS) -o $@ read_config.o

read_config.o: $(PGM_DIR)/cmds.auto.h
	$(HOSTCC) $(HOSTCFLAGS) $(YCFLAGS) -c read_config.c

config.tab.c: cmd.auto.h hlp.auto.h
	$(YACC) -b config -v config.y

cmd.auto.h: $(PM_CMD_SOURCES) cmd.awk
	$(AWK) -f cmd.awk $(PM_CMD_SOURCES) >cmd.auto.h

hlp.auto.h: $(PM_CMD_SOURCES) hlp.awk
	$(AWK) -f hlp.awk $(PM_CMD_SOURCES) >hlp.auto.h

$(PGM_DIR)/cmds.auto.h:
	cd $(@D) && make $(@F)

braille-help: $(HLP_DIR)/$(PM_CONFIG_FILE)

$(HLP_DIR)/$(PM_CONFIG_FILE): $(PM_CONFIG_FILE) $(TXT2HLP)
	for file in brltty-$(DRIVER_CODE)-*.hlp; do $(TXT2HLP) $(HLP_DIR)/$$file $$file; done
	cp $(PM_CONFIG_FILE) $(HLP_DIR)/

$(PM_CONFIG_FILE): read_config
	./read_config h
	./read_config d >$@

###############################################################################

dump-codes: dump-codes.o
	$(CC) $(LDFLAGS) -o $@ dump-codes.o

dump-codes.o:
	$(CC) $(CFLAGS) -c dump-codes.c

serial: serial.o
	$(CC) $(LDFLAGS) -o $@ serial.o -lncurses 

serial.o:
	$(CC) $(CFLAGS) -c serial.c

simulate: simulate.o
	$(CC) $(LDFLAGS) -o $@ simulate.o

simulate.o:
	$(CC) $(CFLAGS) -c simulate.c

###############################################################################

clean::
	-rm -f read_config config.tab.c config.output
	-rm -f brltty-$(DRIVER_CODE)-*.hlp $(PM_CONFIG_FILE)
	-rm -f serial simulate dump-codes

distclean::
	-rm -f *.good *.diff *.out

###############################################################################

TEST_CASES = test.1.in test.2.in

TEST_RESULT = $(TEST_CASES:.in=.out)
TEST_OK = $(TEST_CASES:.in=.good)
TEST_DIFF = $(TEST_CASES:.in=.diff)

dump: read_config
	read_config d

good: $(TEST_OK)
%.good: %.in read_config
	./read_config d $< <$< >$@

diff: $(TEST_DIFF)
%.diff: %.in good
	-diff -b $< $*.good >$@

out: $(TEST_RESULT)
%.out: %.in good read_config
	./read_config d $< <$< | ./read_config d $< | diff -b $*.good - >$@

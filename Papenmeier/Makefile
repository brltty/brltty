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
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS,
# and LDLIBS from the main Makefile
###############################################################################

DRIVER_CODE = pm
DRIVER_NAME = Papenmeier
BRAILLE_MODELS = Tiny, Compact, 2D Lite/Screen, EL2D 40/66/80, EL 40/80, Elba 20/32, IB 80
include ../driver.mk

PM_YACC_CFLAGS = -Wno-parentheses -Wno-unused

BRL_H = ../brl.h config.y

brl.o: brl.c brlconf.h ../brl_driver.h ../brl.h ../scr.h ../misc.h brl-cfg.h config.tab.c cmd.h hlp.h
	$(CC) $(BRL_CFLAGS) $(PM_YACC_CFLAGS) '-DHOME_DIR="$(PREFIX)$(DATA_DIR)"' -DREAD_CONFIG -o $@ -c $<

config.tab.c: config.y
	yacc -b config -v $<

read_config.o: read_config.c config.tab.c cmd.h hlp.h ../cmds.auto.h brl-cfg.h
	$(HOSTCC) $(HOSTCFLAGS) $(PM_YACC_CFLAGS) -o $@ -c $<

read_config: read_config.o
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $<

serial: serial.c ../brl.h brl.c ../text.auto.h ../misc.c config.tab.c brl-cfg.h cmd.h hlp.h
	$(HOSTCC) $(CFLAGS) serial.c -o serial -lncurses 

simulate: simulate.c ../brl.h brl.c ../text.auto.h ../misc.c config.tab.c brl-cfg.h cmd.h hlp.h
	$(HOSTCC) $(CFLAGS) simulate.c -o simulate 


cmd.h: $(BRL_H) cmd.awk
	awk >cmd.h -f cmd.awk $(BRL_H)

hlp.h: $(BRL_H) hlp.awk
	awk >hlp.h -f hlp.awk $(BRL_H)

ifneq (,$(findstring s,$(MAKEFLAGS)))
   READ_CONFIG_D = d
   READ_CONFIG_H = h
else
   READ_CONFIG_D = D
   READ_CONFIG_H = H
endif
brltty-$(DRIVER_CODE).conf: read_config
	./read_config $(READ_CONFIG_H)
	./read_config $(READ_CONFIG_D) >$@

braille-help: brltty-$(DRIVER_CODE).conf
	for target in brltty-$(DRIVER_CODE)*.hlp; \
	do ../txt2hlp $(HELPDIR)/$$target $$target; \
	done
	cp brltty-$(DRIVER_CODE).conf $(HELPDIR)/

distclean:
	rm -f serial simulate read_config dump-codes \
		config.tab.c config.output cmd.h hlp.h \
		brltty-$(DRIVER_CODE)*.hlp brltty-$(DRIVER_CODE).conf \
		*.good *.diff *.out

###############################################################
# 
# stuff related to read_config
#  used for tests only
#
# call whitch HOSTCC=... set - like
#  make HOSTCC="gcc" diff 
#

TEST_CASES := $(wildcard test*.in)

TEST_RESULT := $(patsubst %.in,%.out,$(TEST_CASES))
TEST_OK := $(patsubst %.in,%.good,$(TEST_CASES))
TEST_DIFF := $(patsubst %.in,%.diff,$(TEST_CASES))

test: read_config $(TEST_RESULT) 
	read_config D test > test.99.out
	read_config D test | read_config test | read_config test | read_config test | diff -b - test.99.out

diff: read_config $(TEST_DIFF)

good: $(TEST_OK) read_config

dump: read_config
	read_config 

%.out: %.in read_config
	read_config  $< < $<
	read_config  $< < $<  > $@
	read_config  $< < $< | read_config $< | diff -b - $@

%.diff: %.out read_config
	echo "diff: "
	-diff -b $*.good $<
	diff -b $*.good $< > $*.diff

%.good: %.in
	read_config $< < $<  > $@

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
# Makefile for the Papenmeier Terminal
# This software is maintained by August Hörandl <august.hoerandl@gmx.at>
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS,
# and LDLIBS from the main Makefile
###############################################################################

DRIVER_CODE = pm
DRIVER_NAME = Papenmeier
BRAILLE_MODELS = 
include ../driver.mk

PM_YACC_CFLAGS = -Wno-parentheses -Wno-unused

BRL_H = ../brl.h

brl.o: brl.c brlconf.h ../brl_driver.h ../brl.h ../scr.h ../misc.h brl-cfg.h config.tab.c cmd.h hlp.h
	$(CC) $(BRL_CFLAGS) $(PM_YACC_CFLAGS) -DREAD_CONFIG -o $@ -c $<

config.tab.c: config.y
	yacc -b config -v $<

read_config.o: read_config.c config.tab.c cmd.h hlp.h
	$(HOSTCC) $(HOSTCFLAGS) $(PM_YACC_CFLAGS) -o $@ -c $<

read_config: read_config.o
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $<

serial: serial.c ../brl.h brl.c ../text.auto.h ../misc.c
	$(CC) $(CFLAGS) serial.c -o serial -lncurses 

cmd.h: $(BRL_H)
	awk '/^ *CMD_/ { \
		print "{ \""$$1"\", KEYCODE, " $$1 "},";\
		print "{ \""substr($$1,5)"\", KEYCODE, " $$1 "},";\
	}' < $(BRL_H) > cmd.h

hlp.h:  $(BRL_H)
	awk '/^ *CMD_/  { OFS="";if (x=match($$0,"\/.*\/")) \
	  print "{ ",$$1,", \"#", substr($$0, RSTART+2, RLENGTH-4),"\" },"; \
	} \
	/#define STAT_/  { OFS="";if (x=match($$0,"\/.*\/")) \
	  print "{ OFFS_STAT +",$$2,", \"#", substr($$0, RSTART+2, RLENGTH-4),"\" },"; \
}'  < $(BRL_H) > hlp.h

ifneq (,$(findstring s,$(MAKEFLAGS)))
   READ_CONFIG_D = d
   READ_CONFIG_H = h
else
   READ_CONFIG_D = D
   READ_CONFIG_H = H
endif
brltty.pm.conf: read_config
	./read_config $(READ_CONFIG_H)
	./read_config $(READ_CONFIG_D) > brltty.pm.conf

braille-help: brltty.pm.conf
	for target in brltty-$(DRIVER_CODE)*.hlp; \
	do ../txt2hlp $(HELPDIR)/$$target $$target; \
	done
	cp brltty.pm.conf $(HELPDIR)/

distclean:
	rm -f serial read_config dump-codes \
		config.tab.c config.output cmd.h hlp.h \
		brltty-$(DRIVER_CODE)*.hlp brltty.pm.conf \
		*.good *.diff *.out

###############################################################
# 
# stuff related to read_config
#  used for tests only
#

TEST_CASES := $(wildcard test*.in)

TEST_RESULT := $(patsubst %.in,%.out,$(TEST_CASES))
TEST_OK := $(patsubst %.in,%.good,$(TEST_CASES))
TEST_DIFF := $(patsubst %.in,%.diff,$(TEST_CASES))

test: read_config $(TEST_RESULT) 
	read_config d > test.99.out
	read_config d | read_config | read_config | read_config | diff - test.99.out

diff: read_config $(TEST_DIFF)

good: $(TEST_OK) read_config

dump: read_config
	read_config d 

%.out: %.in read_config
	read_config $< < $<
	read_config $< < $<  > $@
	read_config $< < $< | read_config $< | diff - $@

%.diff: %.out read_config
	-diff $*.good $<
	-diff $*.good $< > $*.diff

%.good: %.in
	read_config $< < $<  > $@

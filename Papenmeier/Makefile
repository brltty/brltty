###############################################################################
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Nicolas Pitre <nico@cam.org>
# Stephane Doyon <doyons@jsp.umontreal.ca>
# Nikhil Nair <nn201@cus.cam.ac.uk>
#
# Copyright (C) 1995-2000 by the BRLTTY team.  All rights reserved.
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# This software is maintained by Nicolas Pitre <nico@cam.org>
###############################################################################

###############################################################################
# Makefile for the Papenmeier Terminal
# This software is maintained by August Hörandl <august.hoerandl@gmx.at>
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS,
# and LDLIBS from the main Makefile
###############################################################################

DRIVER = pm
TARGETS = brl.c brlconf.h brl-cfg.h \
	read_config.c cmd.h hlp.h \
	../brl_driver.h ../brl.h ../scr.h ../misc.h

include ../driver.mk

BRL_H = ../brl.h

brl.o: $(TARGETS)
	$(CC) $(BRL_CFLAGS) -DREAD_CONFIG -c brl.c

read_config.c: read_config.y cmd.h hlp.h brl-cfg.h $(BRL_H) ../misc.h ../misc.c

read_confg: read_config.c

serial: serial.c ../brl.h brl.c ../text.auto.h ../misc.c
	$(CC) $(CFLAGS) serial.c -o serial -lncurses 

brl-lib-name: $(HELPFILE1)
	echo -e "pm\tPapenmeier displays" >> $(BRLNAMES)

cmd.h: $(BRL_H)
	awk '/#define CMD_/ { \
		print "{ \""$$2"\", KEYCODE, " $$2 "},";\
		print "{ \""substr($$2,5)"\", KEYCODE, " $$2 "},";\
	}' < $(BRL_H) > cmd.h

hlp.h:  $(BRL_H)
	awk '/#define CMD_/  { OFS="";if (x=match($$0,"\/.*\/")) \
	  print "{ ",$$2,", \"#", substr($$0, RSTART+2, RLENGTH-4),"\" },"; \
	} \
	/#define STAT_/  { OFS="";if (x=match($$0,"\/.*\/")) \
	  print "{ OFFS_STAT +",$$2,", \"#", substr($$0, RSTART+2, RLENGTH-4),"\" },"; \
}'  < $(BRL_H) > hlp.h

brltty.pm.conf: read_config
	./read_config h
	./read_config d > brltty.pm.conf

braille-help: brltty.pm.conf
	for target in brltty-$(DRIVER)?.hlp; \
	do ../txt2hlp $(HELPDIR)/$$target $$target; \
	done
	cp brltty.pm.conf $(HELPDIR)/

distclean:
	rm -f serial readconfig *.good *.diff  *.out read_config.c \
		brltty-$(DRIVER)?.hlp brltty.pm.conf \
		cmd.h hlp.h read_config dump-codes

###############################################################
# 
# stuff related to read_config
#  used for tests only
#

TEST_CASES := $(wildcard test*.in)

TEST_RESULT := $(patsubst %.in,%.out,$(TEST_CASES))
TEST_OK := $(patsubst %.in,%.good,$(TEST_CASES))
TEST_DIFF := $(patsubst %.in,%.diff,$(TEST_CASES))

BRL_H = ../brl.h

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

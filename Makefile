###############################################################################
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Version 1.0, 26 July 1996
#
# Copyright (C) 1995, 1996 by Nikhil Nair and others.  All rights reserved.
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# This software is maintained by Nikhil Nair <nn201@cus.cam.ac.uk>.
###############################################################################

###############################################################################
# Makefile for BRLTTY
# N. Nair, 24 January 1996
###############################################################################

# Specify your Braille display by uncommenting one and ONLY one of these
# definitions of BRL_TARGET:
#BRL_TARGET = Alva_ABT3
#BRL_TARGET = CombiBraille
#BRL_TARGET = TSI

# Specify the device name for the serial port your Braille display will
# normally be connected to.  For port com(n), use /dev/ttyS(n-1) -
# e.g. for com1 use /dev/ttyS0
BRLDEV = /dev/ttyS0

# [Note that this port can be overridden from the brltty command-line.
# See the documentation for further details.]

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

# This is the name of the character device BRLTTY uses to read the screen.
# It is created during installation if it does not already exist.
# Don't edit this unless you know what you're doing!
SCRDEV = /dev/vcsa0

# This is purely experimental ...
SAY_CMD = NO_SAY_CMD
#SAY_CMD = SAY_CMD

# The following are compiler settings.  If you don't know what they mean,
# LEAVE THEM ALONE!
CC = gcc
# To compile in a.out (if you use ELF by default), you may be able to use
# `-b i486-linuxaout'; however, you may also need to use the -V flag, or
# possibly a different gcc executable, depending on your setup.
CFLAGS = -O2 -g -Wall -D_POSIX_C_SOURCE=2 -D$(BRL_TARGET) -D$(SAY_CMD)
UTILS_CFLAGS = -O2 -g -Wall -D_POSIX_C_SOURCE=2
LD = $(CC)
LDFLAGS =
#LDFLAGS = -static
LDLIBS =
PREFIX =

# ------------------------ DO NOT EDIT BELOW THIS LINE ------------------------

all: brltty install-brltty txt2scrn brltest scrtest tbl2txt txt2tbl convtable

install: brltty txt2scrn install-brltty
	install $(INSTALL_EXEC) --strip brltty $(PREFIX)$(EXEC_PATH)
	install $(INSTALL_EXEC) install-brltty $(PREFIX)$(REINSTALL_PATH)
	install --directory $(PREFIX)$(DATA_DIR)
	install -m 644 *.tbl $(PREFIX)$(DATA_DIR)
	find $(BRL_TARGET) -name '*.dat' -exec install -m 644 {} \
	  $(PREFIX)$(DATA_DIR) \;
	./txt2scrn $(PREFIX)$(DATA_DIR)/brlttyhelp.scr \
	  $(BRL_TARGET)/brlttyh*.txt
	if [ ! -c $(PREFIX)$(SCRDEV) ]; \
	then \
	  mknod -m o= $(PREFIX)$(SCRDEV) c 7 128; \
	  chown root.tty $(PREFIX)$(SCRDEV); \
	  chmod 660 $(PREFIX)$(SCRDEV); \
	fi

uninstall:
	rm -f $(PREFIX)$(EXEC_PATH) $(PREFIX)$(REINSTALL_PATH)
	rm -rf $(PREFIX)$(DATA_DIR)

install-brltty: install.template
	sed -e 's%=E%$(EXEC_PATH)%g' -e 's%=I%$(REINSTALL_PATH)%g' \
	  -e 's%=D%$(DATA_DIR)%g' -e 's%=V%$(SCRDEV)%g' install.template > $@

brltty: brltty.o $(BRL_TARGET)/brl.o scr.o misc.o beeps.o cut-n-paste.o
	$(LD) $(LDFLAGS) -o $@ brltty.o $(BRL_TARGET)/brl.o \
	  scr.o misc.o beeps.o cut-n-paste.o $(LDLIBS)

brltest: brltest.o $(BRL_TARGET)/brl.o misc.o
	$(LD) $(LDFLAGS) -o $@ brltest.o $(BRL_TARGET)/brl.o misc.o $(LDLIBS)
	strip $@

scrtest: scrtest.o scr.o
	$(LD) $(LDFLAGS) -o $@ scrtest.o scr.o $(LDLIBS)
	strip $@

$(BRL_TARGET)/brl.o: $(BRL_TARGET)/brl.c $(BRL_TARGET)/brlconf.h brl.h misc.h
	cd $(BRL_TARGET); \
	$(CC) $(CFLAGS) '-DBRLDEV="$(BRLDEV)"' -c brl.c

scr.o: scr.c scr.h config.h
	$(CC) $(CFLAGS) '-DSCRDEV="$(SCRDEV)"' -c scr.c

misc.o: misc.c misc.h
	$(CC) $(CFLAGS) -c misc.c

brltty.o: brltty.c brl.h scr.h misc.h config.h
	$(CC) $(CFLAGS) '-DHOME_DIR="$(DATA_DIR)"' -c brltty.c

beeps.o: beeps.c beeps.h beeps-songs.h
	$(CC) $(CFLAGS) -c beeps.c

cut-n-paste.o: cut-n-paste.c cut-n-paste.h beeps.h scr.h
	$(CC) $(CFLAGS) -c cut-n-paste.c

scrtest.o: scrtest.c 
	$(CC) $(CFLAGS) -c scrtest.c

brltest.o: brltest.c brl.h config.h
	$(CC) $(CFLAGS) '-DHOME_DIR="$(DATA_DIR)"' -c brltest.c

txt2scrn: txt2scrn.o
	$(LD) $(LDFLAGS) -o $@ txt2scrn.o $(LDLIBS)

txt2scrn.o: txt2scrn.c scr.h
	$(CC) $(UTILS_CFLAGS) -c txt2scrn.c

tbl2txt: tbl2txt.o
	$(LD) $(LDFLAGS) -o $@ tbl2txt.o $(LDLIBS)
	strip $@

tbl2txt.o: tbl2txt.c
	$(CC) $(UTILS_CFLAGS) -c tbl2txt.c

txt2tbl: txt2tbl.o
	$(LD) $(LDFLAGS) -o $@ txt2tbl.o $(LDLIBS)
	strip $@

txt2tbl.o: txt2tbl.c
	$(CC) $(UTILS_CFLAGS) -c txt2tbl.c

convtable: convtable.o
	$(LD) $(LDFLAGS) -o $@ convtable.o $(LDLIBS)
	strip $@

convtable.o: convtable.c
	$(CC) $(UTILS_CFLAGS) -c convtable.c

clean:
	rm -f *.o */*.o install-brltty

distclean: clean
	rm -f brltty txt2scrn *test tbl2txt txt2tbl convtable
	rm -f *~ */*~ *orig */*orig \#*\# */\#*\#

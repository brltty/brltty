# Dependencies for braille.o:
braille.o: braille.c
braille.o: $(TOP_DIR)/config.h
braille.o: brlconf.h
braille.o: $(TOP_DIR)/Programs/brl.h
braille.o: $(TOP_DIR)/Programs/scr.h
braille.o: $(TOP_DIR)/Programs/misc.h
braille.o: $(TOP_DIR)/Programs/brl_driver.h
braille.o: config.tab.c
braille.o: brl-cfg.h

# Dependencies for config.tab.c:
config.tab.c: config.y
config.tab.c: $(TOP_DIR)/config.h
config.tab.c: $(TOP_DIR)/Programs/brl.h
config.tab.c: brl-cfg.h

# Dependencies for dump-codes.o:
dump-codes.o: dump-codes.c
dump-codes.o: $(TOP_DIR)/config.h

# Dependencies for read_config.o:
read_config.o: read_config.c
read_config.o: config.tab.c

# Dependencies for serial.o:
serial.o: serial.c
serial.o: braille.c
serial.o: brl-cfg.h
serial.o: brlconf.h
serial.o: config.tab.c
serial.o: $(TOP_DIR)/Programs/brl.h
serial.o: $(TOP_DIR)/Programs/brl_driver.h
serial.o: $(TOP_DIR)/Programs/misc.h
serial.o: $(TOP_DIR)/Programs/scr.h
serial.o: $(TOP_DIR)/config.h
serial.o: $(TOP_DIR)/Programs/misc.c

# Dependencies for simulate.o:
simulate.o: simulate.c
simulate.o: braille.c
simulate.o: brl-cfg.h
simulate.o: brlconf.h
simulate.o: config.tab.c
simulate.o: $(TOP_DIR)/Programs/brl.h
simulate.o: $(TOP_DIR)/Programs/brl_driver.h
simulate.o: $(TOP_DIR)/Programs/misc.h
simulate.o: $(TOP_DIR)/Programs/scr.h
simulate.o: $(TOP_DIR)/config.h
simulate.o: $(TOP_DIR)/Programs/misc.c


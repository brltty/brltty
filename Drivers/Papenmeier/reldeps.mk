# Dependencies for braille.o:
braille.o: $(SRC_DIR)/braille.c
braille.o: $(BLD_TOP)config.h
braille.o: $(SRC_DIR)/brlconf.h
braille.o: $(SRC_TOP)Programs/brl.h
braille.o: $(SRC_TOP)Programs/misc.h
braille.o: $(SRC_TOP)Programs/brl_driver.h
braille.o: $(BLD_DIR)/config.tab.c
braille.o: $(SRC_DIR)/brl-cfg.h

# Dependencies for config.tab.c:
config.tab.c: $(SRC_DIR)/config.y
config.tab.c: $(BLD_TOP)config.h
config.tab.c: $(SRC_DIR)/brl-cfg.h
config.tab.c: $(SRC_TOP)Programs/brl.h

# Dependencies for dump-codes.o:
dump-codes.o: $(SRC_DIR)/dump-codes.c
dump-codes.o: $(BLD_TOP)config.h

# Dependencies for read_config.o:
read_config.o: $(SRC_DIR)/read_config.c
read_config.o: $(BLD_DIR)/config.tab.c

# Dependencies for serial.o:
serial.o: $(SRC_DIR)/serial.c
serial.o: $(SRC_DIR)/braille.c
serial.o: $(SRC_DIR)/brl-cfg.h
serial.o: $(SRC_DIR)/brlconf.h
serial.o: $(BLD_DIR)/config.tab.c
serial.o: $(SRC_TOP)Programs/brl.h
serial.o: $(SRC_TOP)Programs/brl_driver.h
serial.o: $(SRC_TOP)Programs/misc.h
serial.o: $(BLD_TOP)config.h
serial.o: $(SRC_TOP)Programs/misc.c

# Dependencies for simulate.o:
simulate.o: $(SRC_DIR)/simulate.c
simulate.o: $(SRC_DIR)/braille.c
simulate.o: $(SRC_DIR)/brl-cfg.h
simulate.o: $(SRC_DIR)/brlconf.h
simulate.o: $(BLD_DIR)/config.tab.c
simulate.o: $(SRC_TOP)Programs/brl.h
simulate.o: $(SRC_TOP)Programs/brl_driver.h
simulate.o: $(SRC_TOP)Programs/misc.h
simulate.o: $(BLD_TOP)config.h
simulate.o: $(SRC_TOP)Programs/misc.c


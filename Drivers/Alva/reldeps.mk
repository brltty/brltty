# Dependencies for braille.o:
braille.o: $(SRC_DIR)/braille.c
braille.o: $(BLD_TOP)config.h
braille.o: $(SRC_DIR)/brlconf.h
braille.o: $(SRC_TOP)Programs/misc.h
braille.o: $(SRC_TOP)Programs/brl.h
braille.o: $(SRC_TOP)Programs/brltty.h
braille.o: $(SRC_TOP)Programs/brl_driver.h

# Dependencies for speech.o:
speech.o: $(SRC_DIR)/speech.c
speech.o: $(BLD_TOP)config.h
speech.o: $(SRC_DIR)/brlconf.h
speech.o: $(SRC_DIR)/speech.h
speech.o: $(SRC_TOP)Programs/spk.h
speech.o: $(SRC_TOP)Programs/spk_driver.h
speech.o: $(SRC_TOP)Programs/misc.h


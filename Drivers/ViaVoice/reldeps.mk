# Dependencies for speech.o:
speech.o: $(SRC_DIR)/speech.c
speech.o: $(BLD_TOP)config.h
speech.o: $(SRC_DIR)/speech.h
speech.o: $(SRC_TOP)Programs/spk.h
speech.o: $(SRC_TOP)Programs/misc.h
speech.o: $(SRC_TOP)Programs/spk_driver.h


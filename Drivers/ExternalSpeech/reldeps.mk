# Dependencies for speech.o:
speech.o: speech.c
speech.o: $(TOP_DIR)/config.h
speech.o: $(TOP_DIR)/Programs/misc.h
speech.o: speech.h
speech.o: $(TOP_DIR)/Programs/spk.h
speech.o: $(TOP_DIR)/Programs/spk_driver.h


# Dependencies for braille.o:
braille.o: braille.c
braille.o: $(TOP_DIR)/config.h
braille.o: $(TOP_DIR)/Programs/brl.h
braille.o: $(TOP_DIR)/Programs/misc.h
braille.o: $(TOP_DIR)/Programs/message.h
braille.o: $(TOP_DIR)/Programs/scr.h
braille.o: brlconf.h
braille.o: $(TOP_DIR)/Programs/brl_driver.h
braille.o: bindings.h

# Dependencies for speech.o:
speech.o: speech.c
speech.o: $(TOP_DIR)/config.h
speech.o: speech.h
speech.o: $(TOP_DIR)/Programs/spk.h
speech.o: $(TOP_DIR)/Programs/spk_driver.h
speech.o: $(TOP_DIR)/Programs/misc.h


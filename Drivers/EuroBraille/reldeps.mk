# Dependencies for braille.o:
braille.o: braille.c
braille.o: $(TOP_DIR)/config.h
braille.o: brlconf.h
braille.o: $(TOP_DIR)/Programs/brl.h
braille.o: $(TOP_DIR)/Programs/message.h
braille.o: $(TOP_DIR)/Programs/scr.h
braille.o: $(TOP_DIR)/Programs/misc.h
braille.o: $(TOP_DIR)/Programs/brl_driver.h


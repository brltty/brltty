# Dependencies for vario.o:
vario.o: vario.c
vario.o: $(TOP_DIR)/config.h
vario.o: variolow.h
vario.o: $(TOP_DIR)/Programs/brl.h
vario.o: brlconf.h
vario.o: $(TOP_DIR)/Programs/brl_driver.h
vario.o: $(TOP_DIR)/Programs/misc.h

# Dependencies for variolow.o:
variolow.o: variolow.c
variolow.o: $(TOP_DIR)/config.h
variolow.o: variolow.h
variolow.o: $(TOP_DIR)/Programs/misc.h


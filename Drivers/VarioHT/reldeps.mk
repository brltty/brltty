# Dependencies for vario.o:
vario.o: $(SRC_DIR)/vario.c
vario.o: $(BLD_TOP)config.h
vario.o: $(SRC_DIR)/variolow.h
vario.o: $(SRC_TOP)Programs/brl.h
vario.o: $(SRC_TOP)Programs/brldefs.h
vario.o: $(SRC_DIR)/brlconf.h
vario.o: $(SRC_TOP)Programs/brl_driver.h
vario.o: $(SRC_TOP)Programs/misc.h

# Dependencies for variolow.o:
variolow.o: $(SRC_DIR)/variolow.c
variolow.o: $(BLD_TOP)config.h
variolow.o: $(SRC_DIR)/variolow.h
variolow.o: $(SRC_TOP)Programs/misc.h


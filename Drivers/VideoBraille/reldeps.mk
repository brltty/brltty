# Dependencies for vb.o:
vb.o: $(SRC_DIR)/vb.c
vb.o: $(BLD_TOP)config.h
vb.o: $(SRC_DIR)/vblow.h
vb.o: $(SRC_TOP)Programs/brl.h
vb.o: $(SRC_TOP)Programs/brldefs.h
vb.o: $(SRC_DIR)/brlconf.h
vb.o: $(SRC_TOP)Programs/brl_driver.h
vb.o: $(SRC_TOP)Programs/misc.h

# Dependencies for vblow.o:
vblow.o: $(SRC_DIR)/vblow.c
vblow.o: $(BLD_TOP)config.h
vblow.o: $(SRC_DIR)/brlconf.h
vblow.o: $(SRC_DIR)/vblow.h
vblow.o: $(SRC_TOP)Programs/misc.h
vblow.o: $(SRC_TOP)Programs/system.h


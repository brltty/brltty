# Dependencies for vb.o:
vb.o: vb.c
vb.o: $(TOP_DIR)/config.h
vb.o: vblow.h
vb.o: $(TOP_DIR)/Programs/brl.h
vb.o: brlconf.h
vb.o: $(TOP_DIR)/Programs/brl_driver.h
vb.o: $(TOP_DIR)/Programs/misc.h

# Dependencies for vblow.o:
vblow.o: vblow.c
vblow.o: $(TOP_DIR)/config.h
vblow.o: brlconf.h
vblow.o: vblow.h
vblow.o: $(TOP_DIR)/Programs/misc.h
vblow.o: $(TOP_DIR)/Programs/system.h


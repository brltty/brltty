# Dependencies for brlvger.o:
brlvger.o: $(SRC_DIR)/brlvger.c

# Dependencies for vgertest.o:
vgertest.o: $(SRC_DIR)/vgertest.c
vgertest.o: $(SRC_TOP)Drivers/Voyager/kernel/linux/brlvger.h


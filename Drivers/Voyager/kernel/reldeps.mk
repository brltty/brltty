# Dependencies for brlvger.o:
brlvger.o: brlvger.c

# Dependencies for vgertest.o:
vgertest.o: vgertest.c
vgertest.o: $(TOP_DIR)/Drivers/Voyager/kernel/linux/brlvger.h


# Dependencies for ibm437toiso.o:
ibm437toiso.o: $(SRC_DIR)/ibm437toiso.c
ibm437toiso.o: $(BLD_TOP)config.h

# Dependencies for ibm850toiso.o:
ibm850toiso.o: $(SRC_DIR)/ibm850toiso.c
ibm850toiso.o: $(BLD_TOP)config.h

# Dependencies for isotoibm437.o:
isotoibm437.o: $(SRC_DIR)/isotoibm437.c
isotoibm437.o: $(BLD_TOP)config.h

# Dependencies for tbl2jbt.o:
tbl2jbt.o: $(SRC_DIR)/tbl2jbt.c
tbl2jbt.o: $(BLD_TOP)config.h
tbl2jbt.o: $(SRC_TOP)Programs/options.h
tbl2jbt.o: $(SRC_TOP)Programs/brl.h
tbl2jbt.o: $(SRC_TOP)Programs/brldefs.h

# Dependencies for tbl2tbl.o:
tbl2tbl.o: $(SRC_DIR)/tbl2tbl.c
tbl2tbl.o: $(BLD_TOP)config.h
tbl2tbl.o: $(SRC_TOP)Programs/options.h

# Dependencies for tbl2txt.o:
tbl2txt.o: $(SRC_DIR)/tbl2txt.c
tbl2txt.o: $(BLD_TOP)config.h
tbl2txt.o: $(SRC_TOP)Programs/options.h
tbl2txt.o: $(SRC_TOP)Programs/brl.h
tbl2txt.o: $(SRC_TOP)Programs/brldefs.h
tbl2txt.o: $(SRC_TOP)Unicode/unicode.h

# Dependencies for txt2tbl.o:
txt2tbl.o: $(SRC_DIR)/txt2tbl.c
txt2tbl.o: $(BLD_TOP)config.h
txt2tbl.o: $(SRC_TOP)Programs/options.h
txt2tbl.o: $(SRC_TOP)Programs/brl.h
txt2tbl.o: $(SRC_TOP)Programs/brldefs.h


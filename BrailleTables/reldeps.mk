# Dependencies for ibm437toiso.o:
ibm437toiso.o: ibm437toiso.c
ibm437toiso.o: $(TOP_DIR)/config.h

# Dependencies for ibm850toiso.o:
ibm850toiso.o: ibm850toiso.c
ibm850toiso.o: $(TOP_DIR)/config.h

# Dependencies for isotoibm437.o:
isotoibm437.o: isotoibm437.c
isotoibm437.o: $(TOP_DIR)/config.h

# Dependencies for tbl2jbt.o:
tbl2jbt.o: tbl2jbt.c
tbl2jbt.o: $(TOP_DIR)/config.h
tbl2jbt.o: $(TOP_DIR)/Programs/options.h
tbl2jbt.o: $(TOP_DIR)/Programs/brl.h

# Dependencies for tbl2tbl.o:
tbl2tbl.o: tbl2tbl.c
tbl2tbl.o: $(TOP_DIR)/config.h
tbl2tbl.o: $(TOP_DIR)/Programs/options.h

# Dependencies for tbl2txt.o:
tbl2txt.o: tbl2txt.c
tbl2txt.o: $(TOP_DIR)/config.h
tbl2txt.o: $(TOP_DIR)/Programs/options.h
tbl2txt.o: $(TOP_DIR)/Programs/brl.h
tbl2txt.o: $(TOP_DIR)/Unicode/unicode.h

# Dependencies for txt2tbl.o:
txt2tbl.o: txt2tbl.c
txt2tbl.o: $(TOP_DIR)/config.h
txt2tbl.o: $(TOP_DIR)/Programs/options.h
txt2tbl.o: $(TOP_DIR)/Programs/brl.h


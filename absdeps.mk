# Dependencies for BrailleTables/ibm437toiso.o:
$(BLD_TOP)BrailleTables/ibm437toiso.o: $(SRC_TOP)BrailleTables/ibm437toiso.c
$(BLD_TOP)BrailleTables/ibm437toiso.o: $(BLD_TOP)config.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for BrailleTables/ibm850toiso.o:
$(BLD_TOP)BrailleTables/ibm850toiso.o: $(SRC_TOP)BrailleTables/ibm850toiso.c
$(BLD_TOP)BrailleTables/ibm850toiso.o: $(BLD_TOP)config.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for BrailleTables/isotoibm437.o:
$(BLD_TOP)BrailleTables/isotoibm437.o: $(SRC_TOP)BrailleTables/isotoibm437.c
$(BLD_TOP)BrailleTables/isotoibm437.o: $(BLD_TOP)config.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for BrailleTables/tbl2jbt.o:
$(BLD_TOP)BrailleTables/tbl2jbt.o: $(SRC_TOP)BrailleTables/tbl2jbt.c
$(BLD_TOP)BrailleTables/tbl2jbt.o: $(BLD_TOP)config.h
$(BLD_TOP)BrailleTables/tbl2jbt.o: $(SRC_TOP)Programs/options.h
$(BLD_TOP)BrailleTables/tbl2jbt.o: $(SRC_TOP)Programs/brl.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for BrailleTables/tbl2tbl.o:
$(BLD_TOP)BrailleTables/tbl2tbl.o: $(SRC_TOP)BrailleTables/tbl2tbl.c
$(BLD_TOP)BrailleTables/tbl2tbl.o: $(BLD_TOP)config.h
$(BLD_TOP)BrailleTables/tbl2tbl.o: $(SRC_TOP)Programs/options.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for BrailleTables/tbl2txt.o:
$(BLD_TOP)BrailleTables/tbl2txt.o: $(SRC_TOP)BrailleTables/tbl2txt.c
$(BLD_TOP)BrailleTables/tbl2txt.o: $(BLD_TOP)config.h
$(BLD_TOP)BrailleTables/tbl2txt.o: $(SRC_TOP)Programs/options.h
$(BLD_TOP)BrailleTables/tbl2txt.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)BrailleTables/tbl2txt.o: $(SRC_TOP)Unicode/unicode.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for BrailleTables/txt2tbl.o:
$(BLD_TOP)BrailleTables/txt2tbl.o: $(SRC_TOP)BrailleTables/txt2tbl.c
$(BLD_TOP)BrailleTables/txt2tbl.o: $(BLD_TOP)config.h
$(BLD_TOP)BrailleTables/txt2tbl.o: $(SRC_TOP)Programs/options.h
$(BLD_TOP)BrailleTables/txt2tbl.o: $(SRC_TOP)Programs/brl.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Alva/braille.o:
$(BLD_TOP)Drivers/Alva/braille.o: $(SRC_TOP)Drivers/Alva/braille.c
$(BLD_TOP)Drivers/Alva/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/Alva/braille.o: $(SRC_TOP)Drivers/Alva/brlconf.h
$(BLD_TOP)Drivers/Alva/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/Alva/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/Alva/braille.o: $(SRC_TOP)Programs/brltty.h
$(BLD_TOP)Drivers/Alva/braille.o: $(SRC_TOP)Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Alva/speech.o:
$(BLD_TOP)Drivers/Alva/speech.o: $(SRC_TOP)Drivers/Alva/speech.c
$(BLD_TOP)Drivers/Alva/speech.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/Alva/speech.o: $(SRC_TOP)Drivers/Alva/brlconf.h
$(BLD_TOP)Drivers/Alva/speech.o: $(SRC_TOP)Drivers/Alva/speech.h
$(BLD_TOP)Drivers/Alva/speech.o: $(SRC_TOP)Programs/spk.h
$(BLD_TOP)Drivers/Alva/speech.o: $(SRC_TOP)Programs/spk_driver.h
$(BLD_TOP)Drivers/Alva/speech.o: $(SRC_TOP)Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/BrailleLite/braille.o:
$(BLD_TOP)Drivers/BrailleLite/braille.o: $(SRC_TOP)Drivers/BrailleLite/braille.c
$(BLD_TOP)Drivers/BrailleLite/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/BrailleLite/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/BrailleLite/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/BrailleLite/braille.o: $(SRC_TOP)Programs/message.h
$(BLD_TOP)Drivers/BrailleLite/braille.o: $(SRC_TOP)Drivers/BrailleLite/brlconf.h
$(BLD_TOP)Drivers/BrailleLite/braille.o: $(SRC_TOP)Programs/brl_driver.h
$(BLD_TOP)Drivers/BrailleLite/braille.o: $(SRC_TOP)Drivers/BrailleLite/bindings.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/BrailleLite/speech.o:
$(BLD_TOP)Drivers/BrailleLite/speech.o: $(SRC_TOP)Drivers/BrailleLite/speech.c
$(BLD_TOP)Drivers/BrailleLite/speech.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/BrailleLite/speech.o: $(SRC_TOP)Drivers/BrailleLite/speech.h
$(BLD_TOP)Drivers/BrailleLite/speech.o: $(SRC_TOP)Programs/spk.h
$(BLD_TOP)Drivers/BrailleLite/speech.o: $(SRC_TOP)Programs/spk_driver.h
$(BLD_TOP)Drivers/BrailleLite/speech.o: $(SRC_TOP)Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/BrailleNote/braille.o:
$(BLD_TOP)Drivers/BrailleNote/braille.o: $(SRC_TOP)Drivers/BrailleNote/braille.c
$(BLD_TOP)Drivers/BrailleNote/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/BrailleNote/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/BrailleNote/braille.o: $(SRC_TOP)Drivers/BrailleNote/brlconf.h
$(BLD_TOP)Drivers/BrailleNote/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/BrailleNote/braille.o: $(SRC_TOP)Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/CombiBraille/braille.o:
$(BLD_TOP)Drivers/CombiBraille/braille.o: $(SRC_TOP)Drivers/CombiBraille/braille.c
$(BLD_TOP)Drivers/CombiBraille/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/CombiBraille/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/CombiBraille/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/CombiBraille/braille.o: $(SRC_TOP)Drivers/CombiBraille/brlconf.h
$(BLD_TOP)Drivers/CombiBraille/braille.o: $(SRC_TOP)Programs/brl_driver.h
$(BLD_TOP)Drivers/CombiBraille/braille.o: $(SRC_TOP)Drivers/CombiBraille/cmdtrans.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/CombiBraille/speech.o:
$(BLD_TOP)Drivers/CombiBraille/speech.o: $(SRC_TOP)Drivers/CombiBraille/speech.c
$(BLD_TOP)Drivers/CombiBraille/speech.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/CombiBraille/speech.o: $(SRC_TOP)Drivers/CombiBraille/brlconf.h
$(BLD_TOP)Drivers/CombiBraille/speech.o: $(SRC_TOP)Drivers/CombiBraille/speech.h
$(BLD_TOP)Drivers/CombiBraille/speech.o: $(SRC_TOP)Programs/spk.h
$(BLD_TOP)Drivers/CombiBraille/speech.o: $(SRC_TOP)Programs/spk_driver.h
$(BLD_TOP)Drivers/CombiBraille/speech.o: $(SRC_TOP)Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/EcoBraille/braille.o:
$(BLD_TOP)Drivers/EcoBraille/braille.o: $(SRC_TOP)Drivers/EcoBraille/braille.c
$(BLD_TOP)Drivers/EcoBraille/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/EcoBraille/braille.o: $(SRC_TOP)Drivers/EcoBraille/brlconf.h
$(BLD_TOP)Drivers/EcoBraille/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/EcoBraille/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/EcoBraille/braille.o: $(SRC_TOP)Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/EuroBraille/braille.o:
$(BLD_TOP)Drivers/EuroBraille/braille.o: $(SRC_TOP)Drivers/EuroBraille/braille.c
$(BLD_TOP)Drivers/EuroBraille/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/EuroBraille/braille.o: $(SRC_TOP)Drivers/EuroBraille/brlconf.h
$(BLD_TOP)Drivers/EuroBraille/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/EuroBraille/braille.o: $(SRC_TOP)Programs/message.h
$(BLD_TOP)Drivers/EuroBraille/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/EuroBraille/braille.o: $(SRC_TOP)Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/ExternalSpeech/speech.o:
$(BLD_TOP)Drivers/ExternalSpeech/speech.o: $(SRC_TOP)Drivers/ExternalSpeech/speech.c
$(BLD_TOP)Drivers/ExternalSpeech/speech.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/ExternalSpeech/speech.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/ExternalSpeech/speech.o: $(SRC_TOP)Drivers/ExternalSpeech/speech.h
$(BLD_TOP)Drivers/ExternalSpeech/speech.o: $(SRC_TOP)Programs/spk.h
$(BLD_TOP)Drivers/ExternalSpeech/speech.o: $(SRC_TOP)Programs/spk_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Festival/speech.o:
$(BLD_TOP)Drivers/Festival/speech.o: $(SRC_TOP)Drivers/Festival/speech.c
$(BLD_TOP)Drivers/Festival/speech.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/Festival/speech.o: $(SRC_TOP)Drivers/Festival/speech.h
$(BLD_TOP)Drivers/Festival/speech.o: $(SRC_TOP)Programs/spk.h
$(BLD_TOP)Drivers/Festival/speech.o: $(SRC_TOP)Programs/spk_driver.h
$(BLD_TOP)Drivers/Festival/speech.o: $(SRC_TOP)Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/GenericSay/speech.o:
$(BLD_TOP)Drivers/GenericSay/speech.o: $(SRC_TOP)Drivers/GenericSay/speech.c
$(BLD_TOP)Drivers/GenericSay/speech.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/GenericSay/speech.o: $(SRC_TOP)Drivers/GenericSay/speech.h
$(BLD_TOP)Drivers/GenericSay/speech.o: $(SRC_TOP)Programs/spk.h
$(BLD_TOP)Drivers/GenericSay/speech.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/GenericSay/speech.o: $(SRC_TOP)Programs/spk_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/HandyTech/braille.o:
$(BLD_TOP)Drivers/HandyTech/braille.o: $(SRC_TOP)Drivers/HandyTech/braille.c
$(BLD_TOP)Drivers/HandyTech/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/HandyTech/braille.o: $(SRC_TOP)Drivers/HandyTech/brlconf.h
$(BLD_TOP)Drivers/HandyTech/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/HandyTech/braille.o: $(SRC_TOP)Programs/brltty.h
$(BLD_TOP)Drivers/HandyTech/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/HandyTech/braille.o: $(SRC_TOP)Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/LogText/braille.o:
$(BLD_TOP)Drivers/LogText/braille.o: $(SRC_TOP)Drivers/LogText/braille.c
$(BLD_TOP)Drivers/LogText/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/LogText/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/LogText/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/LogText/braille.o: $(SRC_TOP)Drivers/LogText/brlconf.h
$(BLD_TOP)Drivers/LogText/braille.o: $(SRC_TOP)Programs/brl_driver.h
$(BLD_TOP)Drivers/LogText/braille.o: $(SRC_TOP)Drivers/LogText/input.h
$(BLD_TOP)Drivers/LogText/braille.o: $(SRC_TOP)Drivers/LogText/output.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/MDV/braille.o:
$(BLD_TOP)Drivers/MDV/braille.o: $(SRC_TOP)Drivers/MDV/braille.c
$(BLD_TOP)Drivers/MDV/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/MDV/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/MDV/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/MDV/braille.o: $(SRC_TOP)Drivers/MDV/brlconf.h
$(BLD_TOP)Drivers/MDV/braille.o: $(SRC_TOP)Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/MiniBraille/braille.o:
$(BLD_TOP)Drivers/MiniBraille/braille.o: $(SRC_TOP)Drivers/MiniBraille/braille.c
$(BLD_TOP)Drivers/MiniBraille/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/MiniBraille/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/MiniBraille/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/MiniBraille/braille.o: $(SRC_TOP)Drivers/MiniBraille/brlconf.h
$(BLD_TOP)Drivers/MiniBraille/braille.o: $(SRC_TOP)Programs/brl_driver.h
$(BLD_TOP)Drivers/MiniBraille/braille.o: $(SRC_TOP)Programs/message.h
$(BLD_TOP)Drivers/MiniBraille/braille.o: $(SRC_TOP)Drivers/MiniBraille/minibraille.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/MultiBraille/braille.o:
$(BLD_TOP)Drivers/MultiBraille/braille.o: $(SRC_TOP)Drivers/MultiBraille/braille.c
$(BLD_TOP)Drivers/MultiBraille/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/MultiBraille/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/MultiBraille/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/MultiBraille/braille.o: $(SRC_TOP)Drivers/MultiBraille/brlconf.h
$(BLD_TOP)Drivers/MultiBraille/braille.o: $(SRC_TOP)Programs/brl_driver.h
$(BLD_TOP)Drivers/MultiBraille/braille.o: $(SRC_TOP)Drivers/MultiBraille/tables.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/MultiBraille/speech.o:
$(BLD_TOP)Drivers/MultiBraille/speech.o: $(SRC_TOP)Drivers/MultiBraille/speech.c
$(BLD_TOP)Drivers/MultiBraille/speech.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/MultiBraille/speech.o: $(SRC_TOP)Drivers/MultiBraille/brlconf.h
$(BLD_TOP)Drivers/MultiBraille/speech.o: $(SRC_TOP)Drivers/MultiBraille/speech.h
$(BLD_TOP)Drivers/MultiBraille/speech.o: $(SRC_TOP)Programs/spk.h
$(BLD_TOP)Drivers/MultiBraille/speech.o: $(SRC_TOP)Programs/spk_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Papenmeier/braille.o:
$(BLD_TOP)Drivers/Papenmeier/braille.o: $(SRC_TOP)Drivers/Papenmeier/braille.c
$(BLD_TOP)Drivers/Papenmeier/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/Papenmeier/braille.o: $(SRC_TOP)Drivers/Papenmeier/brlconf.h
$(BLD_TOP)Drivers/Papenmeier/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/Papenmeier/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/Papenmeier/braille.o: $(SRC_TOP)Programs/brl_driver.h
$(BLD_TOP)Drivers/Papenmeier/braille.o: $(BLD_TOP)Drivers/Papenmeier/config.tab.c
$(BLD_TOP)Drivers/Papenmeier/braille.o: $(SRC_TOP)Drivers/Papenmeier/brl-cfg.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Papenmeier/config.tab.c:
$(BLD_TOP)Drivers/Papenmeier/config.tab.c: $(SRC_TOP)Drivers/Papenmeier/config.y
$(BLD_TOP)Drivers/Papenmeier/config.tab.c: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/Papenmeier/config.tab.c: $(SRC_TOP)Drivers/Papenmeier/brl-cfg.h
$(BLD_TOP)Drivers/Papenmeier/config.tab.c: $(SRC_TOP)Programs/brl.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Papenmeier/dump-codes.o:
$(BLD_TOP)Drivers/Papenmeier/dump-codes.o: $(SRC_TOP)Drivers/Papenmeier/dump-codes.c
$(BLD_TOP)Drivers/Papenmeier/dump-codes.o: $(BLD_TOP)config.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Papenmeier/read_config.o:
$(BLD_TOP)Drivers/Papenmeier/read_config.o: $(SRC_TOP)Drivers/Papenmeier/read_config.c
$(BLD_TOP)Drivers/Papenmeier/read_config.o: $(BLD_TOP)Drivers/Papenmeier/config.tab.c
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Papenmeier/serial.o:
$(BLD_TOP)Drivers/Papenmeier/serial.o: $(SRC_TOP)Drivers/Papenmeier/serial.c
$(BLD_TOP)Drivers/Papenmeier/serial.o: $(SRC_TOP)Drivers/Papenmeier/braille.c
$(BLD_TOP)Drivers/Papenmeier/serial.o: $(SRC_TOP)Drivers/Papenmeier/brl-cfg.h
$(BLD_TOP)Drivers/Papenmeier/serial.o: $(SRC_TOP)Drivers/Papenmeier/brlconf.h
$(BLD_TOP)Drivers/Papenmeier/serial.o: $(BLD_TOP)Drivers/Papenmeier/config.tab.c
$(BLD_TOP)Drivers/Papenmeier/serial.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/Papenmeier/serial.o: $(SRC_TOP)Programs/brl_driver.h
$(BLD_TOP)Drivers/Papenmeier/serial.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/Papenmeier/serial.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/Papenmeier/serial.o: $(SRC_TOP)Programs/misc.c
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Papenmeier/simulate.o:
$(BLD_TOP)Drivers/Papenmeier/simulate.o: $(SRC_TOP)Drivers/Papenmeier/simulate.c
$(BLD_TOP)Drivers/Papenmeier/simulate.o: $(SRC_TOP)Drivers/Papenmeier/braille.c
$(BLD_TOP)Drivers/Papenmeier/simulate.o: $(SRC_TOP)Drivers/Papenmeier/brl-cfg.h
$(BLD_TOP)Drivers/Papenmeier/simulate.o: $(SRC_TOP)Drivers/Papenmeier/brlconf.h
$(BLD_TOP)Drivers/Papenmeier/simulate.o: $(BLD_TOP)Drivers/Papenmeier/config.tab.c
$(BLD_TOP)Drivers/Papenmeier/simulate.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/Papenmeier/simulate.o: $(SRC_TOP)Programs/brl_driver.h
$(BLD_TOP)Drivers/Papenmeier/simulate.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/Papenmeier/simulate.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/Papenmeier/simulate.o: $(SRC_TOP)Programs/misc.c
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/TSI/braille.o:
$(BLD_TOP)Drivers/TSI/braille.o: $(SRC_TOP)Drivers/TSI/braille.c
$(BLD_TOP)Drivers/TSI/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/TSI/braille.o: $(SRC_TOP)Drivers/TSI/brlconf.h
$(BLD_TOP)Drivers/TSI/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/TSI/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/TSI/braille.o: $(SRC_TOP)Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Televox/speech.o:
$(BLD_TOP)Drivers/Televox/speech.o: $(SRC_TOP)Drivers/Televox/speech.c
$(BLD_TOP)Drivers/Televox/speech.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/Televox/speech.o: $(SRC_TOP)Drivers/Televox/speech.h
$(BLD_TOP)Drivers/Televox/speech.o: $(SRC_TOP)Programs/spk.h
$(BLD_TOP)Drivers/Televox/speech.o: $(SRC_TOP)Programs/spk_driver.h
$(BLD_TOP)Drivers/Televox/speech.o: $(SRC_TOP)Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Vario/braille.o:
$(BLD_TOP)Drivers/Vario/braille.o: $(SRC_TOP)Drivers/Vario/braille.c
$(BLD_TOP)Drivers/Vario/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/Vario/braille.o: $(SRC_TOP)Drivers/Vario/brlconf.h
$(BLD_TOP)Drivers/Vario/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/Vario/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/Vario/braille.o: $(SRC_TOP)Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/VarioHT/vario.o:
$(BLD_TOP)Drivers/VarioHT/vario.o: $(SRC_TOP)Drivers/VarioHT/vario.c
$(BLD_TOP)Drivers/VarioHT/vario.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/VarioHT/vario.o: $(SRC_TOP)Drivers/VarioHT/variolow.h
$(BLD_TOP)Drivers/VarioHT/vario.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/VarioHT/vario.o: $(SRC_TOP)Drivers/VarioHT/brlconf.h
$(BLD_TOP)Drivers/VarioHT/vario.o: $(SRC_TOP)Programs/brl_driver.h
$(BLD_TOP)Drivers/VarioHT/vario.o: $(SRC_TOP)Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/VarioHT/variolow.o:
$(BLD_TOP)Drivers/VarioHT/variolow.o: $(SRC_TOP)Drivers/VarioHT/variolow.c
$(BLD_TOP)Drivers/VarioHT/variolow.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/VarioHT/variolow.o: $(SRC_TOP)Drivers/VarioHT/variolow.h
$(BLD_TOP)Drivers/VarioHT/variolow.o: $(SRC_TOP)Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/ViaVoice/speech.o:
$(BLD_TOP)Drivers/ViaVoice/speech.o: $(SRC_TOP)Drivers/ViaVoice/speech.c
$(BLD_TOP)Drivers/ViaVoice/speech.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/ViaVoice/speech.o: $(SRC_TOP)Drivers/ViaVoice/speech.h
$(BLD_TOP)Drivers/ViaVoice/speech.o: $(SRC_TOP)Programs/spk.h
$(BLD_TOP)Drivers/ViaVoice/speech.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/ViaVoice/speech.o: $(SRC_TOP)Programs/spk_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/VideoBraille/vb.o:
$(BLD_TOP)Drivers/VideoBraille/vb.o: $(SRC_TOP)Drivers/VideoBraille/vb.c
$(BLD_TOP)Drivers/VideoBraille/vb.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/VideoBraille/vb.o: $(SRC_TOP)Drivers/VideoBraille/vblow.h
$(BLD_TOP)Drivers/VideoBraille/vb.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/VideoBraille/vb.o: $(SRC_TOP)Drivers/VideoBraille/brlconf.h
$(BLD_TOP)Drivers/VideoBraille/vb.o: $(SRC_TOP)Programs/brl_driver.h
$(BLD_TOP)Drivers/VideoBraille/vb.o: $(SRC_TOP)Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/VideoBraille/vblow.o:
$(BLD_TOP)Drivers/VideoBraille/vblow.o: $(SRC_TOP)Drivers/VideoBraille/vblow.c
$(BLD_TOP)Drivers/VideoBraille/vblow.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/VideoBraille/vblow.o: $(SRC_TOP)Drivers/VideoBraille/brlconf.h
$(BLD_TOP)Drivers/VideoBraille/vblow.o: $(SRC_TOP)Drivers/VideoBraille/vblow.h
$(BLD_TOP)Drivers/VideoBraille/vblow.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/VideoBraille/vblow.o: $(SRC_TOP)Programs/system.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/VisioBraille/braille.o:
$(BLD_TOP)Drivers/VisioBraille/braille.o: $(SRC_TOP)Drivers/VisioBraille/braille.c
$(BLD_TOP)Drivers/VisioBraille/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/VisioBraille/braille.o: $(SRC_TOP)Drivers/VisioBraille/brlconf.h
$(BLD_TOP)Drivers/VisioBraille/braille.o: $(SRC_TOP)Drivers/VisioBraille/brlkeydefs.h
$(BLD_TOP)Drivers/VisioBraille/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/VisioBraille/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/VisioBraille/braille.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Drivers/VisioBraille/braille.o: $(SRC_TOP)Programs/message.h
$(BLD_TOP)Drivers/VisioBraille/braille.o: $(SRC_TOP)Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Voyager/braille.o:
$(BLD_TOP)Drivers/Voyager/braille.o: $(SRC_TOP)Drivers/Voyager/braille.c
$(BLD_TOP)Drivers/Voyager/braille.o: $(BLD_TOP)config.h
$(BLD_TOP)Drivers/Voyager/braille.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Drivers/Voyager/braille.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Drivers/Voyager/braille.o: $(SRC_TOP)Programs/brl_driver.h
$(BLD_TOP)Drivers/Voyager/braille.o: $(SRC_TOP)Drivers/Voyager/brlconf.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Voyager/kernel/brlvger.o:
$(BLD_TOP)Drivers/Voyager/kernel/brlvger.o: $(SRC_TOP)Drivers/Voyager/kernel/brlvger.c
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Voyager/kernel/vgertest.o:
$(BLD_TOP)Drivers/Voyager/kernel/vgertest.o: $(SRC_TOP)Drivers/Voyager/kernel/vgertest.c
$(BLD_TOP)Drivers/Voyager/kernel/vgertest.o: $(SRC_TOP)Drivers/Voyager/kernel/linux/brlvger.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/adlib.o:
$(BLD_TOP)Programs/adlib.o: $(SRC_TOP)Programs/adlib.c
$(BLD_TOP)Programs/adlib.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/adlib.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/adlib.o: $(SRC_TOP)Programs/system.h
$(BLD_TOP)Programs/adlib.o: $(SRC_TOP)Programs/adlib.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/beeper.o:
$(BLD_TOP)Programs/beeper.o: $(SRC_TOP)Programs/beeper.c
$(BLD_TOP)Programs/beeper.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/beeper.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/beeper.o: $(SRC_TOP)Programs/system.h
$(BLD_TOP)Programs/beeper.o: $(SRC_TOP)Programs/notes.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/brl.o:
$(BLD_TOP)Programs/brl.o: $(SRC_TOP)Programs/brl.c
$(BLD_TOP)Programs/brl.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/brl.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/brl.o: $(SRC_TOP)Programs/system.h
$(BLD_TOP)Programs/brl.o: $(SRC_TOP)Programs/message.h
$(BLD_TOP)Programs/brl.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Programs/brl.o: $(SRC_TOP)Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/brltest.o:
$(BLD_TOP)Programs/brltest.o: $(SRC_TOP)Programs/brltest.c
$(BLD_TOP)Programs/brltest.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/brltest.o: $(SRC_TOP)Programs/options.h
$(BLD_TOP)Programs/brltest.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Programs/brltest.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/brltest.o: $(SRC_TOP)Programs/message.h
$(BLD_TOP)Programs/brltest.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Programs/brltest.o: $(SRC_TOP)Programs/defaults.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/config.o:
$(BLD_TOP)Programs/config.o: $(SRC_TOP)Programs/config.c
$(BLD_TOP)Programs/config.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/config.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Programs/config.o: $(SRC_TOP)Programs/spk.h
$(BLD_TOP)Programs/config.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Programs/config.o: $(SRC_TOP)Programs/contract.h
$(BLD_TOP)Programs/config.o: $(SRC_TOP)Programs/tunes.h
$(BLD_TOP)Programs/config.o: $(SRC_TOP)Programs/message.h
$(BLD_TOP)Programs/config.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/config.o: $(SRC_TOP)Programs/system.h
$(BLD_TOP)Programs/config.o: $(SRC_TOP)Programs/options.h
$(BLD_TOP)Programs/config.o: $(SRC_TOP)Programs/brltty.h
$(BLD_TOP)Programs/config.o: $(SRC_TOP)Programs/defaults.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/ctb_compile.o:
$(BLD_TOP)Programs/ctb_compile.o: $(SRC_TOP)Programs/ctb_compile.c
$(BLD_TOP)Programs/ctb_compile.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/ctb_compile.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/ctb_compile.o: $(SRC_TOP)Programs/contract.h
$(BLD_TOP)Programs/ctb_compile.o: $(SRC_TOP)Programs/ctb_definitions.h
$(BLD_TOP)Programs/ctb_compile.o: $(SRC_TOP)Programs/brl.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/ctb_translate.o:
$(BLD_TOP)Programs/ctb_translate.o: $(SRC_TOP)Programs/ctb_translate.c
$(BLD_TOP)Programs/ctb_translate.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/ctb_translate.o: $(SRC_TOP)Programs/contract.h
$(BLD_TOP)Programs/ctb_translate.o: $(SRC_TOP)Programs/ctb_definitions.h
$(BLD_TOP)Programs/ctb_translate.o: $(SRC_TOP)Programs/brl.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/cut.o:
$(BLD_TOP)Programs/cut.o: $(SRC_TOP)Programs/cut.c
$(BLD_TOP)Programs/cut.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/cut.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Programs/cut.o: $(SRC_TOP)Programs/tunes.h
$(BLD_TOP)Programs/cut.o: $(SRC_TOP)Programs/cut.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/fm.o:
$(BLD_TOP)Programs/fm.o: $(SRC_TOP)Programs/fm.c
$(BLD_TOP)Programs/fm.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/fm.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Programs/fm.o: $(SRC_TOP)Programs/brltty.h
$(BLD_TOP)Programs/fm.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/fm.o: $(SRC_TOP)Programs/notes.h
$(BLD_TOP)Programs/fm.o: $(SRC_TOP)Programs/adlib.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/main.o:
$(BLD_TOP)Programs/main.o: $(SRC_TOP)Programs/main.c
$(BLD_TOP)Programs/main.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/main.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/main.o: $(SRC_TOP)Programs/message.h
$(BLD_TOP)Programs/main.o: $(SRC_TOP)Programs/tunes.h
$(BLD_TOP)Programs/main.o: $(SRC_TOP)Programs/contract.h
$(BLD_TOP)Programs/main.o: $(SRC_TOP)Programs/route.h
$(BLD_TOP)Programs/main.o: $(SRC_TOP)Programs/cut.h
$(BLD_TOP)Programs/main.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Programs/main.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Programs/main.o: $(SRC_TOP)Programs/spk.h
$(BLD_TOP)Programs/main.o: $(SRC_TOP)Programs/brltty.h
$(BLD_TOP)Programs/main.o: $(SRC_TOP)Programs/defaults.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/midi.o:
$(BLD_TOP)Programs/midi.o: $(SRC_TOP)Programs/midi.c
$(BLD_TOP)Programs/midi.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/midi.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Programs/midi.o: $(SRC_TOP)Programs/brltty.h
$(BLD_TOP)Programs/midi.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/midi.o: $(SRC_TOP)Programs/system.h
$(BLD_TOP)Programs/midi.o: $(SRC_TOP)Programs/notes.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/misc.o:
$(BLD_TOP)Programs/misc.o: $(SRC_TOP)Programs/misc.c
$(BLD_TOP)Programs/misc.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/misc.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/misc.o: $(SRC_TOP)Programs/brl.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/options.o:
$(BLD_TOP)Programs/options.o: $(SRC_TOP)Programs/options.c
$(BLD_TOP)Programs/options.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/options.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/options.o: $(SRC_TOP)Programs/options.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/pcm.o:
$(BLD_TOP)Programs/pcm.o: $(SRC_TOP)Programs/pcm.c
$(BLD_TOP)Programs/pcm.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/pcm.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Programs/pcm.o: $(SRC_TOP)Programs/brltty.h
$(BLD_TOP)Programs/pcm.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/pcm.o: $(SRC_TOP)Programs/system.h
$(BLD_TOP)Programs/pcm.o: $(SRC_TOP)Programs/notes.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/route.o:
$(BLD_TOP)Programs/route.o: $(SRC_TOP)Programs/route.c
$(BLD_TOP)Programs/route.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/route.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/route.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Programs/route.o: $(SRC_TOP)Programs/route.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scr.o:
$(BLD_TOP)Programs/scr.o: $(SRC_TOP)Programs/scr.cc
$(BLD_TOP)Programs/scr.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/scr.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Programs/scr.o: $(SRC_TOP)Programs/scr_base.h
$(BLD_TOP)Programs/scr.o: $(SRC_TOP)Programs/scr_frozen.h
$(BLD_TOP)Programs/scr.o: $(SRC_TOP)Programs/help.h
$(BLD_TOP)Programs/scr.o: $(SRC_TOP)Programs/scr_help.h
$(BLD_TOP)Programs/scr.o: $(SRC_TOP)Programs/scr_real.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scr_base.o:
$(BLD_TOP)Programs/scr_base.o: $(SRC_TOP)Programs/scr_base.cc
$(BLD_TOP)Programs/scr_base.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/scr_base.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/scr_base.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Programs/scr_base.o: $(SRC_TOP)Programs/scr_base.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scr_frozen.o:
$(BLD_TOP)Programs/scr_frozen.o: $(SRC_TOP)Programs/scr_frozen.cc
$(BLD_TOP)Programs/scr_frozen.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/scr_frozen.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/scr_frozen.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Programs/scr_frozen.o: $(SRC_TOP)Programs/scr_base.h
$(BLD_TOP)Programs/scr_frozen.o: $(SRC_TOP)Programs/scr_frozen.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scr_help.o:
$(BLD_TOP)Programs/scr_help.o: $(SRC_TOP)Programs/scr_help.cc
$(BLD_TOP)Programs/scr_help.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/scr_help.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/scr_help.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Programs/scr_help.o: $(SRC_TOP)Programs/help.h
$(BLD_TOP)Programs/scr_help.o: $(SRC_TOP)Programs/scr_base.h
$(BLD_TOP)Programs/scr_help.o: $(SRC_TOP)Programs/scr_help.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scr_linux.o:
$(BLD_TOP)Programs/scr_linux.o: $(SRC_TOP)Programs/scr_linux.cc
$(BLD_TOP)Programs/scr_linux.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/scr_linux.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/scr_linux.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Programs/scr_linux.o: $(SRC_TOP)Programs/scr_base.h
$(BLD_TOP)Programs/scr_linux.o: $(SRC_TOP)Programs/scr_linux.h
$(BLD_TOP)Programs/scr_linux.o: $(SRC_TOP)Programs/scr_real.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scr_real.o:
$(BLD_TOP)Programs/scr_real.o: $(SRC_TOP)Programs/scr_real.cc
$(BLD_TOP)Programs/scr_real.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/scr_real.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/scr_real.o: $(SRC_TOP)Programs/route.h
$(BLD_TOP)Programs/scr_real.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Programs/scr_real.o: $(SRC_TOP)Programs/scr_base.h
$(BLD_TOP)Programs/scr_real.o: $(SRC_TOP)Programs/scr_real.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scr_shm.o:
$(BLD_TOP)Programs/scr_shm.o: $(SRC_TOP)Programs/scr_shm.cc
$(BLD_TOP)Programs/scr_shm.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/scr_shm.o: $(SRC_TOP)Programs/scr.h
$(BLD_TOP)Programs/scr_shm.o: $(SRC_TOP)Programs/scr_base.h
$(BLD_TOP)Programs/scr_shm.o: $(SRC_TOP)Programs/scr_real.h
$(BLD_TOP)Programs/scr_shm.o: $(SRC_TOP)Programs/scr_shm.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scrtest.o:
$(BLD_TOP)Programs/scrtest.o: $(SRC_TOP)Programs/scrtest.c
$(BLD_TOP)Programs/scrtest.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/scrtest.o: $(SRC_TOP)Programs/options.h
$(BLD_TOP)Programs/scrtest.o: $(SRC_TOP)Programs/scr.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/spk.o:
$(BLD_TOP)Programs/spk.o: $(SRC_TOP)Programs/spk.c
$(BLD_TOP)Programs/spk.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/spk.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/spk.o: $(SRC_TOP)Programs/system.h
$(BLD_TOP)Programs/spk.o: $(SRC_TOP)Programs/spk.h
$(BLD_TOP)Programs/spk.o: $(SRC_TOP)Programs/spk_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/sys_hpux.o:
$(BLD_TOP)Programs/sys_hpux.o: $(SRC_TOP)Programs/sys_hpux.c
$(BLD_TOP)Programs/sys_hpux.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/sys_hpux.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/sys_hpux.o: $(SRC_TOP)Programs/system.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/sys_linux.o:
$(BLD_TOP)Programs/sys_linux.o: $(SRC_TOP)Programs/sys_linux.c
$(BLD_TOP)Programs/sys_linux.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/sys_linux.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/sys_linux.o: $(SRC_TOP)Programs/system.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/sys_solaris.o:
$(BLD_TOP)Programs/sys_solaris.o: $(SRC_TOP)Programs/sys_solaris.c
$(BLD_TOP)Programs/sys_solaris.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/sys_solaris.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/sys_solaris.o: $(SRC_TOP)Programs/system.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/tbl2hex.o:
$(BLD_TOP)Programs/tbl2hex.o: $(SRC_TOP)Programs/tbl2hex.c
$(BLD_TOP)Programs/tbl2hex.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/tbl2hex.o: $(SRC_TOP)Programs/options.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/tunes.o:
$(BLD_TOP)Programs/tunes.o: $(SRC_TOP)Programs/tunes.c
$(BLD_TOP)Programs/tunes.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/tunes.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/tunes.o: $(SRC_TOP)Programs/system.h
$(BLD_TOP)Programs/tunes.o: $(SRC_TOP)Programs/message.h
$(BLD_TOP)Programs/tunes.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Programs/tunes.o: $(SRC_TOP)Programs/brltty.h
$(BLD_TOP)Programs/tunes.o: $(SRC_TOP)Programs/tunes.h
$(BLD_TOP)Programs/tunes.o: $(SRC_TOP)Programs/notes.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/tunetest.o:
$(BLD_TOP)Programs/tunetest.o: $(SRC_TOP)Programs/tunetest.c
$(BLD_TOP)Programs/tunetest.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/tunetest.o: $(SRC_TOP)Programs/options.h
$(BLD_TOP)Programs/tunetest.o: $(SRC_TOP)Programs/tunes.h
$(BLD_TOP)Programs/tunetest.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/tunetest.o: $(SRC_TOP)Programs/defaults.h
$(BLD_TOP)Programs/tunetest.o: $(SRC_TOP)Programs/brl.h
$(BLD_TOP)Programs/tunetest.o: $(SRC_TOP)Programs/brltty.h
$(BLD_TOP)Programs/tunetest.o: $(SRC_TOP)Programs/message.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/txt2hlp.o:
$(BLD_TOP)Programs/txt2hlp.o: $(SRC_TOP)Programs/txt2hlp.c
$(BLD_TOP)Programs/txt2hlp.o: $(BLD_TOP)config.h
$(BLD_TOP)Programs/txt2hlp.o: $(SRC_TOP)Programs/options.h
$(BLD_TOP)Programs/txt2hlp.o: $(SRC_TOP)Programs/misc.h
$(BLD_TOP)Programs/txt2hlp.o: $(SRC_TOP)Programs/help.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Unicode/trcp.o:
$(BLD_TOP)Unicode/trcp.o: $(SRC_TOP)Unicode/trcp.c
$(BLD_TOP)Unicode/trcp.o: $(BLD_TOP)config.h
$(BLD_TOP)Unicode/trcp.o: $(SRC_TOP)Unicode/unicode.h
$(BLD_TOP)Unicode/trcp.o: $(SRC_TOP)Programs/options.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Unicode/unicode.o:
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/unicode.c
$(BLD_TOP)Unicode/unicode.o: $(BLD_TOP)config.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/unicode.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/unicode-table.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-1.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-2.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-3.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-4.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-5.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-6.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-7.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-8.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-9.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-10.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-13.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-14.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-15.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/iso-8859-16.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-37.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-424.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-437.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-500.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-737.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-775.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-850.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-852.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-855.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-856.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-857.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-860.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-861.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-862.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-863.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-864.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-865.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-866.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-869.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-874.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-875.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-1006.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-1026.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-1250.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-1251.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-1252.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-1253.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-1254.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-1255.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-1256.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-1257.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/cp-1258.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/koi8-r.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/koi8-ru.h
$(BLD_TOP)Unicode/unicode.o: $(SRC_TOP)Unicode/koi8-u.h
	cd $(@D) && $(MAKE) $(@F)


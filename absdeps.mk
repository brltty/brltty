# Dependencies for BrailleTables/ibm437toiso.o:
$(TOP_DIR)/BrailleTables/ibm437toiso.o: $(TOP_DIR)/BrailleTables/ibm437toiso.c
$(TOP_DIR)/BrailleTables/ibm437toiso.o: $(TOP_DIR)/config.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for BrailleTables/ibm850toiso.o:
$(TOP_DIR)/BrailleTables/ibm850toiso.o: $(TOP_DIR)/BrailleTables/ibm850toiso.c
$(TOP_DIR)/BrailleTables/ibm850toiso.o: $(TOP_DIR)/config.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for BrailleTables/isotoibm437.o:
$(TOP_DIR)/BrailleTables/isotoibm437.o: $(TOP_DIR)/BrailleTables/isotoibm437.c
$(TOP_DIR)/BrailleTables/isotoibm437.o: $(TOP_DIR)/config.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for BrailleTables/tbl2jbt.o:
$(TOP_DIR)/BrailleTables/tbl2jbt.o: $(TOP_DIR)/BrailleTables/tbl2jbt.c
$(TOP_DIR)/BrailleTables/tbl2jbt.o: $(TOP_DIR)/config.h
$(TOP_DIR)/BrailleTables/tbl2jbt.o: $(TOP_DIR)/Programs/options.h
$(TOP_DIR)/BrailleTables/tbl2jbt.o: $(TOP_DIR)/Programs/brl.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for BrailleTables/tbl2tbl.o:
$(TOP_DIR)/BrailleTables/tbl2tbl.o: $(TOP_DIR)/BrailleTables/tbl2tbl.c
$(TOP_DIR)/BrailleTables/tbl2tbl.o: $(TOP_DIR)/config.h
$(TOP_DIR)/BrailleTables/tbl2tbl.o: $(TOP_DIR)/Programs/options.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for BrailleTables/tbl2txt.o:
$(TOP_DIR)/BrailleTables/tbl2txt.o: $(TOP_DIR)/BrailleTables/tbl2txt.c
$(TOP_DIR)/BrailleTables/tbl2txt.o: $(TOP_DIR)/config.h
$(TOP_DIR)/BrailleTables/tbl2txt.o: $(TOP_DIR)/Programs/options.h
$(TOP_DIR)/BrailleTables/tbl2txt.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/BrailleTables/tbl2txt.o: $(TOP_DIR)/Unicode/unicode.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for BrailleTables/txt2tbl.o:
$(TOP_DIR)/BrailleTables/txt2tbl.o: $(TOP_DIR)/BrailleTables/txt2tbl.c
$(TOP_DIR)/BrailleTables/txt2tbl.o: $(TOP_DIR)/config.h
$(TOP_DIR)/BrailleTables/txt2tbl.o: $(TOP_DIR)/Programs/options.h
$(TOP_DIR)/BrailleTables/txt2tbl.o: $(TOP_DIR)/Programs/brl.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Alva/braille.o:
$(TOP_DIR)/Drivers/Alva/braille.o: $(TOP_DIR)/Drivers/Alva/braille.c
$(TOP_DIR)/Drivers/Alva/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/Alva/braille.o: $(TOP_DIR)/Drivers/Alva/brlconf.h
$(TOP_DIR)/Drivers/Alva/braille.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/Alva/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/Alva/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/Alva/braille.o: $(TOP_DIR)/Programs/brltty.h
$(TOP_DIR)/Drivers/Alva/braille.o: $(TOP_DIR)/Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Alva/speech.o:
$(TOP_DIR)/Drivers/Alva/speech.o: $(TOP_DIR)/Drivers/Alva/speech.c
$(TOP_DIR)/Drivers/Alva/speech.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/Alva/speech.o: $(TOP_DIR)/Drivers/Alva/brlconf.h
$(TOP_DIR)/Drivers/Alva/speech.o: $(TOP_DIR)/Drivers/Alva/speech.h
$(TOP_DIR)/Drivers/Alva/speech.o: $(TOP_DIR)/Programs/spk.h
$(TOP_DIR)/Drivers/Alva/speech.o: $(TOP_DIR)/Programs/spk_driver.h
$(TOP_DIR)/Drivers/Alva/speech.o: $(TOP_DIR)/Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/BrailleLite/braille.o:
$(TOP_DIR)/Drivers/BrailleLite/braille.o: $(TOP_DIR)/Drivers/BrailleLite/braille.c
$(TOP_DIR)/Drivers/BrailleLite/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/BrailleLite/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/BrailleLite/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/BrailleLite/braille.o: $(TOP_DIR)/Programs/message.h
$(TOP_DIR)/Drivers/BrailleLite/braille.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/BrailleLite/braille.o: $(TOP_DIR)/Drivers/BrailleLite/brlconf.h
$(TOP_DIR)/Drivers/BrailleLite/braille.o: $(TOP_DIR)/Programs/brl_driver.h
$(TOP_DIR)/Drivers/BrailleLite/braille.o: $(TOP_DIR)/Drivers/BrailleLite/bindings.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/BrailleLite/speech.o:
$(TOP_DIR)/Drivers/BrailleLite/speech.o: $(TOP_DIR)/Drivers/BrailleLite/speech.c
$(TOP_DIR)/Drivers/BrailleLite/speech.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/BrailleLite/speech.o: $(TOP_DIR)/Drivers/BrailleLite/speech.h
$(TOP_DIR)/Drivers/BrailleLite/speech.o: $(TOP_DIR)/Programs/spk.h
$(TOP_DIR)/Drivers/BrailleLite/speech.o: $(TOP_DIR)/Programs/spk_driver.h
$(TOP_DIR)/Drivers/BrailleLite/speech.o: $(TOP_DIR)/Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/BrailleNote/braille.o:
$(TOP_DIR)/Drivers/BrailleNote/braille.o: $(TOP_DIR)/Drivers/BrailleNote/braille.c
$(TOP_DIR)/Drivers/BrailleNote/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/BrailleNote/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/BrailleNote/braille.o: $(TOP_DIR)/Drivers/BrailleNote/brlconf.h
$(TOP_DIR)/Drivers/BrailleNote/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/BrailleNote/braille.o: $(TOP_DIR)/Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/CombiBraille/braille.o:
$(TOP_DIR)/Drivers/CombiBraille/braille.o: $(TOP_DIR)/Drivers/CombiBraille/braille.c
$(TOP_DIR)/Drivers/CombiBraille/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/CombiBraille/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/CombiBraille/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/CombiBraille/braille.o: $(TOP_DIR)/Drivers/CombiBraille/brlconf.h
$(TOP_DIR)/Drivers/CombiBraille/braille.o: $(TOP_DIR)/Programs/brl_driver.h
$(TOP_DIR)/Drivers/CombiBraille/braille.o: $(TOP_DIR)/Drivers/CombiBraille/cmdtrans.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/CombiBraille/speech.o:
$(TOP_DIR)/Drivers/CombiBraille/speech.o: $(TOP_DIR)/Drivers/CombiBraille/speech.c
$(TOP_DIR)/Drivers/CombiBraille/speech.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/CombiBraille/speech.o: $(TOP_DIR)/Drivers/CombiBraille/brlconf.h
$(TOP_DIR)/Drivers/CombiBraille/speech.o: $(TOP_DIR)/Drivers/CombiBraille/speech.h
$(TOP_DIR)/Drivers/CombiBraille/speech.o: $(TOP_DIR)/Programs/spk.h
$(TOP_DIR)/Drivers/CombiBraille/speech.o: $(TOP_DIR)/Programs/spk_driver.h
$(TOP_DIR)/Drivers/CombiBraille/speech.o: $(TOP_DIR)/Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/EcoBraille/braille.o:
$(TOP_DIR)/Drivers/EcoBraille/braille.o: $(TOP_DIR)/Drivers/EcoBraille/braille.c
$(TOP_DIR)/Drivers/EcoBraille/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/EcoBraille/braille.o: $(TOP_DIR)/Drivers/EcoBraille/brlconf.h
$(TOP_DIR)/Drivers/EcoBraille/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/EcoBraille/braille.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/EcoBraille/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/EcoBraille/braille.o: $(TOP_DIR)/Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/EuroBraille/braille.o:
$(TOP_DIR)/Drivers/EuroBraille/braille.o: $(TOP_DIR)/Drivers/EuroBraille/braille.c
$(TOP_DIR)/Drivers/EuroBraille/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/EuroBraille/braille.o: $(TOP_DIR)/Drivers/EuroBraille/brlconf.h
$(TOP_DIR)/Drivers/EuroBraille/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/EuroBraille/braille.o: $(TOP_DIR)/Programs/message.h
$(TOP_DIR)/Drivers/EuroBraille/braille.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/EuroBraille/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/EuroBraille/braille.o: $(TOP_DIR)/Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/ExternalSpeech/speech.o:
$(TOP_DIR)/Drivers/ExternalSpeech/speech.o: $(TOP_DIR)/Drivers/ExternalSpeech/speech.c
$(TOP_DIR)/Drivers/ExternalSpeech/speech.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/ExternalSpeech/speech.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/ExternalSpeech/speech.o: $(TOP_DIR)/Drivers/ExternalSpeech/speech.h
$(TOP_DIR)/Drivers/ExternalSpeech/speech.o: $(TOP_DIR)/Programs/spk.h
$(TOP_DIR)/Drivers/ExternalSpeech/speech.o: $(TOP_DIR)/Programs/spk_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Festival/speech.o:
$(TOP_DIR)/Drivers/Festival/speech.o: $(TOP_DIR)/Drivers/Festival/speech.c
$(TOP_DIR)/Drivers/Festival/speech.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/Festival/speech.o: $(TOP_DIR)/Drivers/Festival/speech.h
$(TOP_DIR)/Drivers/Festival/speech.o: $(TOP_DIR)/Programs/spk.h
$(TOP_DIR)/Drivers/Festival/speech.o: $(TOP_DIR)/Programs/spk_driver.h
$(TOP_DIR)/Drivers/Festival/speech.o: $(TOP_DIR)/Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/GenericSay/speech.o:
$(TOP_DIR)/Drivers/GenericSay/speech.o: $(TOP_DIR)/Drivers/GenericSay/speech.c
$(TOP_DIR)/Drivers/GenericSay/speech.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/GenericSay/speech.o: $(TOP_DIR)/Drivers/GenericSay/speech.h
$(TOP_DIR)/Drivers/GenericSay/speech.o: $(TOP_DIR)/Programs/spk.h
$(TOP_DIR)/Drivers/GenericSay/speech.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/GenericSay/speech.o: $(TOP_DIR)/Programs/spk_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/HandyTech/braille.o:
$(TOP_DIR)/Drivers/HandyTech/braille.o: $(TOP_DIR)/Drivers/HandyTech/braille.c
$(TOP_DIR)/Drivers/HandyTech/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/HandyTech/braille.o: $(TOP_DIR)/Drivers/HandyTech/brlconf.h
$(TOP_DIR)/Drivers/HandyTech/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/HandyTech/braille.o: $(TOP_DIR)/Programs/brltty.h
$(TOP_DIR)/Drivers/HandyTech/braille.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/HandyTech/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/HandyTech/braille.o: $(TOP_DIR)/Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/LogText/braille.o:
$(TOP_DIR)/Drivers/LogText/braille.o: $(TOP_DIR)/Drivers/LogText/braille.c
$(TOP_DIR)/Drivers/LogText/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/LogText/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/LogText/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/LogText/braille.o: $(TOP_DIR)/Drivers/LogText/brlconf.h
$(TOP_DIR)/Drivers/LogText/braille.o: $(TOP_DIR)/Programs/brl_driver.h
$(TOP_DIR)/Drivers/LogText/braille.o: $(TOP_DIR)/Drivers/LogText/input.h
$(TOP_DIR)/Drivers/LogText/braille.o: $(TOP_DIR)/Drivers/LogText/output.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/MDV/braille.o:
$(TOP_DIR)/Drivers/MDV/braille.o: $(TOP_DIR)/Drivers/MDV/braille.c
$(TOP_DIR)/Drivers/MDV/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/MDV/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/MDV/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/MDV/braille.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/MDV/braille.o: $(TOP_DIR)/Drivers/MDV/brlconf.h
$(TOP_DIR)/Drivers/MDV/braille.o: $(TOP_DIR)/Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/MiniBraille/braille.o:
$(TOP_DIR)/Drivers/MiniBraille/braille.o: $(TOP_DIR)/Drivers/MiniBraille/braille.c
$(TOP_DIR)/Drivers/MiniBraille/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/MiniBraille/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/MiniBraille/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/MiniBraille/braille.o: $(TOP_DIR)/Drivers/MiniBraille/brlconf.h
$(TOP_DIR)/Drivers/MiniBraille/braille.o: $(TOP_DIR)/Programs/brl_driver.h
$(TOP_DIR)/Drivers/MiniBraille/braille.o: $(TOP_DIR)/Programs/message.h
$(TOP_DIR)/Drivers/MiniBraille/braille.o: $(TOP_DIR)/Drivers/MiniBraille/minibraille.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/MultiBraille/braille.o:
$(TOP_DIR)/Drivers/MultiBraille/braille.o: $(TOP_DIR)/Drivers/MultiBraille/braille.c
$(TOP_DIR)/Drivers/MultiBraille/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/MultiBraille/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/MultiBraille/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/MultiBraille/braille.o: $(TOP_DIR)/Drivers/MultiBraille/brlconf.h
$(TOP_DIR)/Drivers/MultiBraille/braille.o: $(TOP_DIR)/Programs/brl_driver.h
$(TOP_DIR)/Drivers/MultiBraille/braille.o: $(TOP_DIR)/Drivers/MultiBraille/tables.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/MultiBraille/speech.o:
$(TOP_DIR)/Drivers/MultiBraille/speech.o: $(TOP_DIR)/Drivers/MultiBraille/speech.c
$(TOP_DIR)/Drivers/MultiBraille/speech.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/MultiBraille/speech.o: $(TOP_DIR)/Drivers/MultiBraille/brlconf.h
$(TOP_DIR)/Drivers/MultiBraille/speech.o: $(TOP_DIR)/Drivers/MultiBraille/speech.h
$(TOP_DIR)/Drivers/MultiBraille/speech.o: $(TOP_DIR)/Programs/spk.h
$(TOP_DIR)/Drivers/MultiBraille/speech.o: $(TOP_DIR)/Programs/spk_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Papenmeier/braille.o:
$(TOP_DIR)/Drivers/Papenmeier/braille.o: $(TOP_DIR)/Drivers/Papenmeier/braille.c
$(TOP_DIR)/Drivers/Papenmeier/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/Papenmeier/braille.o: $(TOP_DIR)/Drivers/Papenmeier/brlconf.h
$(TOP_DIR)/Drivers/Papenmeier/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/Papenmeier/braille.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/Papenmeier/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/Papenmeier/braille.o: $(TOP_DIR)/Programs/brl_driver.h
$(TOP_DIR)/Drivers/Papenmeier/braille.o: $(TOP_DIR)/Drivers/Papenmeier/config.tab.c
$(TOP_DIR)/Drivers/Papenmeier/braille.o: $(TOP_DIR)/Drivers/Papenmeier/brl-cfg.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Papenmeier/config.tab.c:
$(TOP_DIR)/Drivers/Papenmeier/config.tab.c: $(TOP_DIR)/Drivers/Papenmeier/config.y
$(TOP_DIR)/Drivers/Papenmeier/config.tab.c: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/Papenmeier/config.tab.c: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/Papenmeier/config.tab.c: $(TOP_DIR)/Drivers/Papenmeier/brl-cfg.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Papenmeier/dump-codes.o:
$(TOP_DIR)/Drivers/Papenmeier/dump-codes.o: $(TOP_DIR)/Drivers/Papenmeier/dump-codes.c
$(TOP_DIR)/Drivers/Papenmeier/dump-codes.o: $(TOP_DIR)/config.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Papenmeier/read_config.o:
$(TOP_DIR)/Drivers/Papenmeier/read_config.o: $(TOP_DIR)/Drivers/Papenmeier/read_config.c
$(TOP_DIR)/Drivers/Papenmeier/read_config.o: $(TOP_DIR)/Drivers/Papenmeier/config.tab.c
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Papenmeier/serial.o:
$(TOP_DIR)/Drivers/Papenmeier/serial.o: $(TOP_DIR)/Drivers/Papenmeier/serial.c
$(TOP_DIR)/Drivers/Papenmeier/serial.o: $(TOP_DIR)/Drivers/Papenmeier/braille.c
$(TOP_DIR)/Drivers/Papenmeier/serial.o: $(TOP_DIR)/Drivers/Papenmeier/brl-cfg.h
$(TOP_DIR)/Drivers/Papenmeier/serial.o: $(TOP_DIR)/Drivers/Papenmeier/brlconf.h
$(TOP_DIR)/Drivers/Papenmeier/serial.o: $(TOP_DIR)/Drivers/Papenmeier/config.tab.c
$(TOP_DIR)/Drivers/Papenmeier/serial.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/Papenmeier/serial.o: $(TOP_DIR)/Programs/brl_driver.h
$(TOP_DIR)/Drivers/Papenmeier/serial.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/Papenmeier/serial.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/Papenmeier/serial.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/Papenmeier/serial.o: $(TOP_DIR)/Programs/misc.c
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Papenmeier/simulate.o:
$(TOP_DIR)/Drivers/Papenmeier/simulate.o: $(TOP_DIR)/Drivers/Papenmeier/simulate.c
$(TOP_DIR)/Drivers/Papenmeier/simulate.o: $(TOP_DIR)/Drivers/Papenmeier/braille.c
$(TOP_DIR)/Drivers/Papenmeier/simulate.o: $(TOP_DIR)/Drivers/Papenmeier/brl-cfg.h
$(TOP_DIR)/Drivers/Papenmeier/simulate.o: $(TOP_DIR)/Drivers/Papenmeier/brlconf.h
$(TOP_DIR)/Drivers/Papenmeier/simulate.o: $(TOP_DIR)/Drivers/Papenmeier/config.tab.c
$(TOP_DIR)/Drivers/Papenmeier/simulate.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/Papenmeier/simulate.o: $(TOP_DIR)/Programs/brl_driver.h
$(TOP_DIR)/Drivers/Papenmeier/simulate.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/Papenmeier/simulate.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/Papenmeier/simulate.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/Papenmeier/simulate.o: $(TOP_DIR)/Programs/misc.c
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/TSI/braille.o:
$(TOP_DIR)/Drivers/TSI/braille.o: $(TOP_DIR)/Drivers/TSI/braille.c
$(TOP_DIR)/Drivers/TSI/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/TSI/braille.o: $(TOP_DIR)/Drivers/TSI/brlconf.h
$(TOP_DIR)/Drivers/TSI/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/TSI/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/TSI/braille.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/TSI/braille.o: $(TOP_DIR)/Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Televox/speech.o:
$(TOP_DIR)/Drivers/Televox/speech.o: $(TOP_DIR)/Drivers/Televox/speech.c
$(TOP_DIR)/Drivers/Televox/speech.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/Televox/speech.o: $(TOP_DIR)/Drivers/Televox/speech.h
$(TOP_DIR)/Drivers/Televox/speech.o: $(TOP_DIR)/Programs/spk.h
$(TOP_DIR)/Drivers/Televox/speech.o: $(TOP_DIR)/Programs/spk_driver.h
$(TOP_DIR)/Drivers/Televox/speech.o: $(TOP_DIR)/Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Vario/braille.o:
$(TOP_DIR)/Drivers/Vario/braille.o: $(TOP_DIR)/Drivers/Vario/braille.c
$(TOP_DIR)/Drivers/Vario/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/Vario/braille.o: $(TOP_DIR)/Drivers/Vario/brlconf.h
$(TOP_DIR)/Drivers/Vario/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/Vario/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/Vario/braille.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/Vario/braille.o: $(TOP_DIR)/Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/VarioHT/vario.o:
$(TOP_DIR)/Drivers/VarioHT/vario.o: $(TOP_DIR)/Drivers/VarioHT/vario.c
$(TOP_DIR)/Drivers/VarioHT/vario.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/VarioHT/vario.o: $(TOP_DIR)/Drivers/VarioHT/variolow.h
$(TOP_DIR)/Drivers/VarioHT/vario.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/VarioHT/vario.o: $(TOP_DIR)/Drivers/VarioHT/brlconf.h
$(TOP_DIR)/Drivers/VarioHT/vario.o: $(TOP_DIR)/Programs/brl_driver.h
$(TOP_DIR)/Drivers/VarioHT/vario.o: $(TOP_DIR)/Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/VarioHT/variolow.o:
$(TOP_DIR)/Drivers/VarioHT/variolow.o: $(TOP_DIR)/Drivers/VarioHT/variolow.c
$(TOP_DIR)/Drivers/VarioHT/variolow.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/VarioHT/variolow.o: $(TOP_DIR)/Drivers/VarioHT/variolow.h
$(TOP_DIR)/Drivers/VarioHT/variolow.o: $(TOP_DIR)/Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/ViaVoice/speech.o:
$(TOP_DIR)/Drivers/ViaVoice/speech.o: $(TOP_DIR)/Drivers/ViaVoice/speech.c
$(TOP_DIR)/Drivers/ViaVoice/speech.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/ViaVoice/speech.o: $(TOP_DIR)/Drivers/ViaVoice/speech.h
$(TOP_DIR)/Drivers/ViaVoice/speech.o: $(TOP_DIR)/Programs/spk.h
$(TOP_DIR)/Drivers/ViaVoice/speech.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/ViaVoice/speech.o: $(TOP_DIR)/Programs/spk_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/VideoBraille/vb.o:
$(TOP_DIR)/Drivers/VideoBraille/vb.o: $(TOP_DIR)/Drivers/VideoBraille/vb.c
$(TOP_DIR)/Drivers/VideoBraille/vb.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/VideoBraille/vb.o: $(TOP_DIR)/Drivers/VideoBraille/vblow.h
$(TOP_DIR)/Drivers/VideoBraille/vb.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/VideoBraille/vb.o: $(TOP_DIR)/Drivers/VideoBraille/brlconf.h
$(TOP_DIR)/Drivers/VideoBraille/vb.o: $(TOP_DIR)/Programs/brl_driver.h
$(TOP_DIR)/Drivers/VideoBraille/vb.o: $(TOP_DIR)/Programs/misc.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/VideoBraille/vblow.o:
$(TOP_DIR)/Drivers/VideoBraille/vblow.o: $(TOP_DIR)/Drivers/VideoBraille/vblow.c
$(TOP_DIR)/Drivers/VideoBraille/vblow.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/VideoBraille/vblow.o: $(TOP_DIR)/Drivers/VideoBraille/brlconf.h
$(TOP_DIR)/Drivers/VideoBraille/vblow.o: $(TOP_DIR)/Drivers/VideoBraille/vblow.h
$(TOP_DIR)/Drivers/VideoBraille/vblow.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/VideoBraille/vblow.o: $(TOP_DIR)/Programs/system.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/VisioBraille/braille.o:
$(TOP_DIR)/Drivers/VisioBraille/braille.o: $(TOP_DIR)/Drivers/VisioBraille/braille.c
$(TOP_DIR)/Drivers/VisioBraille/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/VisioBraille/braille.o: $(TOP_DIR)/Drivers/VisioBraille/brlconf.h
$(TOP_DIR)/Drivers/VisioBraille/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/VisioBraille/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/VisioBraille/braille.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/VisioBraille/braille.o: $(TOP_DIR)/Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Voyager/braille.o:
$(TOP_DIR)/Drivers/Voyager/braille.o: $(TOP_DIR)/Drivers/Voyager/braille.c
$(TOP_DIR)/Drivers/Voyager/braille.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Drivers/Voyager/braille.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Drivers/Voyager/braille.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Drivers/Voyager/braille.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Drivers/Voyager/braille.o: $(TOP_DIR)/Programs/brl_driver.h
$(TOP_DIR)/Drivers/Voyager/braille.o: $(TOP_DIR)/Drivers/Voyager/brlconf.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Voyager/kernel/brlvger.o:
$(TOP_DIR)/Drivers/Voyager/kernel/brlvger.o: $(TOP_DIR)/Drivers/Voyager/kernel/brlvger.c
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Drivers/Voyager/kernel/vgertest.o:
$(TOP_DIR)/Drivers/Voyager/kernel/vgertest.o: $(TOP_DIR)/Drivers/Voyager/kernel/vgertest.c
$(TOP_DIR)/Drivers/Voyager/kernel/vgertest.o: $(TOP_DIR)/Drivers/Voyager/kernel/linux/brlvger.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/adlib.o:
$(TOP_DIR)/Programs/adlib.o: $(TOP_DIR)/Programs/adlib.c
$(TOP_DIR)/Programs/adlib.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/adlib.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/adlib.o: $(TOP_DIR)/Programs/system.h
$(TOP_DIR)/Programs/adlib.o: $(TOP_DIR)/Programs/adlib.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/brl.o:
$(TOP_DIR)/Programs/brl.o: $(TOP_DIR)/Programs/brl.c
$(TOP_DIR)/Programs/brl.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/brl.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/brl.o: $(TOP_DIR)/Programs/system.h
$(TOP_DIR)/Programs/brl.o: $(TOP_DIR)/Programs/message.h
$(TOP_DIR)/Programs/brl.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Programs/brl.o: $(TOP_DIR)/Programs/brl_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/brltest.o:
$(TOP_DIR)/Programs/brltest.o: $(TOP_DIR)/Programs/brltest.c
$(TOP_DIR)/Programs/brltest.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/brltest.o: $(TOP_DIR)/Programs/options.h
$(TOP_DIR)/Programs/brltest.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Programs/brltest.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/brltest.o: $(TOP_DIR)/Programs/message.h
$(TOP_DIR)/Programs/brltest.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Programs/brltest.o: $(TOP_DIR)/Programs/defaults.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/config.o:
$(TOP_DIR)/Programs/config.o: $(TOP_DIR)/Programs/config.c
$(TOP_DIR)/Programs/config.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/config.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Programs/config.o: $(TOP_DIR)/Programs/spk.h
$(TOP_DIR)/Programs/config.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Programs/config.o: $(TOP_DIR)/Programs/contract.h
$(TOP_DIR)/Programs/config.o: $(TOP_DIR)/Programs/tunes.h
$(TOP_DIR)/Programs/config.o: $(TOP_DIR)/Programs/message.h
$(TOP_DIR)/Programs/config.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/config.o: $(TOP_DIR)/Programs/options.h
$(TOP_DIR)/Programs/config.o: $(TOP_DIR)/Programs/brltty.h
$(TOP_DIR)/Programs/config.o: $(TOP_DIR)/Programs/defaults.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/ctb_compile.o:
$(TOP_DIR)/Programs/ctb_compile.o: $(TOP_DIR)/Programs/ctb_compile.c
$(TOP_DIR)/Programs/ctb_compile.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/ctb_compile.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/ctb_compile.o: $(TOP_DIR)/Programs/contract.h
$(TOP_DIR)/Programs/ctb_compile.o: $(TOP_DIR)/Programs/ctb_definitions.h
$(TOP_DIR)/Programs/ctb_compile.o: $(TOP_DIR)/Programs/brl.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/ctb_translate.o:
$(TOP_DIR)/Programs/ctb_translate.o: $(TOP_DIR)/Programs/ctb_translate.c
$(TOP_DIR)/Programs/ctb_translate.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/ctb_translate.o: $(TOP_DIR)/Programs/contract.h
$(TOP_DIR)/Programs/ctb_translate.o: $(TOP_DIR)/Programs/ctb_definitions.h
$(TOP_DIR)/Programs/ctb_translate.o: $(TOP_DIR)/Programs/brl.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/cut.o:
$(TOP_DIR)/Programs/cut.o: $(TOP_DIR)/Programs/cut.c
$(TOP_DIR)/Programs/cut.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/cut.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Programs/cut.o: $(TOP_DIR)/Programs/tunes.h
$(TOP_DIR)/Programs/cut.o: $(TOP_DIR)/Programs/cut.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/main.o:
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/Programs/main.c
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/Programs/message.h
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/Programs/tunes.h
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/Programs/contract.h
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/Programs/route.h
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/Programs/cut.h
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/Programs/spk.h
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/Programs/brltty.h
$(TOP_DIR)/Programs/main.o: $(TOP_DIR)/Programs/defaults.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/misc.o:
$(TOP_DIR)/Programs/misc.o: $(TOP_DIR)/Programs/misc.c
$(TOP_DIR)/Programs/misc.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/misc.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/misc.o: $(TOP_DIR)/Programs/brl.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/options.o:
$(TOP_DIR)/Programs/options.o: $(TOP_DIR)/Programs/options.c
$(TOP_DIR)/Programs/options.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/options.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/options.o: $(TOP_DIR)/Programs/options.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/route.o:
$(TOP_DIR)/Programs/route.o: $(TOP_DIR)/Programs/route.c
$(TOP_DIR)/Programs/route.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/route.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/route.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Programs/route.o: $(TOP_DIR)/Programs/route.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scr.o:
$(TOP_DIR)/Programs/scr.o: $(TOP_DIR)/Programs/scr.cc
$(TOP_DIR)/Programs/scr.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/scr.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Programs/scr.o: $(TOP_DIR)/Programs/help.h
$(TOP_DIR)/Programs/scr.o: $(TOP_DIR)/Programs/scr_base.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scr_base.o:
$(TOP_DIR)/Programs/scr_base.o: $(TOP_DIR)/Programs/scr_base.cc
$(TOP_DIR)/Programs/scr_base.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/scr_base.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/scr_base.o: $(TOP_DIR)/Programs/route.h
$(TOP_DIR)/Programs/scr_base.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Programs/scr_base.o: $(TOP_DIR)/Programs/help.h
$(TOP_DIR)/Programs/scr_base.o: $(TOP_DIR)/Programs/scr_base.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scr_linux.o:
$(TOP_DIR)/Programs/scr_linux.o: $(TOP_DIR)/Programs/scr_linux.cc
$(TOP_DIR)/Programs/scr_linux.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/scr_linux.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/scr_linux.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Programs/scr_linux.o: $(TOP_DIR)/Programs/help.h
$(TOP_DIR)/Programs/scr_linux.o: $(TOP_DIR)/Programs/scr_base.h
$(TOP_DIR)/Programs/scr_linux.o: $(TOP_DIR)/Programs/scr_linux.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scr_shm.o:
$(TOP_DIR)/Programs/scr_shm.o: $(TOP_DIR)/Programs/scr_shm.cc
$(TOP_DIR)/Programs/scr_shm.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/scr_shm.o: $(TOP_DIR)/Programs/scr.h
$(TOP_DIR)/Programs/scr_shm.o: $(TOP_DIR)/Programs/help.h
$(TOP_DIR)/Programs/scr_shm.o: $(TOP_DIR)/Programs/scr_base.h
$(TOP_DIR)/Programs/scr_shm.o: $(TOP_DIR)/Programs/scr_shm.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/scrtest.o:
$(TOP_DIR)/Programs/scrtest.o: $(TOP_DIR)/Programs/scrtest.c
$(TOP_DIR)/Programs/scrtest.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/scrtest.o: $(TOP_DIR)/Programs/options.h
$(TOP_DIR)/Programs/scrtest.o: $(TOP_DIR)/Programs/scr.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/spk.o:
$(TOP_DIR)/Programs/spk.o: $(TOP_DIR)/Programs/spk.c
$(TOP_DIR)/Programs/spk.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/spk.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/spk.o: $(TOP_DIR)/Programs/system.h
$(TOP_DIR)/Programs/spk.o: $(TOP_DIR)/Programs/spk.h
$(TOP_DIR)/Programs/spk.o: $(TOP_DIR)/Programs/spk_driver.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/sys_hpux.o:
$(TOP_DIR)/Programs/sys_hpux.o: $(TOP_DIR)/Programs/sys_hpux.c
$(TOP_DIR)/Programs/sys_hpux.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/sys_hpux.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/sys_hpux.o: $(TOP_DIR)/Programs/system.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/sys_linux.o:
$(TOP_DIR)/Programs/sys_linux.o: $(TOP_DIR)/Programs/sys_linux.c
$(TOP_DIR)/Programs/sys_linux.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/sys_linux.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/sys_linux.o: $(TOP_DIR)/Programs/system.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/sys_solaris.o:
$(TOP_DIR)/Programs/sys_solaris.o: $(TOP_DIR)/Programs/sys_solaris.c
$(TOP_DIR)/Programs/sys_solaris.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/sys_solaris.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/sys_solaris.o: $(TOP_DIR)/Programs/system.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/tbl2hex.o:
$(TOP_DIR)/Programs/tbl2hex.o: $(TOP_DIR)/Programs/tbl2hex.c
$(TOP_DIR)/Programs/tbl2hex.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/tbl2hex.o: $(TOP_DIR)/Programs/options.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/tones_adlib.o:
$(TOP_DIR)/Programs/tones_adlib.o: $(TOP_DIR)/Programs/tones_adlib.c
$(TOP_DIR)/Programs/tones_adlib.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/tones_adlib.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/tones_adlib.o: $(TOP_DIR)/Programs/tones.h
$(TOP_DIR)/Programs/tones_adlib.o: $(TOP_DIR)/Programs/adlib.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/tones_dac.o:
$(TOP_DIR)/Programs/tones_dac.o: $(TOP_DIR)/Programs/tones_dac.c
$(TOP_DIR)/Programs/tones_dac.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/tones_dac.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/tones_dac.o: $(TOP_DIR)/Programs/system.h
$(TOP_DIR)/Programs/tones_dac.o: $(TOP_DIR)/Programs/tones.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/tones_midi.o:
$(TOP_DIR)/Programs/tones_midi.o: $(TOP_DIR)/Programs/tones_midi.c
$(TOP_DIR)/Programs/tones_midi.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/tones_midi.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Programs/tones_midi.o: $(TOP_DIR)/Programs/brltty.h
$(TOP_DIR)/Programs/tones_midi.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/tones_midi.o: $(TOP_DIR)/Programs/system.h
$(TOP_DIR)/Programs/tones_midi.o: $(TOP_DIR)/Programs/tones.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/tones_speaker.o:
$(TOP_DIR)/Programs/tones_speaker.o: $(TOP_DIR)/Programs/tones_speaker.c
$(TOP_DIR)/Programs/tones_speaker.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/tones_speaker.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/tones_speaker.o: $(TOP_DIR)/Programs/system.h
$(TOP_DIR)/Programs/tones_speaker.o: $(TOP_DIR)/Programs/tones.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/tunes.o:
$(TOP_DIR)/Programs/tunes.o: $(TOP_DIR)/Programs/tunes.c
$(TOP_DIR)/Programs/tunes.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/tunes.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/tunes.o: $(TOP_DIR)/Programs/system.h
$(TOP_DIR)/Programs/tunes.o: $(TOP_DIR)/Programs/message.h
$(TOP_DIR)/Programs/tunes.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Programs/tunes.o: $(TOP_DIR)/Programs/brltty.h
$(TOP_DIR)/Programs/tunes.o: $(TOP_DIR)/Programs/tunes.h
$(TOP_DIR)/Programs/tunes.o: $(TOP_DIR)/Programs/tones.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/tunetest.o:
$(TOP_DIR)/Programs/tunetest.o: $(TOP_DIR)/Programs/tunetest.c
$(TOP_DIR)/Programs/tunetest.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/tunetest.o: $(TOP_DIR)/Programs/options.h
$(TOP_DIR)/Programs/tunetest.o: $(TOP_DIR)/Programs/tunes.h
$(TOP_DIR)/Programs/tunetest.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/tunetest.o: $(TOP_DIR)/Programs/defaults.h
$(TOP_DIR)/Programs/tunetest.o: $(TOP_DIR)/Programs/brl.h
$(TOP_DIR)/Programs/tunetest.o: $(TOP_DIR)/Programs/brltty.h
$(TOP_DIR)/Programs/tunetest.o: $(TOP_DIR)/Programs/message.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Programs/txt2hlp.o:
$(TOP_DIR)/Programs/txt2hlp.o: $(TOP_DIR)/Programs/txt2hlp.c
$(TOP_DIR)/Programs/txt2hlp.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Programs/txt2hlp.o: $(TOP_DIR)/Programs/options.h
$(TOP_DIR)/Programs/txt2hlp.o: $(TOP_DIR)/Programs/misc.h
$(TOP_DIR)/Programs/txt2hlp.o: $(TOP_DIR)/Programs/help.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Unicode/trcp.o:
$(TOP_DIR)/Unicode/trcp.o: $(TOP_DIR)/Unicode/trcp.c
$(TOP_DIR)/Unicode/trcp.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Unicode/trcp.o: $(TOP_DIR)/Unicode/unicode.h
$(TOP_DIR)/Unicode/trcp.o: $(TOP_DIR)/Programs/options.h
	cd $(@D) && $(MAKE) $(@F)

# Dependencies for Unicode/unicode.o:
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/unicode.c
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/config.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/unicode.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/unicode-table.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-1.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-2.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-3.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-4.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-5.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-6.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-7.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-8.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-9.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-10.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-13.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-14.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-15.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/iso-8859-16.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-37.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-424.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-437.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-500.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-737.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-775.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-850.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-852.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-855.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-856.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-857.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-860.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-861.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-862.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-863.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-864.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-865.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-866.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-869.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-874.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-875.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-1006.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-1026.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-1250.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-1251.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-1252.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-1253.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-1254.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-1255.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-1256.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-1257.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/cp-1258.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/koi8-r.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/koi8-ru.h
$(TOP_DIR)/Unicode/unicode.o: $(TOP_DIR)/Unicode/koi8-u.h
	cd $(@D) && $(MAKE) $(@F)


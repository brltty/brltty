# Dependencies for adlib.o:
adlib.o: adlib.c
adlib.o: $(TOP_DIR)/config.h
adlib.o: misc.h
adlib.o: system.h
adlib.o: adlib.h

# Dependencies for brl.o:
brl.o: brl.c
brl.o: $(TOP_DIR)/config.h
brl.o: misc.h
brl.o: system.h
brl.o: message.h
brl.o: brl.h
brl.o: brl_driver.h

# Dependencies for brltest.o:
brltest.o: brltest.c
brltest.o: $(TOP_DIR)/config.h
brltest.o: options.h
brltest.o: brl.h
brltest.o: misc.h
brltest.o: message.h
brltest.o: scr.h
brltest.o: defaults.h

# Dependencies for config.o:
config.o: config.c
config.o: $(TOP_DIR)/config.h
config.o: brl.h
config.o: spk.h
config.o: scr.h
config.o: contract.h
config.o: tunes.h
config.o: message.h
config.o: misc.h
config.o: options.h
config.o: brltty.h
config.o: defaults.h

# Dependencies for ctb_compile.o:
ctb_compile.o: ctb_compile.c
ctb_compile.o: $(TOP_DIR)/config.h
ctb_compile.o: misc.h
ctb_compile.o: contract.h
ctb_compile.o: ctb_definitions.h
ctb_compile.o: brl.h

# Dependencies for ctb_translate.o:
ctb_translate.o: ctb_translate.c
ctb_translate.o: $(TOP_DIR)/config.h
ctb_translate.o: contract.h
ctb_translate.o: ctb_definitions.h
ctb_translate.o: brl.h

# Dependencies for cut.o:
cut.o: cut.c
cut.o: $(TOP_DIR)/config.h
cut.o: scr.h
cut.o: tunes.h
cut.o: cut.h

# Dependencies for main.o:
main.o: main.c
main.o: $(TOP_DIR)/config.h
main.o: misc.h
main.o: message.h
main.o: tunes.h
main.o: contract.h
main.o: route.h
main.o: cut.h
main.o: scr.h
main.o: brl.h
main.o: spk.h
main.o: brltty.h
main.o: defaults.h

# Dependencies for misc.o:
misc.o: misc.c
misc.o: $(TOP_DIR)/config.h
misc.o: misc.h
misc.o: brl.h

# Dependencies for options.o:
options.o: options.c
options.o: $(TOP_DIR)/config.h
options.o: misc.h
options.o: options.h

# Dependencies for route.o:
route.o: route.c
route.o: $(TOP_DIR)/config.h
route.o: misc.h
route.o: scr.h
route.o: route.h

# Dependencies for scr.o:
scr.o: scr.cc
scr.o: $(TOP_DIR)/config.h
scr.o: scr.h
scr.o: help.h
scr.o: scr_base.h

# Dependencies for scr_base.o:
scr_base.o: scr_base.cc
scr_base.o: $(TOP_DIR)/config.h
scr_base.o: misc.h
scr_base.o: route.h
scr_base.o: scr.h
scr_base.o: help.h
scr_base.o: scr_base.h

# Dependencies for scr_linux.o:
scr_linux.o: scr_linux.cc
scr_linux.o: $(TOP_DIR)/config.h
scr_linux.o: misc.h
scr_linux.o: scr.h
scr_linux.o: help.h
scr_linux.o: scr_base.h
scr_linux.o: scr_linux.h

# Dependencies for scr_shm.o:
scr_shm.o: scr_shm.cc
scr_shm.o: $(TOP_DIR)/config.h
scr_shm.o: scr.h
scr_shm.o: help.h
scr_shm.o: scr_base.h
scr_shm.o: scr_shm.h

# Dependencies for scrtest.o:
scrtest.o: scrtest.c
scrtest.o: $(TOP_DIR)/config.h
scrtest.o: options.h
scrtest.o: scr.h

# Dependencies for spk.o:
spk.o: spk.c
spk.o: $(TOP_DIR)/config.h
spk.o: misc.h
spk.o: system.h
spk.o: spk.h
spk.o: spk_driver.h

# Dependencies for sys_hpux.o:
sys_hpux.o: sys_hpux.c
sys_hpux.o: $(TOP_DIR)/config.h
sys_hpux.o: misc.h
sys_hpux.o: system.h

# Dependencies for sys_linux.o:
sys_linux.o: sys_linux.c
sys_linux.o: $(TOP_DIR)/config.h
sys_linux.o: misc.h
sys_linux.o: system.h

# Dependencies for sys_solaris.o:
sys_solaris.o: sys_solaris.c
sys_solaris.o: $(TOP_DIR)/config.h
sys_solaris.o: misc.h
sys_solaris.o: system.h

# Dependencies for tbl2hex.o:
tbl2hex.o: tbl2hex.c
tbl2hex.o: $(TOP_DIR)/config.h
tbl2hex.o: options.h

# Dependencies for tones_adlib.o:
tones_adlib.o: tones_adlib.c
tones_adlib.o: $(TOP_DIR)/config.h
tones_adlib.o: misc.h
tones_adlib.o: tones.h
tones_adlib.o: adlib.h

# Dependencies for tones_dac.o:
tones_dac.o: tones_dac.c
tones_dac.o: $(TOP_DIR)/config.h
tones_dac.o: misc.h
tones_dac.o: system.h
tones_dac.o: tones.h

# Dependencies for tones_midi.o:
tones_midi.o: tones_midi.c
tones_midi.o: $(TOP_DIR)/config.h
tones_midi.o: brl.h
tones_midi.o: brltty.h
tones_midi.o: misc.h
tones_midi.o: system.h
tones_midi.o: tones.h

# Dependencies for tones_speaker.o:
tones_speaker.o: tones_speaker.c
tones_speaker.o: $(TOP_DIR)/config.h
tones_speaker.o: misc.h
tones_speaker.o: system.h
tones_speaker.o: tones.h

# Dependencies for tunes.o:
tunes.o: tunes.c
tunes.o: $(TOP_DIR)/config.h
tunes.o: misc.h
tunes.o: system.h
tunes.o: message.h
tunes.o: brl.h
tunes.o: brltty.h
tunes.o: tunes.h
tunes.o: tones.h

# Dependencies for tunetest.o:
tunetest.o: tunetest.c
tunetest.o: $(TOP_DIR)/config.h
tunetest.o: options.h
tunetest.o: tunes.h
tunetest.o: misc.h
tunetest.o: defaults.h
tunetest.o: brl.h
tunetest.o: brltty.h
tunetest.o: message.h

# Dependencies for txt2hlp.o:
txt2hlp.o: txt2hlp.c
txt2hlp.o: $(TOP_DIR)/config.h
txt2hlp.o: options.h
txt2hlp.o: misc.h
txt2hlp.o: help.h


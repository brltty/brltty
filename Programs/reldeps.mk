# Dependencies for adlib.o:
adlib.o: $(SRC_DIR)/adlib.c
adlib.o: $(BLD_TOP)config.h
adlib.o: $(SRC_DIR)/misc.h
adlib.o: $(SRC_DIR)/system.h
adlib.o: $(SRC_DIR)/adlib.h

# Dependencies for beeper.o:
beeper.o: $(SRC_DIR)/beeper.c
beeper.o: $(BLD_TOP)config.h
beeper.o: $(SRC_DIR)/misc.h
beeper.o: $(SRC_DIR)/system.h
beeper.o: $(SRC_DIR)/notes.h

# Dependencies for brl.o:
brl.o: $(SRC_DIR)/brl.c
brl.o: $(BLD_TOP)config.h
brl.o: $(SRC_DIR)/misc.h
brl.o: $(SRC_DIR)/system.h
brl.o: $(SRC_DIR)/message.h
brl.o: $(SRC_DIR)/brl.h
brl.o: $(SRC_DIR)/brl_driver.h

# Dependencies for brltest.o:
brltest.o: $(SRC_DIR)/brltest.c
brltest.o: $(BLD_TOP)config.h
brltest.o: $(SRC_DIR)/options.h
brltest.o: $(SRC_DIR)/brl.h
brltest.o: $(SRC_DIR)/misc.h
brltest.o: $(SRC_DIR)/message.h
brltest.o: $(SRC_DIR)/scr.h
brltest.o: $(SRC_DIR)/defaults.h

# Dependencies for config.o:
config.o: $(SRC_DIR)/config.c
config.o: $(BLD_TOP)config.h
config.o: $(SRC_DIR)/brl.h
config.o: $(SRC_DIR)/spk.h
config.o: $(SRC_DIR)/scr.h
config.o: $(SRC_DIR)/contract.h
config.o: $(SRC_DIR)/tunes.h
config.o: $(SRC_DIR)/message.h
config.o: $(SRC_DIR)/misc.h
config.o: $(SRC_DIR)/system.h
config.o: $(SRC_DIR)/options.h
config.o: $(SRC_DIR)/brltty.h
config.o: $(SRC_DIR)/defaults.h

# Dependencies for ctb_compile.o:
ctb_compile.o: $(SRC_DIR)/ctb_compile.c
ctb_compile.o: $(BLD_TOP)config.h
ctb_compile.o: $(SRC_DIR)/misc.h
ctb_compile.o: $(SRC_DIR)/contract.h
ctb_compile.o: $(SRC_DIR)/ctb_definitions.h
ctb_compile.o: $(SRC_DIR)/brl.h

# Dependencies for ctb_translate.o:
ctb_translate.o: $(SRC_DIR)/ctb_translate.c
ctb_translate.o: $(BLD_TOP)config.h
ctb_translate.o: $(SRC_DIR)/contract.h
ctb_translate.o: $(SRC_DIR)/ctb_definitions.h
ctb_translate.o: $(SRC_DIR)/brl.h

# Dependencies for cut.o:
cut.o: $(SRC_DIR)/cut.c
cut.o: $(BLD_TOP)config.h
cut.o: $(SRC_DIR)/scr.h
cut.o: $(SRC_DIR)/tunes.h
cut.o: $(SRC_DIR)/cut.h

# Dependencies for fm.o:
fm.o: $(SRC_DIR)/fm.c
fm.o: $(BLD_TOP)config.h
fm.o: $(SRC_DIR)/brl.h
fm.o: $(SRC_DIR)/brltty.h
fm.o: $(SRC_DIR)/misc.h
fm.o: $(SRC_DIR)/notes.h
fm.o: $(SRC_DIR)/adlib.h

# Dependencies for main.o:
main.o: $(SRC_DIR)/main.c
main.o: $(BLD_TOP)config.h
main.o: $(SRC_DIR)/misc.h
main.o: $(SRC_DIR)/message.h
main.o: $(SRC_DIR)/tunes.h
main.o: $(SRC_DIR)/contract.h
main.o: $(SRC_DIR)/route.h
main.o: $(SRC_DIR)/cut.h
main.o: $(SRC_DIR)/scr.h
main.o: $(SRC_DIR)/brl.h
main.o: $(SRC_DIR)/spk.h
main.o: $(SRC_DIR)/brltty.h
main.o: $(SRC_DIR)/defaults.h

# Dependencies for midi.o:
midi.o: $(SRC_DIR)/midi.c
midi.o: $(BLD_TOP)config.h
midi.o: $(SRC_DIR)/brl.h
midi.o: $(SRC_DIR)/brltty.h
midi.o: $(SRC_DIR)/misc.h
midi.o: $(SRC_DIR)/system.h
midi.o: $(SRC_DIR)/notes.h

# Dependencies for misc.o:
misc.o: $(SRC_DIR)/misc.c
misc.o: $(BLD_TOP)config.h
misc.o: $(SRC_DIR)/misc.h
misc.o: $(SRC_DIR)/brl.h

# Dependencies for options.o:
options.o: $(SRC_DIR)/options.c
options.o: $(BLD_TOP)config.h
options.o: $(SRC_DIR)/misc.h
options.o: $(SRC_DIR)/options.h

# Dependencies for pcm.o:
pcm.o: $(SRC_DIR)/pcm.c
pcm.o: $(BLD_TOP)config.h
pcm.o: $(SRC_DIR)/brl.h
pcm.o: $(SRC_DIR)/brltty.h
pcm.o: $(SRC_DIR)/misc.h
pcm.o: $(SRC_DIR)/system.h
pcm.o: $(SRC_DIR)/notes.h

# Dependencies for route.o:
route.o: $(SRC_DIR)/route.c
route.o: $(BLD_TOP)config.h
route.o: $(SRC_DIR)/misc.h
route.o: $(SRC_DIR)/scr.h
route.o: $(SRC_DIR)/route.h

# Dependencies for scr.o:
scr.o: $(SRC_DIR)/scr.cc
scr.o: $(BLD_TOP)config.h
scr.o: $(SRC_DIR)/scr.h
scr.o: $(SRC_DIR)/scr_base.h
scr.o: $(SRC_DIR)/scr_frozen.h
scr.o: $(SRC_DIR)/help.h
scr.o: $(SRC_DIR)/scr_help.h
scr.o: $(SRC_DIR)/scr_real.h

# Dependencies for scr_base.o:
scr_base.o: $(SRC_DIR)/scr_base.cc
scr_base.o: $(BLD_TOP)config.h
scr_base.o: $(SRC_DIR)/misc.h
scr_base.o: $(SRC_DIR)/scr.h
scr_base.o: $(SRC_DIR)/scr_base.h

# Dependencies for scr_frozen.o:
scr_frozen.o: $(SRC_DIR)/scr_frozen.cc
scr_frozen.o: $(BLD_TOP)config.h
scr_frozen.o: $(SRC_DIR)/misc.h
scr_frozen.o: $(SRC_DIR)/scr.h
scr_frozen.o: $(SRC_DIR)/scr_base.h
scr_frozen.o: $(SRC_DIR)/scr_frozen.h

# Dependencies for scr_help.o:
scr_help.o: $(SRC_DIR)/scr_help.cc
scr_help.o: $(BLD_TOP)config.h
scr_help.o: $(SRC_DIR)/misc.h
scr_help.o: $(SRC_DIR)/scr.h
scr_help.o: $(SRC_DIR)/help.h
scr_help.o: $(SRC_DIR)/scr_base.h
scr_help.o: $(SRC_DIR)/scr_help.h

# Dependencies for scr_linux.o:
scr_linux.o: $(SRC_DIR)/scr_linux.cc
scr_linux.o: $(BLD_TOP)config.h
scr_linux.o: $(SRC_DIR)/misc.h
scr_linux.o: $(SRC_DIR)/scr.h
scr_linux.o: $(SRC_DIR)/scr_base.h
scr_linux.o: $(SRC_DIR)/scr_linux.h
scr_linux.o: $(SRC_DIR)/scr_real.h

# Dependencies for scr_real.o:
scr_real.o: $(SRC_DIR)/scr_real.cc
scr_real.o: $(BLD_TOP)config.h
scr_real.o: $(SRC_DIR)/misc.h
scr_real.o: $(SRC_DIR)/route.h
scr_real.o: $(SRC_DIR)/scr.h
scr_real.o: $(SRC_DIR)/scr_base.h
scr_real.o: $(SRC_DIR)/scr_real.h

# Dependencies for scr_shm.o:
scr_shm.o: $(SRC_DIR)/scr_shm.cc
scr_shm.o: $(BLD_TOP)config.h
scr_shm.o: $(SRC_DIR)/scr.h
scr_shm.o: $(SRC_DIR)/scr_base.h
scr_shm.o: $(SRC_DIR)/scr_real.h
scr_shm.o: $(SRC_DIR)/scr_shm.h

# Dependencies for scrtest.o:
scrtest.o: $(SRC_DIR)/scrtest.c
scrtest.o: $(BLD_TOP)config.h
scrtest.o: $(SRC_DIR)/options.h
scrtest.o: $(SRC_DIR)/scr.h

# Dependencies for spk.o:
spk.o: $(SRC_DIR)/spk.c
spk.o: $(BLD_TOP)config.h
spk.o: $(SRC_DIR)/misc.h
spk.o: $(SRC_DIR)/system.h
spk.o: $(SRC_DIR)/spk.h
spk.o: $(SRC_DIR)/spk_driver.h

# Dependencies for sys_hpux.o:
sys_hpux.o: $(SRC_DIR)/sys_hpux.c
sys_hpux.o: $(BLD_TOP)config.h
sys_hpux.o: $(SRC_DIR)/misc.h
sys_hpux.o: $(SRC_DIR)/system.h

# Dependencies for sys_linux.o:
sys_linux.o: $(SRC_DIR)/sys_linux.c
sys_linux.o: $(BLD_TOP)config.h
sys_linux.o: $(SRC_DIR)/misc.h
sys_linux.o: $(SRC_DIR)/system.h

# Dependencies for sys_solaris.o:
sys_solaris.o: $(SRC_DIR)/sys_solaris.c
sys_solaris.o: $(BLD_TOP)config.h
sys_solaris.o: $(SRC_DIR)/misc.h
sys_solaris.o: $(SRC_DIR)/system.h

# Dependencies for tbl2hex.o:
tbl2hex.o: $(SRC_DIR)/tbl2hex.c
tbl2hex.o: $(BLD_TOP)config.h
tbl2hex.o: $(SRC_DIR)/options.h

# Dependencies for tunes.o:
tunes.o: $(SRC_DIR)/tunes.c
tunes.o: $(BLD_TOP)config.h
tunes.o: $(SRC_DIR)/misc.h
tunes.o: $(SRC_DIR)/system.h
tunes.o: $(SRC_DIR)/message.h
tunes.o: $(SRC_DIR)/brl.h
tunes.o: $(SRC_DIR)/brltty.h
tunes.o: $(SRC_DIR)/tunes.h
tunes.o: $(SRC_DIR)/notes.h

# Dependencies for tunetest.o:
tunetest.o: $(SRC_DIR)/tunetest.c
tunetest.o: $(BLD_TOP)config.h
tunetest.o: $(SRC_DIR)/options.h
tunetest.o: $(SRC_DIR)/tunes.h
tunetest.o: $(SRC_DIR)/misc.h
tunetest.o: $(SRC_DIR)/defaults.h
tunetest.o: $(SRC_DIR)/brl.h
tunetest.o: $(SRC_DIR)/brltty.h
tunetest.o: $(SRC_DIR)/message.h

# Dependencies for txt2hlp.o:
txt2hlp.o: $(SRC_DIR)/txt2hlp.c
txt2hlp.o: $(BLD_TOP)config.h
txt2hlp.o: $(SRC_DIR)/options.h
txt2hlp.o: $(SRC_DIR)/misc.h
txt2hlp.o: $(SRC_DIR)/help.h


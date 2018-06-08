Brltty support for dracut

This module provides brltty functionality in the initrd.
Module pickups user settings from system and install
necessary files like drivers and used text tables to initramfs.

For now the module is reliably functional from phase pre-mount
in earlier phases the module is not functional

TBD fix functionality from earlier phases than pre-mount

Module in instalation takes some options from environment variables.

BRLTTY_DRACUT_INCLUDE_DRIVERS forces include of screen or braille
driver
example:
BRLTTY_DRACUT_INCLUDE_DRIVERS=bpm beu

BRLTTY_DRACUT_INCLUDE_TEXT_FILES forces include of text tables
example:
BRLTTY_DRACUT_INCLUDE_TEXT_FILES=kok.ttb lv.tti

BRLTTY_LOCALE needs to be used when building initram image
to install used text table 
see https://bugzilla.redhat.com/show_bug.cgi?id=1584036
example
BRLTTY_LOCALE=cs_CZ.UTF-8


Module adds boot command line parameters which are parsed
and exported as a coresponding environment variables. The following
is a list of supported boot command line parameters and their mapping
to the environment variables recognized by brltty (for details about
the variables see man brltty):

Boot command line parameter         Environment variable
brltty.api_parameters               BRLTTY_API_PARAMETERS
brltty.attributes_table             BRLTTY_ATTRIBUTES_TABLE
brltty.braille_device               BRLTTY_BRAILLE_DEVICE
brltty.braille_driver               BRLTTY_BRAILLE_DRIVER
brltty.braille_parameters           BRLTTY_BRAILLE_PARAMETERS
brltty.configuration_file           BRLTTY_CONFIGURATION_FILE
brltty.contraction_table            BRLTTY_CONTRACTION_TABLE
brltty.midi_device                  BRLTTY_MIDI_DEVICE
brltty.pcm_device                   BRLTTY_PCM_DEVICE
brltty.preferences_file             BRLTTY_PREFERENCES_FILE
brltty.release_device               BRLTTY_RELEASE_DEVICE
brltty.screen_driver                BRLTTY_SCREEN_DRIVER
brltty.screen_parameters            BRLTTY_SCREEN_PARAMETERS
brltty.speech_driver                BRLTTY_SPEECH_DRIVER
brltty.speech_input                 BRLTTY_SPEECH_INPUT
brltty.speech_parameters            BRLTTY_SPEECH_PARAMETERS
brltty.text_table                   BRLTTY_TEXT_TABLE

Example:
brltty.braile_driver="ba" brltty.braille_parameters="auth=none,host=IP:0"

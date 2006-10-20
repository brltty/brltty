/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>

struct MidiDeviceStruct {
  AUGraph graph;
  AudioUnit synth;
  /* Note that is currently playing. */
  int note;
};
  

MidiDevice *
openMidiDevice (int errorLevel, const char *device) {
  MidiDevice *midi;

  if ((midi = malloc(sizeof(*midi)))) {
    /* Create a graph with a synth and a default output unit. */
    AUNode synthNode, outNode;
    ComponentDescription cd;

    cd.componentManufacturer = kAudioUnitManufacturer_Apple;
    cd.componentFlags = 0;
    cd.componentFlagsMask = 0;

    NewAUGraph(&midi->graph);

    cd.componentType = kAudioUnitType_MusicDevice;
    cd.componentSubType = kAudioUnitSubType_DLSSynth;
    AUGraphNewNode(midi->graph, &cd, 0, NULL, &synthNode);

    cd.componentType = kAudioUnitType_Output;
    cd.componentSubType = kAudioUnitSubType_DefaultOutput;
    AUGraphNewNode(midi->graph, &cd, 0, NULL, &outNode);

    AUGraphOpen(midi->graph);
    AUGraphConnectNodeInput(midi->graph, synthNode, 0, outNode, 0);

    AUGraphGetNodeInfo(midi->graph, synthNode, 0, 0, 0, &midi->synth);

    AUGraphInitialize(midi->graph);

    /* Turn off the reverb.  The value range is -120 to 40 dB. */
    AudioUnitSetParameter(midi->synth, kMusicDeviceParam_ReverbVolume,
			  kAudioUnitScope_Global, 0, -120, 0);

    /* TODO: Maybe just start the graph when we are going to use it? */
    AUGraphStart(midi->graph);
  }

  return midi;
}

void
closeMidiDevice (MidiDevice *midi) {
  if (midi) {
    DisposeAUGraph(midi->graph);
    free(midi);
  }
}

int
flushMidiDevice (MidiDevice *midi) {
  return 1;
}

int
setMidiInstrument (MidiDevice *midi, unsigned char channel, unsigned char instrument) {
  MusicDeviceMIDIEvent(midi->synth, 0xC0 | channel, instrument, 0, 0);

  return 1;
}

int
beginMidiBlock (MidiDevice *midi) {
  return 1;
}

int
endMidiBlock (MidiDevice *midi) {
  return 1;
}

int
startMidiNote (MidiDevice *midi, unsigned char channel, unsigned char note, unsigned char volume) {
  MusicDeviceMIDIEvent(midi->synth, 0x90 | channel, note, volume, 0);
  midi->note = note;

  return 1;
}

int
stopMidiNote (MidiDevice *midi, unsigned char channel) {
  return startMidiNote(midi, channel, midi->note, 0);
}

int
insertMidiWait (MidiDevice *midi, int duration) {
  accurateDelay(duration);
  return 1;
}

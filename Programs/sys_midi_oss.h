/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

static MidiBufferFlusher flushMidiBuffer;
static int midiDevice;

#ifndef SAMPLE_TYPE_AWE
#define SAMPLE_TYPE_AWE 0x20
#endif /* SAMPLE_TYPE_AWE */

int
getMidiDevice (int errorLevel, MidiBufferFlusher flushBuffer) {
  const char *path = "/dev/sequencer";
  int descriptor = open(path, O_WRONLY);
  if (descriptor == -1) {
    LogPrint(errorLevel, "Cannot open MIDI device: %s: %s", path, strerror(errno));
  } else {
    flushMidiBuffer = flushBuffer;

    {
      int count;
      int awe = -1;
      int fm = -1;
      int gus = -1;
      int ext = -1;

      if (ioctl(descriptor, SNDCTL_SEQ_NRSYNTHS, &count) != -1) {
        int index;
        for (index=0; index<count; ++index) {
          struct synth_info info;
          info.device = index;
          if (ioctl(descriptor, SNDCTL_SYNTH_INFO, &info) != -1) {
            switch (info.synth_type) {
              case SYNTH_TYPE_SAMPLE:
                switch (info.synth_subtype) {
                  case SAMPLE_TYPE_AWE:
                    awe = index;
                    continue;
                  case SAMPLE_TYPE_GUS:
                    gus = index;
                    continue;
                }
                break;
              case SYNTH_TYPE_FM:
                fm = index;
                continue;
            }
            LogPrint(LOG_DEBUG, "Unknown synthesizer: %d[%d]: %s",
                     info.synth_type, info.synth_subtype, info.name);
          } else {
            LogPrint(errorLevel, "Cannot get description for synthesizer %d: %s",
                     index, strerror(errno));
          }
        }

        if (gus >= 0)
          if (ioctl(descriptor, SNDCTL_SEQ_RESETSAMPLES, &gus) == -1)
            LogPrint(errorLevel, "Cannot reset samples for gus synthesizer %d: %s",
                     gus, strerror(errno));
      } else {
        LogPrint(errorLevel, "Cannot get MIDI synthesizer count: %s",
                 strerror(errno));
      }

      if (ioctl(descriptor, SNDCTL_SEQ_NRMIDIS, &count) != -1) {
        if (count > 0) ext = count - 1;
      } else {
        LogPrint(errorLevel, "Cannot get MIDI device count: %s",
                 strerror(errno));
      }

      midiDevice = (awe >= 0)? awe:
                   (gus >= 0)? gus:
                   (fm >= 0)? fm:
                   (ext >= 0)? ext:
                   0;
    }
  }
  return descriptor;
}

SEQ_DEFINEBUF(0X80);
void
seqbuf_dump (void) {
  flushMidiBuffer(_seqbuf, _seqbufptr);
  _seqbufptr = 0;
}

void
setMidiInstrument (unsigned char channel, unsigned char instrument) {
  SEQ_SET_PATCH(midiDevice, channel, instrument);
}

void
beginMidiBlock (int descriptor) {
  SEQ_START_TIMER();
}

void
endMidiBlock (int descriptor) {
  SEQ_STOP_TIMER();
  seqbuf_dump();
  ioctl(descriptor, SNDCTL_SEQ_SYNC);
}

void
startMidiNote (unsigned char channel, unsigned char note, unsigned char volume) {
  SEQ_START_NOTE(midiDevice, channel, note, 0X7F*volume/100);
}

void
stopMidiNote (unsigned char channel) {
  SEQ_STOP_NOTE(midiDevice, channel, 0, 0);
}

void
insertMidiWait (int duration) {
  SEQ_DELTA_TIME((duration + 9) / 10);
}

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

#include <alsa/asoundlib.h>

struct MidiDeviceStruct {
  snd_seq_t    *handle;
  int           port;
  int           queue;
  unsigned char notePlaying;
  int           noteon_duration;
  int           noteoff_duration;
};

MidiDevice *
openMidiDevice (int errorLevel) {
  MidiDevice *midi;

  if ((midi = malloc(sizeof(*midi)))) {
    int result;

    if ((result = snd_seq_open(&midi->handle, "default",
			       SND_SEQ_OPEN_OUTPUT, 0)) == 0) {
      snd_seq_set_client_name(midi->handle, PACKAGE_TITLE);

      if ((midi->port =
	   snd_seq_create_simple_port(
	    midi->handle, "out0",
	    SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
	    SND_SEQ_PORT_TYPE_APPLICATION)) >= 0) {
	if ((midi->queue = snd_seq_alloc_queue(midi->handle)) >= 0) {
	  if ((result = snd_seq_connect_to(midi->handle, midi->port, 65, 0)) == 0) {
	    if ((result = snd_seq_start_queue(midi->handle, midi->queue, NULL)) >= 0) {
	      midi->noteon_duration = midi->noteoff_duration = 0;

	      return midi;
	    } else {
	      LogPrint(errorLevel, "Failed to start queue: %s", snd_strerror(result));
	    }
	  } else {
	    LogPrint(errorLevel, "Unable to connect to: %s", snd_strerror(result));
	  }
	} else {
	  LogPrint(errorLevel, "Unable to allocate queue: %s", snd_strerror(result));
	}
      } else {
	LogPrint(errorLevel, "Can't open output port: %s", snd_strerror(midi->port));
      }

      snd_seq_close(midi->handle);
    } else {
      LogPrint(errorLevel, "Cannot open MIDI device: %s", snd_strerror(result));
    }

    free(midi);
  } else {
    LogError("MIDI device allocation");
  }
  return NULL;
}

void
closeMidiDevice (MidiDevice *midi) {
  snd_seq_close(midi->handle);
  free(midi);
}

int
flushMidiDevice (MidiDevice *midi) {
  return 1;
}

int
setMidiInstrument (MidiDevice *midi, unsigned char channel, unsigned char instrument) {
  snd_seq_event_t ev;
  int result;

  snd_seq_ev_set_source(&ev, midi->port);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_pgmchange(&ev, channel, instrument);
  if ((result = snd_seq_event_output(midi->handle, &ev)) >= 0) {
    snd_seq_drain_output(midi->handle);
    return 1;
  } else {
    LogPrint(LOG_DEBUG, "Write error: %s", snd_strerror(result));
  }
  return 0;
}

int
beginMidiBlock (MidiDevice *midi) {
  midi->noteoff_duration = midi->notePlaying = 0;
  return 1;
}

int
endMidiBlock (MidiDevice *midi) {
  midi->noteoff_duration = 0;
  return 1;
}

int
startMidiNote (MidiDevice *midi, unsigned char channel, unsigned char note, unsigned char volume) {
  snd_seq_event_t ev;
  int result;

  snd_seq_ev_set_source(&ev, midi->port);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_noteon(&ev, channel, note, volume);
  midi->notePlaying = note;
  if (midi->noteon_duration) {
    snd_seq_real_time_t time;
    time.tv_sec = (unsigned)(midi->noteon_duration / 1000);
    time.tv_nsec = (unsigned)(midi->noteon_duration * 1000);
    snd_seq_ev_schedule_real(&ev, midi->queue, 1, &time);
    midi->noteon_duration = 0;
  }
  if ((result = snd_seq_event_output(midi->handle, &ev)) == 0) {
    snd_seq_drain_output(midi->handle);
    return 1;
  } else {
    LogPrint(LOG_DEBUG, "Write error: %s", snd_strerror(result));
  }
  return 0;
}

int
stopMidiNote (MidiDevice *midi, unsigned char channel) {
  snd_seq_event_t ev;
  int result;

  snd_seq_ev_set_source(&ev, midi->port);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_noteoff(&ev, channel, midi->notePlaying, 0);
  midi->notePlaying = 0;
  if (midi->noteoff_duration) {
    snd_seq_real_time_t time;
    time.tv_sec = (unsigned)(midi->noteoff_duration / 1000);
    time.tv_nsec = (unsigned)(midi->noteoff_duration * 1000);
    snd_seq_ev_schedule_real(&ev, midi->queue, 1, &time);
    midi->noteoff_duration = 0;
  }
  if ((result = snd_seq_event_output(midi->handle, &ev)) == 0) {
    snd_seq_drain_output(midi->handle);
    return 1;
  } else {
    LogPrint(LOG_DEBUG, "Write error: %s", snd_strerror(result));
  }
  return 0;
}

int
insertMidiWait (MidiDevice *midi, int duration) {
  if (midi->notePlaying) {
    midi->noteoff_duration = duration;
  } else {
    midi->noteon_duration = duration;
  }
  return 1;
}

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
  snd_seq_t    *sequencer;
  int           port;
  int           queue;
  unsigned char note;
  int           duration;
};

static int
findMidiDevice (MidiDevice *midi, int *client, int *port) {
  snd_seq_client_info_t *clientInformation = mallocWrapper(snd_seq_client_info_sizeof());
  memset(clientInformation, 0, snd_seq_client_info_sizeof());
  snd_seq_client_info_set_client(clientInformation, -1);

  while (snd_seq_query_next_client(midi->sequencer, clientInformation) >= 0) {
    int clientIdentifier = snd_seq_client_info_get_client(clientInformation);

    snd_seq_port_info_t *portInformation = mallocWrapper(snd_seq_port_info_sizeof());
    memset(portInformation, 0, snd_seq_port_info_sizeof());
    snd_seq_port_info_set_client(portInformation, clientIdentifier);
    snd_seq_port_info_set_port(portInformation, -1);

    while (snd_seq_query_next_port(midi->sequencer, portInformation) >= 0) {
      int portIdentifier = snd_seq_port_info_get_port(portInformation);
      int actualCapabilities = snd_seq_port_info_get_capability(portInformation);
      const int neededCapabilties = SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;

      if (((actualCapabilities & neededCapabilties) == neededCapabilties) &&
          !(actualCapabilities & SND_SEQ_PORT_CAP_NO_EXPORT)) {
        *client = clientIdentifier;
        *port = portIdentifier;

        free(portInformation);
        free(clientInformation);
        return 1;
      }
    }

    free(portInformation);
  }

  free(clientInformation);
  return 0;
}

static int
parseMidiDevice (MidiDevice *midi, int errorLevel, const char *device, int *client, int *port) {
  char **components = splitString(device, ':', NULL);
  char **component = components;
  if (*component && **component) {
    const char *clientSpecifier = *component++;

    if (*component && **component) {
      const char *portSpecifier = *component++;

      if (!*component) {
        int clientIdentifier;
        int clientOk = 0;

        if (isInteger(&clientIdentifier, clientSpecifier)) {
          if ((clientIdentifier >= 0) && (clientIdentifier <= 0XFFFF)) clientOk = 1;
        } else {
        /*
          snd_seq_client_info_t *info = mallocWrapper(snd_seq_client_info_sizeof());
          memset(info, 0, snd_seq_client_info_sizeof());
          snd_seq_client_info_set_client(info, -1);
          snd_seq_client_info_set_name(info, clientSpecifier);
          if (snd_seq_query_next_client(midi->sequencer, info) >= 0) {
            clientIdentifier = snd_seq_client_info_get_client(info);
            clientOk = 1;
          }
          free(info);
        */
        }

        if (clientOk) {
          int portIdentifier;
          int portOk = 0;

          if (isInteger(&portIdentifier, portSpecifier)) {
            if ((portIdentifier >= 0) && (portIdentifier <= 0XFFFF)) portOk = 1;
          }

          if (portOk) {
            *client = clientIdentifier;
            *port = portIdentifier;
            return 1;
          } else {
            LogPrint(errorLevel, "Invalid ALSA MIDI port: %s", device);
          }
        } else {
          LogPrint(errorLevel, "Invalid ALSA MIDI client: %s", device);
        }
      } else {
        LogPrint(errorLevel, "Too many ALSA MIDI device components: %s", device);
      }
    } else {
      LogPrint(errorLevel, "Missing ALSA MIDI port specifier: %s", device);
    }
  } else {
    LogPrint(errorLevel, "Missing ALSA MIDI client specifier: %s", device);
  }

  deallocateStrings(components);
  return 0;
}

MidiDevice *
openMidiDevice (int errorLevel, const char *device) {
  MidiDevice *midi;

  if ((midi = malloc(sizeof(*midi)))) {
    const char *sequencerName = "default";
    int result;

    if ((result = snd_seq_open(&midi->sequencer, sequencerName,
                               SND_SEQ_OPEN_OUTPUT, 0)) >= 0) {
      snd_seq_set_client_name(midi->sequencer, PACKAGE_TITLE);

      if ((midi->port = snd_seq_create_simple_port(midi->sequencer, "out0",
                                                   SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
                                                   SND_SEQ_PORT_TYPE_APPLICATION)) >= 0) {
        if ((midi->queue = snd_seq_alloc_queue(midi->sequencer)) >= 0) {
          int client;
          int port;
          int deviceOk;

          if (device) {
            deviceOk = parseMidiDevice(midi, errorLevel, device, &client, &port);
          } else {
            deviceOk = findMidiDevice(midi, &client, &port);
          }

          if (deviceOk) {
            LogPrint(LOG_DEBUG, "Connecting to ALSA MIDI device: %d:%d", client, port);

            if ((result = snd_seq_connect_to(midi->sequencer, midi->port, client, port)) >= 0) {
              if ((result = snd_seq_start_queue(midi->sequencer, midi->queue, NULL)) >= 0) {
                midi->duration = 0;

                return midi;
              } else {
                LogPrint(errorLevel, "Cannot start ALSA MIDI queue: %d:%d: %s",
                         client, port, snd_strerror(result));
              }
            } else {
              LogPrint(errorLevel, "Cannot connect to ALSA MIDI device: %d:%d: %s",
                       client, port, snd_strerror(result));
            }
          }
        } else {
          LogPrint(errorLevel, "Cannot allocate ALSA MIDI queue: %s",
                   snd_strerror(result));
        }
      } else {
        LogPrint(errorLevel, "Cannot create ALSA MIDI output port: %s",
                 snd_strerror(midi->port));
      }

      snd_seq_close(midi->sequencer);
    } else {
      LogPrint(errorLevel, "Cannot open ALSA sequencer: %s: %s",
               sequencerName, snd_strerror(result));
    }

    free(midi);
  } else {
    LogError("MIDI device allocation");
  }
  return NULL;
}

void
closeMidiDevice (MidiDevice *midi) {
  snd_seq_close(midi->sequencer);
  free(midi);
}

int
flushMidiDevice (MidiDevice *midi) {
  return 1;
}

static void
prepareMidiEvent (MidiDevice *midi, snd_seq_event_t *event) {
  snd_seq_ev_clear(event);
  snd_seq_ev_set_source(event, midi->port);
  snd_seq_ev_set_subs(event);
}

static void
scheduleMidiEvent (MidiDevice *midi, snd_seq_event_t *event) {
  if (midi->duration) {
    snd_seq_real_time_t time;
    time.tv_sec = midi->duration / 1000;
    time.tv_nsec = (midi->duration % 1000) * 1000000;
    snd_seq_ev_schedule_real(event, midi->queue, 1, &time);
    midi->duration = 0;
  }
}

static int
sendMidiEvent (MidiDevice *midi, snd_seq_event_t *event) {
  int result;

  if ((result = snd_seq_event_output(midi->sequencer, event)) >= 0) {
    snd_seq_drain_output(midi->sequencer);
    return 1;
  } else {
    LogPrint(LOG_ERR, "ALSA MIDI write error: %s", snd_strerror(result));
  }
  return 0;
}

int
setMidiInstrument (MidiDevice *midi, unsigned char channel, unsigned char instrument) {
  snd_seq_event_t event;

  prepareMidiEvent(midi, &event);
  snd_seq_ev_set_pgmchange(&event, channel, instrument);
  return sendMidiEvent(midi, &event);
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
  snd_seq_event_t event;

  prepareMidiEvent(midi, &event);
  snd_seq_ev_set_noteon(&event, channel, note, volume);
  midi->note = note;
  scheduleMidiEvent(midi, &event);
  return sendMidiEvent(midi, &event);
}

int
stopMidiNote (MidiDevice *midi, unsigned char channel) {
  snd_seq_event_t event;

  prepareMidiEvent(midi, &event);
  snd_seq_ev_set_noteoff(&event, channel, midi->note, 0);
  midi->note = 0;
  scheduleMidiEvent(midi, &event);
  return sendMidiEvent(midi, &event);
}

int
insertMidiWait (MidiDevice *midi, int duration) {
  midi->duration += duration;
  return 1;
}

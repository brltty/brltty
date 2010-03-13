/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_SYSTEM
#define BRLTTY_INCLUDED_SYSTEM

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


extern char *getProgramPath (void);


extern char *getBootParameters (const char *name);


extern int executeHostCommand (const char *const *arguments);


extern int mountFileSystem (const char *path, const char *reference, const char *type);


extern void *loadSharedObject (const char *path);
extern void unloadSharedObject (void *object);
extern int findSharedSymbol (void *object, const char *symbol, void *pointerAddress);


extern int canBeep (void);
extern int synchronousBeep (unsigned short frequency, unsigned short milliseconds);
extern int asynchronousBeep (unsigned short frequency, unsigned short milliseconds);
extern int startBeep (unsigned short frequency);
extern int stopBeep (void);
extern void endBeep (void);


typedef struct PcmDeviceStruct PcmDevice;

typedef enum {
  PCM_FMT_U8,
  PCM_FMT_S8,
  PCM_FMT_U16B,
  PCM_FMT_S16B,
  PCM_FMT_U16L,
  PCM_FMT_S16L,
  PCM_FMT_ULAW,
  PCM_FMT_ALAW,
  PCM_FMT_UNKNOWN
} PcmAmplitudeFormat;

extern PcmDevice *openPcmDevice (int errorLevel, const char *device);
extern void closePcmDevice (PcmDevice *pcm);
extern int writePcmData (PcmDevice *pcm, const unsigned char *buffer, int count);
extern int getPcmBlockSize (PcmDevice *pcm);
extern int getPcmSampleRate (PcmDevice *pcm);
extern int setPcmSampleRate (PcmDevice *pcm, int rate);
extern int getPcmChannelCount (PcmDevice *pcm);
extern int setPcmChannelCount (PcmDevice *pcm, int channels);
extern PcmAmplitudeFormat getPcmAmplitudeFormat (PcmDevice *pcm);
extern PcmAmplitudeFormat setPcmAmplitudeFormat (PcmDevice *pcm, PcmAmplitudeFormat format);
extern void forcePcmOutput (PcmDevice *pcm);
extern void awaitPcmOutput (PcmDevice *pcm);
extern void cancelPcmOutput (PcmDevice *pcm);


typedef struct MidiDeviceStruct MidiDevice;
typedef void (*MidiBufferFlusher) (unsigned char *buffer, int count);

extern MidiDevice *openMidiDevice (int errorLevel, const char *device);
extern void closeMidiDevice (MidiDevice *midi);
extern int flushMidiDevice (MidiDevice *midi);
extern int setMidiInstrument (MidiDevice *midi, unsigned char channel, unsigned char instrument);
extern int beginMidiBlock (MidiDevice *midi);
extern int endMidiBlock (MidiDevice *midi);
extern int startMidiNote (MidiDevice *midi, unsigned char channel, unsigned char note, unsigned char volume);
extern int stopMidiNote (MidiDevice *midi, unsigned char channel);
extern int insertMidiWait (MidiDevice *midi, int duration);


extern int enablePorts (int errorLevel, unsigned short int base, unsigned short int count);
extern int disablePorts (unsigned short int base, unsigned short int count);
extern unsigned char readPort1 (unsigned short int port);
extern void writePort1 (unsigned short int port, unsigned char value);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SYSTEM */

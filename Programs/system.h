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

#ifndef _SYSTEM_H
#define _SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern char *getBootParameters (void);

extern void *loadSharedObject (const char *path);
extern void unloadSharedObject (void *object);
extern int findSharedSymbol (void *object, const char *symbol, const void **address);

extern int canBeep (void);
extern int timedBeep (unsigned short frequency, unsigned short milliseconds);
extern int startBeep (unsigned short frequency);
extern int stopBeep (void);

typedef enum {
  PCM_FMT_U8,
  PCM_FMT_S8,
  PCM_FMT_U16B,
  PCM_FMT_S16B,
  PCM_FMT_U16L,
  PCM_FMT_S16L,
  PCM_FMT_ULAW,
  PCM_FMT_UNKNOWN
} PcmAmplitudeFormat;
extern int getPcmDevice (int errorLevel);
extern int getPcmBlockSize (int descriptor);
extern int getPcmSampleRate (int descriptor);
extern int getPcmChannelCount (int descriptor);
extern PcmAmplitudeFormat getPcmAmplitudeFormat (int descriptor);

typedef void (*MidiBufferFlusher) (unsigned char *buffer, int count);
extern int getMidiDevice (int errorLevel, MidiBufferFlusher flushBuffer);
extern void setMidiInstrument (unsigned char channel, unsigned char instrument);
extern void beginMidiBlock (int descriptor);
extern void endMidiBlock (int descriptor);
extern void startMidiNote (unsigned char channel, unsigned char note, unsigned char volume);
extern void stopMidiNote (unsigned char channel);
extern void insertMidiWait (int duration);

extern int enablePorts (int errorLevel, unsigned short int base, unsigned short int count);
extern int disablePorts (unsigned short int base, unsigned short int count);
extern unsigned char readPort1 (unsigned short int port);
extern void writePort1 (unsigned short int port, unsigned char value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYSTEM_H */

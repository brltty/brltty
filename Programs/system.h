/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

extern void *loadSharedObject (const char *path);
extern void unloadSharedObject (void *object);
extern int findSharedSymbol (void *object, const char *symbol, const void **address);

extern int canBeep (void);
extern int timedBeep (unsigned short frequency, unsigned short milliseconds);
extern int startBeep (unsigned short frequency);
extern int stopBeep (void);

typedef enum {
  DAC_FMT_U8,
  DAC_FMT_S8,
  DAC_FMT_U16B,
  DAC_FMT_S16B,
  DAC_FMT_U16L,
  DAC_FMT_S16L,
  DAC_FMT_ULAW,
  DAC_FMT_UNKNOWN
} DacAmplitudeFormat;
extern int getDacDevice (void);
extern int getDacBlockSize (int descriptor);
extern int getDacSampleRate (int descriptor);
extern int getDacChannelCount (int descriptor);
extern DacAmplitudeFormat getDacAmplitudeFormat (int descriptor);

typedef void (*MidiBufferFlusher) (unsigned char *buffer, int count);
extern int getMidiDevice (MidiBufferFlusher flushBuffer);
extern void setMidiInstrument (unsigned char device, unsigned char channel, unsigned char instrument);
extern void beginMidiBlock (int descriptor);
extern void endMidiBlock (int descriptor);
extern void startMidiNote (unsigned char device, unsigned char channel, unsigned char note);
extern void stopMidiNote (unsigned char device, unsigned char channel, unsigned char note);
extern void insertMidiWait (int duration);

extern int enablePorts (unsigned short int base, unsigned short int count);
extern int disablePorts (unsigned short int base, unsigned short int count);
extern unsigned char readPort1 (unsigned short int port);
extern void writePort1 (unsigned short int port, unsigned char value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYSTEM_H */

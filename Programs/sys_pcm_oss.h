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

#include "iomisc.h"

struct PcmDeviceStruct {
  int fileDescriptor;
};

PcmDevice *
openPcmDevice (int errorLevel, const char *device) {
  PcmDevice *pcm;
  if ((pcm = malloc(sizeof(*pcm)))) {
    if (!device) device = PCM_OSS_DEVICE_PATH;
    if ((pcm->fileDescriptor = open(device, O_WRONLY|O_NONBLOCK)) != -1) {
      /* Nonblocking if snd_seq_oss is loaded with nonblock_open=1.
       * There appears to be a bug in this case as write() always
       * returns the full count even though large chunks of sound are
       * missing. For now, therefore, force blocking output.
       */
      setBlockingIo(pcm->fileDescriptor, 1);

      return pcm;
    } else {
      LogPrint(errorLevel, "Cannot open PCM device: %s: %s", device, strerror(errno));
    }
    free(pcm);
  } else {
    LogError("PCM device allocation");
  }
  return NULL;
}

void
closePcmDevice (PcmDevice *pcm) {
  close(pcm->fileDescriptor);
  free(pcm);
}

int
writePcmData (PcmDevice *pcm, const unsigned char *buffer, int count) {
  return writeData(pcm->fileDescriptor, buffer, count) != -1;
}

int
getPcmBlockSize (PcmDevice *pcm) {
  int fragmentCount = (1 << 0X10) - 1;
  int fragmentShift = 7;
  int fragmentSize = 1 << fragmentShift;
  int fragmentSetting = (fragmentCount << 0X10) | fragmentShift;
  ioctl(pcm->fileDescriptor, SNDCTL_DSP_SETFRAGMENT, &fragmentSetting);

  {
    int blockSize;
    if (ioctl(pcm->fileDescriptor, SNDCTL_DSP_GETBLKSIZE, &blockSize) != -1) return blockSize;
  }
  return fragmentSize;
}

int
getPcmSampleRate (PcmDevice *pcm) {
  int rate;
  if (ioctl(pcm->fileDescriptor, SOUND_PCM_READ_RATE, &rate) != -1) return rate;
  return 8000;
}

int
setPcmSampleRate (PcmDevice *pcm, int rate) {
  if (ioctl(pcm->fileDescriptor, SOUND_PCM_WRITE_RATE, &rate) != -1) return rate;
  return getPcmSampleRate(pcm);
}

int
getPcmChannelCount (PcmDevice *pcm) {
  int channels;
  if (ioctl(pcm->fileDescriptor, SOUND_PCM_READ_CHANNELS, &channels) != -1) return channels;
  return 1;
}

int
setPcmChannelCount (PcmDevice *pcm, int channels) {
  if (ioctl(pcm->fileDescriptor, SOUND_PCM_WRITE_CHANNELS, &channels) != -1) return channels;
  return getPcmChannelCount(pcm);
}

typedef struct {
  PcmAmplitudeFormat internal;
  int external;
} AmplitudeFormatEntry;
static const AmplitudeFormatEntry amplitudeFormatTable[] = {
  {PCM_FMT_U8     , AFMT_U8    },
  {PCM_FMT_S8     , AFMT_S8    },
  {PCM_FMT_U16B   , AFMT_U16_BE},
  {PCM_FMT_S16B   , AFMT_S16_BE},
  {PCM_FMT_U16L   , AFMT_U16_LE},
  {PCM_FMT_S16L   , AFMT_S16_LE},
  {PCM_FMT_ULAW   , AFMT_MU_LAW},
  {PCM_FMT_UNKNOWN, AFMT_QUERY }
};

static PcmAmplitudeFormat
doPcmAmplitudeFormat (PcmDevice *pcm, int format) {
  if (ioctl(pcm->fileDescriptor, SNDCTL_DSP_SETFMT, &format) != -1) {
    const AmplitudeFormatEntry *entry = amplitudeFormatTable;
    while (entry->internal != PCM_FMT_UNKNOWN) {
      if (entry->external == format) return entry->internal;
      ++entry;
    }
  }
  return PCM_FMT_UNKNOWN;
}

PcmAmplitudeFormat
getPcmAmplitudeFormat (PcmDevice *pcm) {
  return doPcmAmplitudeFormat(pcm, AFMT_QUERY);
}

PcmAmplitudeFormat
setPcmAmplitudeFormat (PcmDevice *pcm, PcmAmplitudeFormat format) {
  const AmplitudeFormatEntry *entry = amplitudeFormatTable;
  while (entry->internal != PCM_FMT_UNKNOWN) {
    if (entry->internal == format) break;
    ++entry;
  }
  return doPcmAmplitudeFormat(pcm, entry->external);
}

void
forcePcmOutput (PcmDevice *pcm) {
  ioctl(pcm->fileDescriptor, SNDCTL_DSP_POST, 0);
}

void
awaitPcmOutput (PcmDevice *pcm) {
  ioctl(pcm->fileDescriptor, SNDCTL_DSP_SYNC, 0);
}

void
cancelPcmOutput (PcmDevice *pcm) {
  ioctl(pcm->fileDescriptor, SNDCTL_DSP_RESET, 0);
}

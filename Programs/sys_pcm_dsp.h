int
getPcmDevice (int errorLevel) {
  const char *path = "/dev/dsp";
  int descriptor = open(path, O_WRONLY|O_NONBLOCK);
  if (descriptor == -1) LogPrint(errorLevel, "Cannot open PCM device: %s: %s", path, strerror(errno));
  return descriptor;
}

int
getPcmBlockSize (int descriptor) {
  int fragmentCount = (1 << 0X10) - 1;
  int fragmentShift = 7;
  int fragmentSize = 1 << fragmentShift;
  int fragmentSetting = (fragmentCount << 0X10) | fragmentShift;
  if (descriptor != -1) {
    int blockSize;
    ioctl(descriptor, SNDCTL_DSP_SETFRAGMENT, &fragmentSetting);
    if (ioctl(descriptor, SNDCTL_DSP_GETBLKSIZE, &blockSize) != -1) return blockSize;
  }
  return fragmentSize;
}

int
getPcmSampleRate (int descriptor) {
  if (descriptor != -1) {
    int rate;
    if (ioctl(descriptor, SOUND_PCM_READ_RATE, &rate) != -1) return rate;
  }
  return 8000;
}

int
setPcmSampleRate (int descriptor, int rate) {
  if (descriptor != -1) {
    if (ioctl(descriptor, SOUND_PCM_WRITE_RATE, &rate) != -1) return rate;
  }
  return getPcmSampleRate(descriptor);
}

int
getPcmChannelCount (int descriptor) {
  if (descriptor != -1) {
    int channels;
    if (ioctl(descriptor, SOUND_PCM_READ_CHANNELS, &channels) != -1) return channels;
  }
  return 1;
}

int
setPcmChannelCount (int descriptor, int channels) {
  if (descriptor != -1) {
    if (ioctl(descriptor, SOUND_PCM_WRITE_CHANNELS, &channels) != -1) return channels;
  }
  return getPcmChannelCount(descriptor);
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
doPcmAmplitudeFormat (int descriptor, int format) {
  if (descriptor != -1) {
    if (ioctl(descriptor, SNDCTL_DSP_SETFMT, &format) != -1) {
      const AmplitudeFormatEntry *entry = amplitudeFormatTable;
      while (entry->internal != PCM_FMT_UNKNOWN) {
        if (entry->external == format) return entry->internal;
        ++entry;
      }
    }
  }
  return PCM_FMT_UNKNOWN;
}

PcmAmplitudeFormat
getPcmAmplitudeFormat (int descriptor) {
  return doPcmAmplitudeFormat(descriptor, AFMT_QUERY);
}

PcmAmplitudeFormat
setPcmAmplitudeFormat (int descriptor, PcmAmplitudeFormat format) {
  const AmplitudeFormatEntry *entry = amplitudeFormatTable;
  while (entry->internal != PCM_FMT_UNKNOWN) {
    if (entry->internal == format) break;
    ++entry;
  }
  return doPcmAmplitudeFormat(descriptor, entry->external);
}

void
forcePcmOutput (int descriptor) {
  ioctl(descriptor, SNDCTL_DSP_POST, 0);
}

void
awaitPcmOutput (int descriptor) {
  ioctl(descriptor, SNDCTL_DSP_SYNC, 0);
}

void
cancelPcmOutput (int descriptor) {
  ioctl(descriptor, SNDCTL_DSP_RESET, 0);
}

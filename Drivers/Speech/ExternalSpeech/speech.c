/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2024 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* ExternalSpeech/speech.c - Speech library (driver)
 * For external programs, using my own protocol. Features indexing.
 * Stéphane Doyon <s.doyon@videotron.ca>
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "log.h"
#include "timing.h"
#include "io_misc.h"
#include "async_handle.h"
#include "async_io.h"

typedef enum {
  PARM_SOCKET_PATH,
} DriverParameter;

#define SPKPARMS "socket_path"
#include "spk_driver.h"
#include "speech.h"

static const char *socketPath = NULL;
static struct sockaddr_un socketAddress;
static int socketDescriptor = -1;

static int sendSettings (SpeechSynthesizer *spk);
static unsigned char volumePercentage;
static float rateMultiplier;
static float pitchMultiplier;
static unsigned char punctuationLevel;

static uint16_t totalCharacterCount;
#define TRACK_DATA_SIZE 2
static AsyncHandle trackHandle = NULL;

ASYNC_INPUT_CALLBACK(xsHandleSpeechTrackingInput) {
  SpeechSynthesizer *spk = parameters->data;

  if (parameters->error) {
    logMessage(LOG_WARNING,
      "speech tracking input error %d: %s",
      parameters->error, strerror(parameters->error)
    );
  } else if (parameters->end) {
    logMessage(LOG_WARNING, "speech tracking end-of-file");
  } else if (parameters->length >= TRACK_DATA_SIZE) {
    const unsigned char *buffer = parameters->buffer;
    uint16_t location = (buffer[0] << 8) | buffer[1];

    if (location < totalCharacterCount) {
      tellSpeechLocation(spk, location);
    } else {
      tellSpeechFinished(spk);
    }

    return TRACK_DATA_SIZE;
  }

  return 0;
}

static int
amConnected (void) {
  return socketDescriptor != -1;
}

static int
connectToServer (SpeechSynthesizer *spk) {
  if (amConnected()) return 1;

  logMessage(LOG_CATEGORY(SPEECH_DRIVER), "connecting to server: %s", socketPath);
  int sd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sd != -1) {
    if (setCloseOnExec(sd, 1)) {
      if (connect(sd, (const struct sockaddr *)&socketAddress, sizeof(socketAddress)) != -1) {
        if (setBlockingIo(sd, 0)) {
          if (asyncReadSocket(&trackHandle, sd, TRACK_DATA_SIZE, xsHandleSpeechTrackingInput, spk)) {
            logMessage(LOG_CATEGORY(SPEECH_DRIVER), "connected to server: fd=%d", sd);
            socketDescriptor = sd;
            sendSettings(spk);
            return amConnected();
          }
        }
      } else {
        logSystemError("connect");
      }
    }

    close(sd);
  } else {
    logSystemError("socket");
  }

  return 0;
}

static void
disconnectFromServer (void) {
  if (amConnected()) {
    logMessage(LOG_CATEGORY(SPEECH_DRIVER), "disconnecting from server");

    if (trackHandle) {
      asyncCancelRequest(trackHandle);
      trackHandle = NULL;
    }

    close(socketDescriptor);
    socketDescriptor = -1;
  }
}

static int
sendPacket (SpeechSynthesizer *spk, const unsigned char *packet, size_t length) {
  if (!amConnected()) {
    if (!connectToServer(spk)) {
      return 0;
    }
  }

  const unsigned char *position = packet;
  const unsigned char *end = position + length;

  TimePeriod period;
  startTimePeriod(&period, XS_WRITE_TIMEOUT);

  while (position < end) {
    if (afterTimePeriod(&period, NULL)) {
      logMessage(LOG_ERR, "ExternalSpeech write timed out");
      disconnectFromServer();
      return 0;
    }

    ssize_t result = write(socketDescriptor, position, (end - position));

    if (result == -1) {
      if ((errno == EINTR) || (errno == EAGAIN)) continue;

      logMessage(LOG_ERR,
        "ExternalSpeech write error %d: %s",
        errno, strerror(errno)
      );

      disconnectFromServer();
      if (!connectToServer(spk)) return 0;

      position = packet;
      continue;
    }

    position += result;
  }

  return 1;
}

static int
sendByteSetting (SpeechSynthesizer *spk, unsigned char code, unsigned char value) {
  unsigned char packet[2] = {code, value};

  return sendPacket(spk, packet, sizeof(packet));
}

static int
sendMultplierSetting (SpeechSynthesizer *spk, unsigned char code, float value) {
  unsigned char packet[5] = {code};
  const unsigned char *v = (const unsigned char *)&value;

#ifdef WORDS_BIGENDIAN
  packet[1] = v[0];
  packet[2] = v[1];
  packet[3] = v[2];
  packet[4] = v[3];
#else /* WORDS_BIGENDIAN */
  packet[1] = v[3];
  packet[2] = v[2];
  packet[3] = v[1];
  packet[4] = v[0];
#endif /* WORDS_BIGENDIAN */

  return sendPacket(spk, packet, sizeof(packet));
}

static int
sendVolumePercentage (SpeechSynthesizer *spk) {
  return sendByteSetting(spk, 2, volumePercentage);
}

static int
sendRateMultiplier (SpeechSynthesizer *spk) {
  return sendMultplierSetting(spk, 3, (1.0 / rateMultiplier));
}

static int
sendPitchMultiplier (SpeechSynthesizer *spk) {
  return sendMultplierSetting(spk, 5, pitchMultiplier);
}

static int
sendPunctuationLevel (SpeechSynthesizer *spk) {
  return sendByteSetting(spk, 6, punctuationLevel);
}

static int
sendSettings (SpeechSynthesizer *spk) {
  typedef int SendSettingHandler (SpeechSynthesizer *spk);

  static SendSettingHandler *const handlers[] = {
    sendVolumePercentage,
    sendRateMultiplier,
    sendPitchMultiplier,
    sendPunctuationLevel,
    NULL
  };

  {
    SendSettingHandler *const *handler = handlers;

    while (*handler) {
      if (!(*handler)(spk)) return 0;
      handler += 1;
    }
  }

  return 1;
}

static void
spk_setVolume (SpeechSynthesizer *spk, unsigned char setting) {
  volumePercentage = getIntegerSpeechVolume(setting, 100);

  logMessage(
    LOG_CATEGORY(SPEECH_DRIVER),
    "set volume to %u (%u%%)",
    setting, volumePercentage
  );

  sendVolumePercentage(spk);
}

static void
spk_setRate (SpeechSynthesizer *spk, unsigned char setting) {
  rateMultiplier = getFloatSpeechRate(setting); 

  logMessage(
    LOG_CATEGORY(SPEECH_DRIVER),
    "set rate to %u (multiplier %f)",
    setting, rateMultiplier
  );

  sendRateMultiplier(spk);
}

static void
spk_setPitch (SpeechSynthesizer *spk, unsigned char setting) {
  pitchMultiplier = getFloatSpeechPitch(setting); 

  logMessage(
    LOG_CATEGORY(SPEECH_DRIVER),
    "set pitch to %u (multiplier %f)",
    setting, pitchMultiplier
  );

  sendPitchMultiplier(spk);
}

static void
spk_setPunctuation (SpeechSynthesizer *spk, SpeechPunctuation setting) {
  punctuationLevel = setting;

  logMessage(
    LOG_CATEGORY(SPEECH_DRIVER),
    "set punctuation to %u",
    setting
  );

  sendPunctuationLevel(spk);
}

static void
spk_mute (SpeechSynthesizer *spk) {
  logMessage(LOG_CATEGORY(SPEECH_DRIVER), "mute");

  unsigned char packet[] = {1};
  sendPacket(spk, packet, sizeof(packet));
}

static void
spk_say (SpeechSynthesizer *spk, const unsigned char *text, size_t length, size_t count, const unsigned char *attributes) {
  if (!attributes) count = 0;

  unsigned char packet[5 + length + count];
  unsigned char *p = packet;

  *p++ = 4;
  *p++ = length >> 8;
  *p++ = length & 0XFF;
  *p++ = count >> 8;
  *p++ = count & 0XFF;
  p = mempcpy(p, text, length);
  p = mempcpy(p, attributes, count);

  if (sendPacket(spk, packet, (p - packet))) {
    totalCharacterCount = count;
  }
}

static int
spk_construct (SpeechSynthesizer *spk, char **parameters) {
  spk->setVolume = spk_setVolume;
  spk->setRate = spk_setRate;
  spk->setPitch = spk_setPitch;
  spk->setPunctuation = spk_setPunctuation;

  volumePercentage = 100;
  rateMultiplier = 1.0;
  pitchMultiplier = 1.0;
  punctuationLevel = 0;

  socketPath = parameters[PARM_SOCKET_PATH];
  if (!socketPath || !*socketPath) socketPath = XS_DEFAULT_SOCKET_PATH;

  memset(&socketAddress, 0, sizeof(socketAddress));
  socketAddress.sun_family = AF_UNIX;
  strncpy(socketAddress.sun_path, socketPath, sizeof(socketAddress.sun_path)-1);

  socketDescriptor = -1;
  trackHandle = NULL;
  return connectToServer(spk);
}

static void
spk_destruct (SpeechSynthesizer *spk) {
  disconnectFromServer();
}

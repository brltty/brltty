/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/un.h>


#include "log.h"
#include "timing.h"
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

static uint16_t totalCharacterCount;

#define TRACK_DATA_SIZE 2
static AsyncHandle trackHandle = NULL;

ASYNC_INPUT_CALLBACK(xsHandleSpeechTrackingInput) {
  SpeechSynthesizer *spk = parameters->data;

  if (parameters->error) {
    logMessage(LOG_WARNING, "speech tracking input error: %s", strerror(parameters->error));
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
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (fd != -1) {
    if (connect(fd, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) != -1) {
      if (fcntl(fd, F_SETFL, O_NONBLOCK) != -1) {
        if (asyncReadFile(&trackHandle, socketDescriptor, TRACK_DATA_SIZE*10, xsHandleSpeechTrackingInput, spk)) {
          socketDescriptor = fd;
          return 1;
        }
      } else {
        logSystemError("fcntl[F_SETFL,O_NONBLOCK]");
      }
    } else {
      logSystemError("connect");
    }

    close(fd);
  } else {
    logSystemError("socket");
  }

  return amConnected();
}

static void
disconnectFromServer (void) {
  if (amConnected()) {
    if (trackHandle) {
      asyncCancelRequest(trackHandle);
      trackHandle = NULL;
    }

    close(socketDescriptor);
    socketDescriptor = -1;
  }
}

static int
sendData (SpeechSynthesizer *spk, const void *buffer, size_t length) {
  if (!amConnected()) {
    if (!connectToServer(spk)) {
      return 0;
    }
  }

  const unsigned char *position = buffer;
  const unsigned char *end = position + length;

  TimePeriod period;
  startTimePeriod(&period, 2000);

  while (position < end) {
    if (afterTimePeriod(&period, NULL)) break;
    ssize_t result = write(socketDescriptor, position, (end - position));

    if (result == -1) {
      if ((errno == EINTR) || (errno == EAGAIN)) continue;

      logMessage(
        LOG_CATEGORY(SPEECH_DRIVER),
        "ExternalSpeech write error %d: %s",
        errno, strerror(errno)
      );

      disconnectFromServer();
      if (!connectToServer(spk)) return 0;

      position = buffer;
      continue;
    }

    position += result;
  }

  int done = position == end;
  if (!done) logMessage(LOG_CATEGORY(SPEECH_DRIVER), "write timed out");
  return done;
}

static void
spk_say (SpeechSynthesizer *spk, const unsigned char *text, size_t length, size_t count, const unsigned char *attributes) {
  unsigned char l[5];
  l[0] = 4; /* say code */
  l[1] = length >> 8;
  l[2] = length & 0xFF;

  if (attributes) {
    l[3] = count >> 8;
    l[4] = count & 0xFF;
  } else {
    l[3] = 0;
    l[4] = 0;
  }

  if (!sendData(spk, l, sizeof(l))) return;
  if (!sendData(spk, text, length)) return;

  if (attributes) {
    if (!sendData(spk, attributes, count)) {
      return;
    }
  }

  totalCharacterCount = count;
}

static void
spk_mute (SpeechSynthesizer *spk) {
  logMessage(LOG_CATEGORY(SPEECH_DRIVER), "mute");
  unsigned char l[1];
  l[0] = 1;
  sendData(spk, l, sizeof(l));
}

static void
spk_setVolume (SpeechSynthesizer *spk, unsigned char setting) {
  logMessage(LOG_DEBUG,"set volume to %u", setting);

  unsigned char l[2];
  l[0] = 2;
  l[1] = getIntegerSpeechVolume(setting, 100);
  sendData(spk, l, sizeof(l));
}

static int
sendFloatSetting (SpeechSynthesizer *spk, unsigned char code, float value) {
  unsigned char l[5] = {code};
  unsigned char *p = (unsigned char *)&value;

#ifdef WORDS_BIGENDIAN
  l[1] = p[0]; l[2] = p[1]; l[3] = p[2]; l[4] = p[3];
#else /* WORDS_BIGENDIAN */
  l[1] = p[3]; l[2] = p[2]; l[3] = p[1]; l[4] = p[0];
#endif /* WORDS_BIGENDIAN */

  return sendData(spk, l, sizeof(l));
}

static void
spk_setRate (SpeechSynthesizer *spk, unsigned char setting) {
  float expand = 1.0 / getFloatSpeechRate(setting); 
  logMessage(LOG_DEBUG,"set rate to %u (time scale %f)", setting, expand);
  sendFloatSetting(spk, 3, expand);
}

static void
spk_setPitch (SpeechSynthesizer *spk, unsigned char setting) {
  float multiplier = getFloatSpeechPitch(setting); 
  logMessage(LOG_DEBUG,"set pitch to %u (multiplier %f)", setting, multiplier);
  sendFloatSetting(spk, 5, multiplier);
}

static int
spk_construct (SpeechSynthesizer *spk, char **parameters) {
  spk->setVolume = spk_setVolume;
  spk->setRate = spk_setRate;
  spk->setPitch = spk_setPitch;

  socketPath = parameters[PARM_SOCKET_PATH];
  if (!socketPath || !*socketPath) socketPath = HELPER_SOCKET_PATH;

  memset(&socketAddress, 0, sizeof(socketAddress));
  socketAddress.sun_family = AF_UNIX;
  strncpy(socketAddress.sun_path, socketPath, sizeof(socketAddress.sun_path)-1);

  return connectToServer(spk);
}

static void
spk_destruct (SpeechSynthesizer *spk) {
  disconnectFromServer();
}

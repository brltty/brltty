/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/un.h>


#include "log.h"
#include "timing.h"
#include "async_io.h"

typedef enum {
  PARM_SOCK_PATH,
} DriverParameter;
#define SPKPARMS "socket_path"

#include "spk_driver.h"
#include "speech.h"

static int helper_fd = -1;
static uint16_t totalCharacterCount;

#define TRACK_DATA_SIZE 2
static AsyncHandle trackHandle = NULL;

#define ERRBUFLEN 200
static void myerror(volatile SpeechSynthesizer *spk, const char *fmt, ...)
{
  char buf[ERRBUFLEN];
  int offs;
  va_list argp;
  va_start(argp, fmt);
  offs = snprintf(buf, ERRBUFLEN, "ExternalSpeech: ");
  if(offs < ERRBUFLEN) {
    offs += vsnprintf(buf+offs, ERRBUFLEN-offs, fmt, argp);
  }
  buf[ERRBUFLEN-1] = 0;
  va_end(argp);
  logMessage(LOG_ERR, "%s", buf);
  spk_destruct(spk);
}
static void myperror(volatile SpeechSynthesizer *spk, const char *fmt, ...)
{
  char buf[ERRBUFLEN];
  int offs;
  va_list argp;
  va_start(argp, fmt);
  offs = snprintf(buf, ERRBUFLEN, "ExternalSpeech: ");
  if(offs < ERRBUFLEN) {
    offs += vsnprintf(buf+offs, ERRBUFLEN-offs, fmt, argp);
    if(offs < ERRBUFLEN)
      snprintf(buf+offs, ERRBUFLEN-offs, ": %s", strerror(errno));
  }
  buf[ERRBUFLEN-1] = 0;
  va_end(argp);
  logMessage(LOG_ERR, "%s", buf);
  spk_destruct(spk);
}

static void mywrite(volatile SpeechSynthesizer *spk, int fd, const void *buf, int len)
{
  char *pos = (char *)buf;
  int w;
  TimePeriod period;
  if(fd<0) return;
  startTimePeriod(&period, 2000);
  do {
    if((w = write(fd, pos, len)) < 0) {
      if(errno == EINTR || errno == EAGAIN) continue;
      else if(errno == EPIPE)
	myerror(spk, "ExternalSpeech: pipe to helper program was broken");
         /* try to reinit may be ??? */
      else myperror(spk, "ExternalSpeech: pipe to helper program: write");
      return;
    }
    pos += w; len -= w;
  } while(len && !afterTimePeriod(&period, NULL));
  if(len)
    myerror(spk, "ExternalSpeech: pipe to helper program: write timed out");
}

static void spk_say(volatile SpeechSynthesizer *spk, const unsigned char *text, size_t length, size_t count, const unsigned char *attributes)
{
  unsigned char l[5];
  if(helper_fd < 0) return;
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
  mywrite(spk, helper_fd, l, 5);
  mywrite(spk, helper_fd, text, length);
  if (attributes) mywrite(spk, helper_fd, attributes, count);
  totalCharacterCount = count;
}

static void spk_mute (volatile SpeechSynthesizer *spk)
{
  unsigned char c = 1;
  if(helper_fd < 0) return;
  logMessage(LOG_DEBUG,"mute");
  mywrite(spk, helper_fd, &c,1);
}

static void spk_setRate (volatile SpeechSynthesizer *spk, unsigned char setting)
{
  float expand = 1.0 / getFloatSpeechRate(setting); 
  unsigned char *p = (unsigned char *)&expand;
  unsigned char l[5];
  if(helper_fd < 0) return;
  logMessage(LOG_DEBUG,"set rate to %u (time scale %f)", setting, expand);
  l[0] = 3; /* time scale code */
#ifdef WORDS_BIGENDIAN
  l[1] = p[0]; l[2] = p[1]; l[3] = p[2]; l[4] = p[3];
#else /* WORDS_BIGENDIAN */
  l[1] = p[3]; l[2] = p[2]; l[3] = p[1]; l[4] = p[0];
#endif /* WORDS_BIGENDIAN */
  mywrite(spk, helper_fd, &l, 5);
}

ASYNC_INPUT_CALLBACK(xsHandleSpeechTrackingInput) {
  volatile SpeechSynthesizer *spk = parameters->data;

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

static int spk_construct (volatile SpeechSynthesizer *spk, char **parameters)
{
  const char *extSockPath = parameters[PARM_SOCK_PATH];

  spk->setRate = spk_setRate;

  if(!*extSockPath) extSockPath = HELPER_SOCKET_PATH;

  if((helper_fd = socket(PF_UNIX, SOCK_STREAM, 0)) <0) {
    myperror(spk, "socket");
    return 0;
  }

  struct sockaddr_un tgtaddr;
  memset(&tgtaddr, 0, sizeof(tgtaddr));
  tgtaddr.sun_family = PF_UNIX;
  strncpy(tgtaddr.sun_path, extSockPath, sizeof(tgtaddr.sun_path)-1);
  if(connect(helper_fd, (struct sockaddr *)&tgtaddr, sizeof(tgtaddr)) <0) {
    myperror(spk, "connect to %s", extSockPath);
    return 0;
  }

  if(fcntl(helper_fd, F_SETFL,O_NONBLOCK) < 0) {
    myperror(spk, "fcntl F_SETFL O_NONBLOCK");
    //close(helper_fd);
    //helper_fd = -1;
    return 0;
  }
  logMessage(LOG_INFO, "Connected to ExternalSpeech helper socket at %s", extSockPath);


  asyncReadFile(&trackHandle, helper_fd, TRACK_DATA_SIZE*10, xsHandleSpeechTrackingInput, (void *)spk);
  return 1;
}

static void spk_destruct (volatile SpeechSynthesizer *spk)
{
  if(trackHandle)
    asyncCancelRequest(trackHandle);
  if(helper_fd >= 0)
    close(helper_fd);
  helper_fd = -1;
  trackHandle = NULL;
}

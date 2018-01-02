/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
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

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif /* HAVE_PWD_H */

#ifdef HAVE_GRP_H
#include <grp.h>
#endif /* HAVE_GRP_H */

#ifdef __MINGW32__
#include <io.h>
#endif /* __MINGW32__ */

#include "log.h"
#include "timing.h"
#include "async_io.h"

typedef enum {
  PARM_PROGRAM=0,
  PARM_UID, PARM_GID
} DriverParameter;
#define SPKPARMS "program", "uid", "gid"

#include "spk_driver.h"
#include "speech.h"

static int helper_fd_in = -1, helper_fd_out = -1;
static uint16_t totalCharacterCount;

#define TRACK_DATA_SIZE 2
static AsyncHandle trackHandle = NULL;

#define ERRBUFLEN 200
static void myerror(volatile SpeechSynthesizer *spk, char *fmt, ...)
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
static void myperror(volatile SpeechSynthesizer *spk, char *fmt, ...)
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
  if(helper_fd_out < 0) return;
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
  mywrite(spk, helper_fd_out, l, 5);
  mywrite(spk, helper_fd_out, text, length);
  if (attributes) mywrite(spk, helper_fd_out, attributes, count);
  totalCharacterCount = count;
}

static void spk_mute (volatile SpeechSynthesizer *spk)
{
  unsigned char c = 1;
  if(helper_fd_out < 0) return;
  logMessage(LOG_DEBUG,"mute");
  mywrite(spk, helper_fd_out, &c,1);
}

static void spk_setRate (volatile SpeechSynthesizer *spk, unsigned char setting)
{
  float expand = 1.0 / getFloatSpeechRate(setting); 
  unsigned char *p = (unsigned char *)&expand;
  unsigned char l[5];
  if(helper_fd_out < 0) return;
  logMessage(LOG_DEBUG,"set rate to %u (time scale %f)", setting, expand);
  l[0] = 3; /* time scale code */
#ifdef WORDS_BIGENDIAN
  l[1] = p[0]; l[2] = p[1]; l[3] = p[2]; l[4] = p[3];
#else /* WORDS_BIGENDIAN */
  l[1] = p[3]; l[2] = p[2]; l[3] = p[1]; l[4] = p[0];
#endif /* WORDS_BIGENDIAN */
  mywrite(spk, helper_fd_out, &l, 5);
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
  char *extProgPath = parameters[PARM_PROGRAM];

  spk->setRate = spk_setRate;

  if(!*extProgPath) extProgPath = HELPER_PROG_PATH;

#ifdef __MINGW32__
  STARTUPINFO startupinfo;
  PROCESS_INFORMATION processinfo;
  SECURITY_ATTRIBUTES attributes;
  HANDLE fd1R,fd1W,fd2R,fd2W;

  memset(&attributes, 0, sizeof(attributes));
  attributes.nLength = sizeof(attributes);
  attributes.bInheritHandle = TRUE;

  if (!CreatePipe(&fd1R,&fd1W,&attributes,0)
      ||!CreatePipe(&fd2R,&fd2W,&attributes,0)) {
    logWindowsSystemError("CreatePipe");
    return 0;
  }

  memset(&startupinfo, 0, sizeof(startupinfo));
  startupinfo.cb = sizeof(startupinfo);
  startupinfo.dwFlags = STARTF_USESTDHANDLES;
  startupinfo.hStdInput = fd2R;
  startupinfo.hStdOutput = fd1W;
  if (!CreateProcess(NULL, extProgPath, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &startupinfo, &processinfo)) {
    logWindowsSystemError("CreateProcess");
    return 0;
  }
  if ((helper_fd_in = _open_osfhandle((long)fd1R, O_RDONLY)) < 0) {
    logSystemError("open_osfhandle");
    return 0;
  }
  if ((helper_fd_out = _open_osfhandle((long)fd2W, O_WRONLY)) < 0) {
    logSystemError("open_osfhandle");
    return 0;
  }
#else /* __MINGW32__ */
  int fd1[2], fd2[2];
  uid_t uid, gid;
  char
    *s_uid = parameters[PARM_UID],
    *s_gid = parameters[PARM_GID];

  if(*s_uid) {
#ifdef HAVE_PWD_H
    struct passwd *pe = getpwnam(s_uid);
    if (pe) {
      uid = pe->pw_uid;
    } else
#endif /* HAVE_PWD_H */
    {
      char *ptr;
      uid = strtol(s_uid, &ptr, 0);
      if(*ptr != 0) {
        myerror(spk, "Unable to get an uid value with '%s'", s_uid);
        return 0;
      }
    }
  }else uid = UID;

  if(*s_gid) {
#ifdef HAVE_GRP_H
    struct group *ge = getgrnam(s_gid);
    if (ge) {
      gid = ge->gr_gid;
    } else
#endif /* HAVE_GRP_H */
    {
      char *ptr;
      gid = strtol(s_gid, &ptr, 0);
      if(*ptr != 0) {
        myerror(spk, "Unable to get a gid value with '%s'", s_gid);
        return 0;
      }
    }
  }else gid = GID;

  if(pipe(fd1) < 0
     || pipe(fd2) < 0) {
    myperror(spk, "pipe");
    return 0;
  }
  logMessage(LOG_DEBUG, "pipe fds: fd1 %d %d, fd2 %d %d",
	     fd1[0],fd1[1], fd2[0],fd2[1]);
  switch(fork()) {
  case -1:
    myperror(spk, "fork");
    return 0;
  case 0: {
    int i;
    if(setgid(gid) <0) {
      myperror(spk, "setgid to %u", gid);
      _exit(1);
    }
    if(setuid(uid) <0) {
      myperror(spk, "setuid to %u", uid);
      _exit(1);
    }

    {
      unsigned long uid = getuid();
      unsigned long gid = getgid();
      logMessage(LOG_INFO, "ExternalSpeech program uid is %lu, gid is %lu", uid, gid);
    }

    if(dup2(fd2[0], 0) < 0 /* stdin */
       || dup2(fd1[1], 1) < 0){ /* stdout */
      myperror(spk, "dup2");
      _exit(1);
    }
    {
      long numfds = sysconf(_SC_OPEN_MAX);
      for(i=2; i<numfds; i++) close(i);
    }
    execl(extProgPath, extProgPath, (void *)NULL);
    myperror(spk, "Unable to execute external speech program '%s'", extProgPath);
    _exit(1);
  }
  default:
    helper_fd_in = fd1[0];
    helper_fd_out = fd2[1];
    close(fd1[1]);
    close(fd2[0]);
    if(fcntl(helper_fd_in, F_SETFL,O_NONBLOCK) < 0
       || fcntl(helper_fd_out, F_SETFL,O_NONBLOCK) < 0) {
      myperror(spk, "fcntl F_SETFL O_NONBLOCK");
      return 0;
    }
  };
#endif /* __MINGW32__ */

  logMessage(LOG_INFO,"Opened pipe to external speech program '%s'",
	     extProgPath);

  asyncReadFile(&trackHandle, helper_fd_in, TRACK_DATA_SIZE*10, xsHandleSpeechTrackingInput, (void *)spk);
  return 1;
}

static void spk_destruct (volatile SpeechSynthesizer *spk)
{
  if(trackHandle)
    asyncCancelRequest(trackHandle);
  if(helper_fd_in >= 0)
    close(helper_fd_in);
  if(helper_fd_out >= 0)
    close(helper_fd_out);
  helper_fd_in = helper_fd_out = -1;
  trackHandle = NULL;
}

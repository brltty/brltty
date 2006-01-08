/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Team. All rights reserved.
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
#define ES_VERSION "BRLTTY External Speech driver, version 0.8 (September 2004)"
#define ES_COPYRIGHT "Copyright (C) 2000-2001,2004 by Stéphane Doyon " \
                  "<s.doyon@videotron.ca>"
/* ExternalSpeech/speech.c - Speech library (driver)
 * For external programs, using my own protocol. Features indexing.
 * Stéphane Doyon <s.doyon@videotron.ca>
 */

#include "prologue.h"

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

#include "Programs/misc.h"

typedef enum {
  PARM_PROGRAM=0,
  PARM_UID, PARM_GID
} DriverParameter;
#define SPKPARMS "program", "uid", "gid"

#define SPK_HAVE_TRACK
#define SPK_HAVE_EXPRESS
#define SPK_HAVE_RATE
#include "Programs/spk_driver.h"
#include "speech.h"

static int helper_fd_in = -1, helper_fd_out = -1;
static unsigned short lastIndex, finalIndex;
static char speaking = 0;

static void spk_identify (void)
{
  LogPrint(LOG_NOTICE, ES_VERSION);
  LogPrint(LOG_INFO, "   "ES_COPYRIGHT);
}

#define ERRBUFLEN 200
static void myerror(char *fmt, ...)
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
  LogPrint(LOG_ERR, "%s", buf);
  spk_close();
}
static void myperror(char *fmt, ...)
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
  LogPrint(LOG_ERR, "%s", buf);
  spk_close();
}

static int spk_open (char **parameters)
{
  char *extProgPath = parameters[PARM_PROGRAM];

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
    LogWindowsError("CreatePipe");
    return 0;
  }

  memset(&startupinfo, 0, sizeof(startupinfo));
  startupinfo.cb = sizeof(startupinfo);
  startupinfo.dwFlags = STARTF_USESTDHANDLES;
  startupinfo.hStdInput = fd2R;
  startupinfo.hStdOutput = fd1W;
  if (!CreateProcess(NULL, extProgPath, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &startupinfo, &processinfo)) {
    LogWindowsError("CreateProcess");
    return 0;
  }
  if ((helper_fd_in = _open_osfhandle((long)fd1R, O_RDONLY)) < 0) {
    LogError("open_osfhandle");
    return 0;
  }
  if ((helper_fd_out = _open_osfhandle((long)fd2W, O_WRONLY)) < 0) {
    LogError("open_osfhandle");
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
        myerror("Unable to get an uid value with '%s'", s_uid);
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
        myerror("Unable to get a gid value with '%s'", s_gid);
        return 0;
      }
    }
  }else gid = GID;

  if(pipe(fd1) < 0
     || pipe(fd2) < 0) {
    myperror("pipe");
    return 0;
  }
  LogPrint(LOG_DEBUG, "pipe fds: fd1 %d %d, fd2 %d %d",
	   fd1[0],fd1[1], fd2[0],fd2[1]);
  switch(fork()) {
  case -1:
    myperror("fork");
    return 0;
  case 0: {
    int i;
    if(setgid(gid) <0) {
      myperror("setgid to %u", gid);
      _exit(1);
    }
    if(setuid(uid) <0) {
      myperror("setuid to %u", uid);
      _exit(1);
    }

    {
      unsigned long uid = getuid();
      unsigned long gid = getgid();
      LogPrint(LOG_INFO, "ExternalSpeech program uid is %lu, gid is %lu", uid, gid);
    }

    if(dup2(fd2[0], 0) < 0 /* stdin */
       || dup2(fd1[1], 1) < 0){ /* stdout */
      myperror("dup2");
      _exit(1);
    }
    {
      long numfds = sysconf(_SC_OPEN_MAX);
      for(i=2; i<numfds; i++) close(i);
    }
    execl(extProgPath, extProgPath, (void *)NULL);
    myperror("Unable to execute external speech program '%s'", extProgPath);
    _exit(1);
  }
  default:
    helper_fd_in = fd1[0];
    helper_fd_out = fd2[1];
    close(fd1[1]);
    close(fd2[0]);
    if(fcntl(helper_fd_in, F_SETFL,O_NDELAY) < 0
       || fcntl(helper_fd_out, F_SETFL,O_NDELAY) < 0) {
      myperror("fcntl F_SETFL O_NDELAY");
      return 0;
    }
  };
#endif /* __MINGW32__ */

  LogPrint(LOG_INFO,"Opened pipe to external speech program '%s'",
	   extProgPath);

  return 1;
}

static void mywrite(int fd, const void *buf, int len)
{
  char *pos = (char *)buf;
  int w;
  if(fd<0) return;
  hasTimedOut(0);
  do {
    if((w = write(fd, pos, len)) < 0) {
      if(errno == EINTR || errno == EAGAIN) continue;
      else if(errno == EPIPE)
	myerror("ExternalSpeech: pipe to helper program was broken");
         /* try to reinit may be ??? */
      else myperror("ExternalSpeech: pipe to helper program: write");
      return;
    }
    pos += w; len -= w;
  } while(len && !hasTimedOut(2000));
  if(len)
    myerror("ExternalSpeech: pipe to helper program: write timed out");
}

static int myread(int fd, void *buf, int len)
{
  char *pos = (char *)buf;
  int r;
  int firstTime = 1;
  if(fd<0) return 0;
  hasTimedOut(0);
  do {
    if((r = read(fd, pos, len)) < 0) {
      if(errno == EINTR) continue;
      else if(errno == EAGAIN) {
	if(firstTime) return 0;
	else continue;
      }else myperror("ExternalSpeech: pipe to helper program: read");
    }else if(r==0)
      myerror("ExternalSpeech: pipe to helper program: read: EOF!");
    if(r<=0) return 0;
    firstTime = 0;
    pos += r; len -= r;
  } while(len && !hasTimedOut(400));
  if(len) {
    myerror("ExternalSpeech: pipe to helper program: read timed out");
    return 0;
  }
  return 1;
}

static void sayit(const unsigned char *buffer, int len, int attriblen)
{
  unsigned char l[5];
  if(helper_fd_out < 0) return;
  LogPrint(LOG_DEBUG,"Say %d bytes", len);
  l[0] = 2; /* say code */
  l[1] = len>>8;
  l[2] = len & 0xFF;
  l[3] = attriblen>>8;
  l[4] = attriblen & 0xFF;
  speaking = 1;
  mywrite(helper_fd_out, l, 5);
  mywrite(helper_fd_out, buffer, len+attriblen);
  lastIndex = 0;
  finalIndex = len;
}

static void spk_say(const unsigned char *buffer, int len)
{
  sayit(buffer,len,0);
}
static void spk_express(const unsigned char *buffer, int len)
{
  sayit(buffer,len,len);
}

static void spk_doTrack(void)
{
  unsigned char b[2];
  if(helper_fd_in < 0) return;
  while(myread(helper_fd_in, b, 2)) {
    unsigned inx;
    inx = (b[0]<<8 | b[1]);
    LogPrint(LOG_DEBUG, "spktrk: Received index %u", inx);
    if(inx >= finalIndex) {
      speaking = 0;
      LogPrint(LOG_DEBUG, "spktrk: Done speaking %d", lastIndex);
	/* do not change last_inx: remain on position of last spoken words,
	   not after them. */
    }else lastIndex = inx;
  }
}

static int spk_getTrack(void)
{
  return lastIndex;
}

static int spk_isSpeaking(void)
{
  return speaking;
}

static void spk_mute (void)
{
  unsigned char c = 1;
  if(helper_fd_out < 0) return;
  LogPrint(LOG_DEBUG,"mute");
  speaking = 0;
  mywrite(helper_fd_out, &c,1);
}

static void spk_rate (float setting)
{
  float expand = 1.0 / setting; 
  unsigned char *p = (unsigned char *)&expand;
  unsigned char l[5];
  if(helper_fd_out < 0) return;
  LogPrint(LOG_DEBUG,"set rate to %f (time scale %f)", setting, expand);
  l[0] = 3; /* time scale code */
#ifdef WORDS_BIGENDIAN
  l[1] = p[0]; l[2] = p[1]; l[3] = p[2]; l[4] = p[3];
#else /* WORDS_BIGENDIAN */
  l[1] = p[3]; l[2] = p[2]; l[3] = p[1]; l[4] = p[0];
#endif /* WORDS_BIGENDIAN */
  mywrite(helper_fd_out, &l, 5);
}

static void spk_close (void)
{
  if(helper_fd_in >= 0)
    close(helper_fd_in);
  if(helper_fd_out >= 0)
    close(helper_fd_out);
  helper_fd_in = helper_fd_out = -1;
  speaking = 0;
}

/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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
#define VERSION "BRLTTY External Speech driver, version 0.7 (September 2001)"
#define COPYRIGHT "Copyright (C) 2000-2001 by Stéphane Doyon " \
                  "<s.doyon@videotron.ca>"
/* ExternalSpeech/speech.c - Speech library (driver)
 * For external programs, using my own protocol. Features indexing.
 * Stéphane Doyon <s.doyon@videotron.ca>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "Programs/misc.h"
#include "speech.h"
#include "Programs/spk.h"

#define SPK_HAVE_TRACK
#define SPK_HAVE_EXPRESS

typedef enum {
  PARM_PROGRAM=0,
  PARM_UID, PARM_GID
} DriverParameter;
#define SPKPARMS "program", "uid", "gid"

#include "Programs/spk_driver.h"

static int helper_fd_in = -1, helper_fd_out = -1;
static unsigned short lastIndex, finalIndex;
static char speaking = 0;

static void spk_identify (void)
{
  LogPrint(LOG_NOTICE, VERSION);
  LogPrint(LOG_INFO, "   "COPYRIGHT);
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

static void spk_open (char **parameters)
{
  int fd1[2], fd2[2];
  uid_t uid, gid;
  char
    *extProgPath = parameters[PARM_PROGRAM],
    *s_uid = parameters[PARM_UID],
    *s_gid = parameters[PARM_GID];

  if(!*extProgPath) extProgPath = HELPER_PROG_PATH;
  if(*s_uid) {
    char *ptr;
    uid = strtol(s_uid, &ptr, 0);
    if(*ptr != 0) {
      myerror("Unable to parse uid value '%s'", s_uid);
      return;
    }
  }else uid = UID;
  if(*s_gid) {
    char *ptr;
    gid = strtol(s_gid, &ptr, 0);
    if(*ptr != 0) {
      myerror("Unable to parse gid value '%s'", s_uid);
      return;
    }
  }else gid = GID;

  if(pipe(fd1) < 0
     || pipe(fd2) < 0) {
    myperror("pipe");
    return;
  }
  LogPrint(LOG_DEBUG, "pipe fds: fd1 %d %d, fd2 %d %d",
	   fd1[0],fd1[1], fd2[0],fd2[1]);
  switch(fork()) {
  case -1:
    myperror("fork");
    return;
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
    execl(extProgPath, extProgPath, 0);
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
      return;
    }
  };

  LogPrint(LOG_INFO,"Opened pipe to external speech program '%s'",
	   extProgPath);
}

static void mywrite(int fd, const void *buf, int len)
{
  char *pos = (char *)buf;
  int w;
  if(fd<0) return;
  timeout_yet(0);
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
  } while(len && !timeout_yet(2000));
  if(len)
    myerror("ExternalSpeech: pipe to helper program: write timed out");
}

static int myread(int fd, void *buf, int len)
{
  char *pos = (char *)buf;
  int r;
  int firstTime = 1;
  if(fd<0) return 0;
  timeout_yet(0);
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
  } while(len && !timeout_yet(400));
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

static void spk_close (void)
{
  if(helper_fd_in >= 0)
    close(helper_fd_in);
  if(helper_fd_out >= 0)
    close(helper_fd_out);
  helper_fd_in = helper_fd_out = -1;
  speaking = 0;
}

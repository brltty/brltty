/*
 * BrlTty - A daemon providing access to the Linux console (when in text
 *          mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BrlTty Team. All rights reserved.
 *
 * BrlTty comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */
#define VERSION "BRLTTY External Speech driver, version 0.6 (March 2001)"
#define COPYRIGHT "Copyright (C) 2000-2001 by Stéphane Doyon " \
                  "<s.doyon@videotron.ca>"
/* ExternalSpeech/speech.c - Speech library (driver)
 * For external programs, using my own protocol. Features indexing.
 * Stéphane Doyon <s.doyon@videotron.ca>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <linux/limits.h>
#include <string.h>
#include <stdlib.h>

#define SPEECH_C 1

#include "../misc.h"
#include "speech.h"
#include "../spk.h"

#define SPK_HAVE_TRACK
#define SPK_HAVE_SAYATTRIBS

#include "../spk_driver.h"

static char *extProgPath = NULL;
static uid_t uid, gid;
static int helper_fd_in = -1, helper_fd_out = -1;
static unsigned short lastIndex, finalIndex;
static char speaking = 0;

static void identspk (void)
{
  LogAndStderr(LOG_INFO, VERSION);
  LogAndStderr(LOG_INFO, "   "COPYRIGHT);
}

static void myerror(char *msg)
{
  LogPrint(LOG_ERR, msg);
  closespk();
}
static void myperror(char *msg)
{
  char buf[200];
  if(strlen(msg) > 190)
    msg = "...ERROR MESSAGE TOO LONG!!!";
  strcpy(buf, msg);
  strcat(buf, ": %s");
  LogPrint(LOG_ERR, buf, strerror(errno));
  closespk();
}
static void myperror2(char *msg, void *str1)
{
  char buf[200];
  if(strlen(msg) > 190)
    msg = "...ERROR MESSAGE TOO LONG!!!";
  strcpy(buf, msg);
  strcat(buf, ": %s");
  LogPrint(LOG_ERR, buf, str1, strerror(errno));
  closespk();
}

static void initspk (char *speechparm)
{
  int fd1[2], fd2[2];
  char *ptr, *s_uid, *s_gid;
  char *parm = NULL;
  if(speechparm){
    /* We don't want to modify speechparm, in particular if we are ever
       to reinit this driver... */
    parm = strdup(speechparm);
    if(!parm){
      myperror("strdup -> out of memory");
      return;
    }
    /* but now we must not forget to free parm! */
    extProgPath = strtok_r(parm, ",", &ptr);
    s_uid = strtok_r(NULL, ",", &ptr);
    s_gid = strtok_r(NULL, ",", &ptr);
    if(s_uid) {
      uid = strtol(s_uid, &ptr, 0);
      if(*ptr != 0)
	s_gid = NULL;
    }
    if(s_gid) {
      gid = strtol(s_gid, &ptr, 0);
      if(*ptr != 0)
	s_gid = NULL;
    }
    if(s_uid && !s_gid) {
      myperror2("Unable to parse parameter to ExternalSpeech driver. "
		"Expected format is '<extProgramPath>' or "
		"'<extProgramPath>,<numerical_uid>,<numerical_gid>'. "
		"Got parameter '%s'", speechparm);
      free(parm);
      extProgPath = NULL;
      return;
    }
  }else extProgPath = s_uid = s_gid = NULL;
  if(!extProgPath) extProgPath = HELPER_PROG_PATH;
  extProgPath = strdup(extProgPath);
  if(!extProgPath) {
    myperror("strdup -> out of memory");
    if(parm) free(parm);
    return;
  }
  if(!s_uid) uid = UID;
  if(!s_gid) gid = GID;
  if(parm) free(parm);

  if(pipe(fd1) < 0
     || pipe(fd2) < 0) {
    myperror("pipe");
    return;
  }
  switch(fork()) {
  case -1:
    myperror("fork");
    return;
  case 0: {
    int i;
    if(setgid(gid) <0) {
      myperror2("setgid to %u", (void *)gid);
      _exit(1);
    }
    if(setuid(uid) <0) {
      myperror2("setuid to %u", (void *)uid);
      _exit(1);
    }
    LogPrint(LOG_INFO, "ExternalSpeech program uid is %u, gid is %u",
	     getuid(), getgid());
    if(dup2(fd2[0], 0) < 0 /* stdin */
       || dup2(fd1[1], 1) < 0){ /* stdout */
      myperror("dup2");
      _exit(1);
    }
    for(i=2; i<OPEN_MAX; i++) close(i);
    execl(extProgPath, extProgPath, 0);
    myperror2("Unable to execute external speech program %s: exec",
	     HELPER_PROG_PATH);
    _exit(1);
  }
  default:
    helper_fd_in = fd1[0];
    helper_fd_out = fd2[1];
    close(fd1[1]);
    close(fd2[0]);
    if(fcntl(helper_fd_in, F_SETFL,FNDELAY) < 0
       || fcntl(helper_fd_out, F_SETFL,FNDELAY) < 0) {
      myperror("fcntl F_SETFL FNDELAY");
      return;
    }
  };

  LogPrint(LOG_INFO,"Opened pipe to external speech program '%s'",
	   extProgPath);
}

static void mywrite(int fd, void *buf, int len)
{
  char *pos = (char *)buf;
  int w;
  if(fd<0) return;
  timeout_yet(0);
  do {
    if((w = write(fd, pos, len)) < 0) {
      if(errno == EINTR || errno == EAGAIN) continue;
      else if(errno == EPIPE)
	myperror("ExternalSpeech: pipe to helper program was broken");
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

static void sayit(unsigned char *buffer, int len, int attriblen)
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

static void say(unsigned char *buffer, int len)
{
  sayit(buffer,len,0);
}
static void sayWithAttribs(unsigned char *buffer, int len)
{
  sayit(buffer,len,len);
}

static void processSpkTracking()
{
  unsigned char b[2];
  if(helper_fd_in < 0) return;
  while(myread(helper_fd_in, b, 2)) {
    unsigned inx;
    inx = (b[0]<<8 | b[1]);
    LogPrint(LOG_DEBUG, "spktrk: Received index %u", inx);
    if(inx >= finalIndex) {
      speaking = 0;
      LogPrint(LOG_DEBUG, "spktrk: Done speaking", lastIndex);
	/* do not change last_inx: remain on position of last spoken words,
	   not after them. */
    }else lastIndex = inx;
  }
}

static int trackspk()
{
  return lastIndex;
}

static int isSpeaking()
{
  return speaking;
}

static void mutespk (void)
{
  unsigned char c = 1;
  if(helper_fd_out < 0) return;
  LogPrint(LOG_DEBUG,"mute");
  speaking = 0;
  mywrite(helper_fd_out, &c,1);
}

static void closespk (void)
{
  if(extProgPath) free(extProgPath);
  extProgPath = NULL;
  if(helper_fd_in >= 0)
    close(helper_fd_in);
  if(helper_fd_out >= 0)
    close(helper_fd_out);
  helper_fd_in = helper_fd_out = -1;
  speaking = 0;
}

/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
 *
 * Web Page: http://www.cam.org/~nico/brltty
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 */

/* ExternalSpeech/speech.c - Speech library (driver)
 * For external programs, using my own protocol. Features indexing.
 * Stéphane Doyon <s.doyon@videotron.ca>
 * Version 0.4 beta, September 2000
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <limits.h>
#include <string.h>

#define SPEECH_C 1

#include "../misc.h"
#include "speech.h"
#include "../spk.h"

#define SPK_HAVE_TRACK
#define SPK_HAVE_SAYATTRIBS

#include "../spk_driver.h"

static char *extProgPath = HELPER_PROG_PATH;
static int helper_fd_in = -1, helper_fd_out = -1;
static unsigned short lastIndex, finalIndex;
static char speaking = 0;

static void identspk (void)
{
  LogAndStderr(LOG_NOTICE, "Using External speech program.");
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
static void myperror2(char *msg, char *str1)
{
  char buf[200];
  if(strlen(msg) > 190)
    msg = "...ERROR MESSAGE TOO LONG!!!";
  strcpy(buf, msg);
  strcat(buf, ": %s");
  LogPrint(LOG_ERR, buf, str1, strerror(errno));
  closespk();
}

static void initspk (char *parm)
{
  int fd1[2], fd2[2];
  extProgPath = (parm == NULL) ? HELPER_PROG_PATH : parm;
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
    if(dup2(fd2[0], 0) < 0 /* stdin */
       || dup2(fd1[1], 1) < 0){ /* stdout */
      myperror("dup2");
      return;
    }
    for(i=2; i<OPEN_MAX; i++) close(i);
    execl(extProgPath, HELPER_PROG_PATH, 0);
    myperror2("Unable to execute external speech program %s: exec",
	     HELPER_PROG_PATH);
    _exit(1);
  }
  default:
    helper_fd_in = fd1[0];
    helper_fd_out = fd2[1];
    close(fd1[1]);
    close(fd2[0]);
    if(fcntl(helper_fd_in, F_SETFL,FNDELAY) < 0) {
      myperror("fcntl F_SETFL FNDELAY");
      return;
    }
  };

  LogPrint(LOG_DEBUG,"Opened pipe to external speech program");
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
  } while(len && !timeout_yet(150));
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
      }else if(errno == EPIPE)
	myperror("ExternalSpeech: pipe to helper program was broken");
         /* try to reinit may be ?? */
      else myperror("ExternalSpeech: pipe to helper program: read");
    }else if(r==0)
      myerror("ExternalSpeech: pipe to helper program: read: EOF!");
    if(r<=0) return 0;
    firstTime = 0;
    pos += r; len -= r;
  } while(len && !timeout_yet(150));
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
  mywrite(helper_fd_out, l, 5);
  mywrite(helper_fd_out, buffer, len+attriblen);
  speaking = 1;
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
  mywrite(helper_fd_out, &c,1);
  speaking = 0;
}

static void closespk (void)
{
  if(helper_fd_in >= 0)
    close(helper_fd_in);
  if(helper_fd_out >= 0)
    close(helper_fd_out);
  helper_fd_in = helper_fd_out = -1;
  speaking = 0;
}

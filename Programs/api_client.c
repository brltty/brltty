/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2004 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 * All rights reserved.
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 * Please see the file LGPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* api_client.c handles connection with BrlApi */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif /* HAVE_ALLOCA_H */

#ifdef linux
#include <linux/major.h>
#include <linux/tty.h>
#define MAXIMUM_VIRTUAL_CONSOLE MAX_NR_CONSOLES
#endif /* linux */

#ifdef __OpenBSD__
#define MAXIMUM_VIRTUAL_CONSOLE 16
#endif /* __OpenBSD__ */

#ifndef MAXIMUM_VIRTUAL_CONSOLE
#define MAXIMUM_VIRTUAL_CONSOLE 1
#endif /* MAXIMUM_VIRTUAL_CONSOLE */

#include "api.h"
#include "api_protocol.h"
#include "brldefs.h"
#include "api_common.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b))? (a): (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b) (((a) > (b))? (a): (b))
#endif /* MAX */

/* API states */
#define STCONNECTED 1
#define STRAW 2
#define STCONTROLLINGTTY 4

/* for remembering getaddrinfo error code */
static int gai_error;

/* Some useful global variables */
static uint32_t brlx = 0;
static uint32_t brly = 0;
static int fd = -1; /* Descriptor of the socket connected to BrlApi */
static int truetty = -1;

pthread_mutex_t brlapi_fd_mutex = PTHREAD_MUTEX_INITIALIZER; /* to protect concurrent fd access */
static int state = 0;
static pthread_mutex_t stateMutex = PTHREAD_MUTEX_INITIALIZER;

/* key presses buffer, for when key presses are received instead of
 * acknowledgements for instance
 *
 * every function must hence be able to read at least sizeof(brl_keycode_t) */

static brl_keycode_t keybuf[BRL_KEYBUF_SIZE];
static unsigned keybuf_next;
static unsigned keybuf_nb;
static pthread_mutex_t keybuf_mutex = PTHREAD_MUTEX_INITIALIZER; /* to protect concurrent key buffering */

static brlapi_errorHandler_t brlapi_errorHandler = brlapi_defaultErrorHandler;
static pthread_mutex_t brlapi_errorHandler_mutex = PTHREAD_MUTEX_INITIALIZER;

static void handle_keycode(brl_keycode_t code)
{
  pthread_mutex_lock(&keybuf_mutex);
  if (keybuf_nb>=BRL_KEYBUF_SIZE) {
    pthread_mutex_unlock(&keybuf_mutex);
    syslog(LOG_WARNING,"lost key 0x%x !\n",code);
  } else {
    keybuf[(keybuf_next+keybuf_nb++)%BRL_KEYBUF_SIZE]=ntohl(code);
    pthread_mutex_unlock(&keybuf_mutex);
  }
}

/* brlapi_waitForPacket */
/* Waits for the specified type of packet: must be called with brlapi_fd_mutex locked */
/* If the right packet type arrives, returns its size */
/* Returns -1 if a non-fatal error is encountered */
/* Calls the error handler if an exception is encountered */
static ssize_t brlapi_waitForPacket(brl_type_t expectedPacketType, void *packet, size_t size)
{
  unsigned char *localPacket[BRLAPI_MAXPACKETSIZE];
  int st;
  brl_type_t type;
  ssize_t res;
  brl_keycode_t *code = (brl_keycode_t *) localPacket;
  errorPacket_t *errorPacket = (errorPacket_t *) localPacket;
  int hdrSize = sizeof(errorPacket->code)+sizeof(errorPacket->type);
  while ((res=brlapi_readPacket(fd,&type,localPacket,sizeof(localPacket)))>=0) {
    if (type==expectedPacketType) {
      if (res<=size) {
        if (res>0) memcpy(packet, localPacket, res);
        return res;
      } else {
        brlapi_libcerrno=EFBIG;
        brlapi_libcerrfun="brlapi_waitForPacket";
        brlapi_errno=BRLERR_LIBCERR;
        return -1;
      }
    }
    if (type==BRLPACKET_ERROR) {
      brlapi_errno = ntohl(*code);
      return -1;
    }
    if (type==BRLPACKET_EXCEPTION) {
      size_t esize;
      if (res<hdrSize) esize = 0; else esize = res-hdrSize;
      brlapi_errorHandler(ntohl(errorPacket->code), ntohl(errorPacket->type), &errorPacket->packet, esize);
      continue;
    }
    pthread_mutex_lock(&stateMutex);
    st = state;
    pthread_mutex_unlock(&stateMutex);
    if ((type==BRLPACKET_KEY) && (st & STCONTROLLINGTTY) && (res==sizeof(brl_keycode_t))) {
      handle_keycode(ntohl(*code));
      continue;
    }
    syslog(LOG_ERR,"(brlapi_waitForPacket) Received unexpected packet of type %s and size %d\n",brlapi_packetType(type),res);
  }
  if (res==-2) brlapi_errno=BRLERR_EOF;
  return -1;
}

/* brlapi_waitForAck */
/* Wait for an acknowledgement, must be called with brlapi_fd_mutex locked */
static int brlapi_waitForAck()
{
  return brlapi_waitForPacket(BRLPACKET_ACK, NULL, 0);
}

/* brlapi_writePacketWaitForAck */
/* write a packet and wait for an acknowledgement */
static int brlapi_writePacketWaitForAck(int fd, brl_type_t type, const void *buf, size_t size)
{
  ssize_t res;
  pthread_mutex_lock(&brlapi_fd_mutex);
  if ((res=brlapi_writePacket(fd,type,buf,size))<0) {
    pthread_mutex_unlock(&brlapi_fd_mutex);
    return res;
  }
  res=brlapi_waitForAck();
  pthread_mutex_unlock(&brlapi_fd_mutex);
  return res;
}

/* Function : updateSettings */
/* Updates the content of a brlapi_settings_t structure according to */
/* another structure of the same type */
static void updateSettings(brlapi_settings_t *s1, const brlapi_settings_t *s2)
{
  if (s2==NULL) return;
  if ((s2->authKey) && (*s2->authKey))
    s1->authKey = s2->authKey;
  if ((s2->hostName) && (*s2->hostName))
    s1->hostName = s2->hostName;
}

/* Function: brlapi_initializeConnection
 * Creates a socket to connect to BrlApi */
int brlapi_initializeConnection(const brlapi_settings_t *clientSettings, brlapi_settings_t *usedSettings)
{
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  authStruct *auth = (authStruct *) packet;
  struct addrinfo *res,*cur;
  struct addrinfo hints;
  char *hostname = NULL;
  char *port = NULL;
  int err;
  size_t authKeyLength;

  brlapi_settings_t settings = { BRLAPI_DEFAUTHPATH, "127.0.0.1:" BRLAPI_SOCKETPORT };
  brlapi_settings_t envsettings = { getenv("BRLAPI_AUTHPATH"), getenv("BRLAPI_HOSTNAME") };

  /* Here update settings with the parameters from misc sources (files, env...) */
  updateSettings(&settings, &envsettings);
  updateSettings(&settings, clientSettings);
  if (usedSettings!=NULL) updateSettings(usedSettings, &settings);

  if ((err=brlapi_loadAuthKey(settings.authKey,&authKeyLength,(void *) &auth->key))<0)
    return err;

  auth->protocolVersion = BRLAPI_PROTOCOL_VERSION;

  brlapi_splitHost(settings.hostName,&hostname,&port);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  gai_error = getaddrinfo(hostname, port, &hints, &res);
  if (hostname)
    free(hostname);
  free(port);
  if (gai_error) {
    brlapi_errno=BRLERR_GAIERR;
    return -1;
  }
  pthread_mutex_lock(&brlapi_fd_mutex);
  for(cur = res; cur; cur = cur->ai_next) {
    fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
    if (fd<0) continue;
    if (connect(fd, cur->ai_addr, cur->ai_addrlen)<0) {
      close(fd);
      fd = -1;
      continue;
    }
    break;
  }
  freeaddrinfo(res);
  if (!cur) {
    pthread_mutex_unlock(&brlapi_fd_mutex);
    brlapi_errno=BRLERR_CONNREFUSED;
    return -1;
  }
  if ((err=brlapi_writePacket(fd, BRLPACKET_AUTHKEY, packet, sizeof(auth->protocolVersion)+authKeyLength))<0) {
    pthread_mutex_unlock(&brlapi_fd_mutex);
    close(fd);
    fd = -1;
    return err;
  }
  if ((err=brlapi_waitForAck())<0) {
    pthread_mutex_unlock(&brlapi_fd_mutex);
    close(fd);
    fd = -1;
    return err;
  }
  pthread_mutex_unlock(&brlapi_fd_mutex);

  pthread_mutex_lock(&stateMutex);
  state = STCONNECTED;
  pthread_mutex_unlock(&stateMutex);
  return fd;
}

/* brlapi_closeConnection */
/* Cleanly close the socket */
void brlapi_closeConnection()
{
  pthread_mutex_lock(&stateMutex);
  state = 0;
  pthread_mutex_unlock(&stateMutex);
  pthread_mutex_lock(&brlapi_fd_mutex);
  close(fd);
  fd = -1;
  pthread_mutex_unlock(&brlapi_fd_mutex);
}

/* brlapi_getRaw */
/* Switch to Raw mode */
int brlapi_getRaw()
{
  int res;
  const uint32_t magic = htonl(BRLRAW_MAGIC);
  res = brlapi_writePacketWaitForAck(fd, BRLPACKET_GETRAW, &magic, sizeof(magic));
  if (res!=-1) {
    pthread_mutex_lock(&stateMutex);
    state |= STRAW;
    pthread_mutex_unlock(&stateMutex);
  }
  return res;
}

/* brlapi_leaveRaw */
/* Leave Raw mode */
int brlapi_leaveRaw()
{
  int res = brlapi_writePacketWaitForAck(fd, BRLPACKET_LEAVERAW, NULL, 0);
  pthread_mutex_lock(&stateMutex);
  state &= ~STRAW;
  pthread_mutex_unlock(&stateMutex);
  return res;
}

/* brlapi_sendRaw */
/* Send a Raw Packet */
ssize_t brlapi_sendRaw(const unsigned char *buf, size_t size)
{
  ssize_t res;
  pthread_mutex_lock(&brlapi_fd_mutex);
  res=brlapi_writePacket(fd, BRLPACKET_PACKET, buf, size);
  pthread_mutex_unlock(&brlapi_fd_mutex);
  return res;
}

/* brlapi_recvRaw */
/* Get a Raw packet */
ssize_t brlapi_recvRaw(unsigned char *buf, size_t size)
{
  ssize_t res;
  pthread_mutex_lock(&brlapi_fd_mutex);
  res = brlapi_waitForPacket(BRLPACKET_PACKET, buf, size);
  pthread_mutex_unlock(&brlapi_fd_mutex);
  return res;
}

/* Function brlapi_request */
/* Sends a request to the API and waits for the answer */
/* The answer is put in the given packet */
static ssize_t brlapi_request(brl_type_t request, void *packet, size_t size)
{
  ssize_t res;
  pthread_mutex_lock(&brlapi_fd_mutex);
  res = brlapi_writePacket(fd, request, NULL, 0);
  if (res==-1) {
    pthread_mutex_unlock(&brlapi_fd_mutex);
    return -1;
  }
  res = brlapi_waitForPacket(request, packet, size);
  pthread_mutex_unlock(&brlapi_fd_mutex);
  return res;
}

/* Function : brlapi_getDriverId */
/* Identify the driver used by brltty */
int brlapi_getDriverId(unsigned char *id, size_t n)
{
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  ssize_t res = brlapi_request(BRLPACKET_GETDRIVERID, packet, sizeof(packet));
  if (res<0) return -1;
  return snprintf(id, n, "%s", packet);
}

/* Function : brlapi_getDriverName */
/* Name of the driver used by brltty */
int brlapi_getDriverName(unsigned char *name, size_t n)
{
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  ssize_t res = brlapi_request(BRLPACKET_GETDRIVERNAME, packet, sizeof(packet));
  if (res<0) return -1;
  return snprintf(name, n, "%s", packet);
}

/* Function : brlapi_getDisplaySize */
/* Returns the size of the braille display */
int brlapi_getDisplaySize(unsigned int *x, unsigned int *y)
{
  uint32_t displaySize[2];
  ssize_t res;

  if (brlx*brly) { *x = brlx; *y = brly; return 0; }
  res = brlapi_request(BRLPACKET_GETDISPLAYSIZE, displaySize, sizeof(displaySize)); 
  if (res==-1) { return -1; }
  brlx = ntohl(displaySize[0]);
  brly = ntohl(displaySize[1]);
  *x = brlx; *y = brly;
  return 0;
}

/* Function : brlapi_getControllingTty */
/* Returns the number of the caller's controlling terminal */
/* -1 if error or unknown */
static int brlapi_getControllingTty()
{
  int tty;
  const char *env;

#ifdef linux
  {
    int vt = 0;
    pid_t pid = getpid();
    while (pid != 1) {
      int ok = 0;
      char path[0X40];
      FILE *stream;

      int process;
      char command[0X200];
      char status;
      int parent;
      int group;
      int session;
      int tty;

      snprintf(path, sizeof(path), "/proc/%d/stat", pid);
      if ((stream = fopen(path, "r"))) {
        if (fscanf(stream, "%d %s %c %d %d %d %d", &process, command, &status, &parent, &group, &session, &tty) >= 7)
          if (process == pid)
            ok = 1;
        fclose(stream);
      }
      if (!ok) break;

      if (major(tty) == TTY_MAJOR) {
        vt = minor(tty);
        break;
      }

      pid = parent;
    }
    if ((vt >= 1) && (vt <= MAXIMUM_VIRTUAL_CONSOLE)) return vt;
  }
#endif /* linux */
  /*if ((env = getenv("WINDOW")) && sscanf(env, "%u", &tty) == 1) return tty;*/
  if ((env = getenv("WINDOWID")) && sscanf(env, "%u", &tty) == 1) return tty;
  if ((env = getenv("CONTROLVT")) && sscanf(env, "%u", &tty) == 1) return tty;
  return -1;
}

/* Function : brlapi_getTty */
/* Takes control of a tty */
int brlapi_getTty(int tty, int how)
{
  uint32_t uints[BRLAPI_MAXPACKETSIZE/sizeof(uint32_t)],*curuints=uints;
  int res;
  char *ttytreepath,*ttytreepathstop;
  int ttypath;

  /* Determine which tty to take control of */
  if (tty<=0) truetty = brlapi_getControllingTty(); else truetty = tty;
  // 0 can be a valid screen WINDOW
  // 0xffffffff can not be a valid WINDOWID (top 3 bits guaranteed to be zero)
  if (truetty<0) { brlapi_errno=BRLERR_UNKNOWNTTY; return -1; }
  
  if (brlapi_getDisplaySize(&brlx, &brly)<0) return -1;
  
  /* Clear key buffer before taking the tty, just in case... */
  pthread_mutex_lock(&keybuf_mutex);
  keybuf_next = keybuf_nb = 0;
  pthread_mutex_unlock(&keybuf_mutex);

  /* OK, Now we know where we are, so get the effective control of the terminal! */
  ttytreepath = getenv("WINDOWSPATH");
  if (ttytreepath)
  for(; *ttytreepath && curuints-uints+2<=BRLAPI_MAXPACKETSIZE/sizeof(uint32_t);
      *curuints++ = htonl(ttypath), ttytreepath = ttytreepathstop+1) {
    ttypath=strtol(ttytreepath,&ttytreepathstop,0);
    /* TODO: log it out. check overflow/underflow & co */
    if (ttytreepathstop==ttytreepath) break;
  }

  *curuints++ = htonl(truetty); 
  *curuints++ = htonl(how);
  if ((res=brlapi_writePacketWaitForAck(fd,BRLPACKET_GETTTY,&uints[0],(curuints-uints)*sizeof(uint32_t)))<0)
    return res;

  pthread_mutex_lock(&stateMutex);
  state |= STCONTROLLINGTTY;
  pthread_mutex_unlock(&stateMutex);

  return truetty;
}

/* Function : brlapi_leaveTty */
/* Gives back control of our tty to brltty */
int brlapi_leaveTty()
{
  int res;
  brlx = 0; brly = 0;
  res = brlapi_writePacketWaitForAck(fd,BRLPACKET_LEAVETTY,NULL,0);
  pthread_mutex_lock(&stateMutex);
  state &= ~STCONTROLLINGTTY;
  pthread_mutex_unlock(&stateMutex);  
  return res;
}

/* Function : brlapi_setFocus */
/* sends the current focus to brltty */
int brlapi_setFocus(int tty)
{
  uint32_t utty;
  utty = htonl(tty);
  int res;
  pthread_mutex_lock(&brlapi_fd_mutex);
  res = brlapi_writePacket(fd, BRLPACKET_SETFOCUS, &utty, sizeof(utty));
  pthread_mutex_unlock(&brlapi_fd_mutex);
  return res;
}

/* Function : brlapi_writeText */
/* Writes a string to the braille display */
int brlapi_writeText(int cursor, const unsigned char *str)
{
  int dispSize = brlx * brly;
  uint32_t min, i;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  extWriteStruct *ews = (extWriteStruct *) packet;
  unsigned char *p = &ews->data;
  int res;
  if ((dispSize == 0) || (dispSize > BRLAPI_MAXPACKETSIZE/4)) {
    brlapi_errno=BRLERR_INVALID_PARAMETER;
    return -1;
  }
  ews->flags = BRLAPI_EWF_TEXT;
  min = MIN( strlen(str), dispSize);
  strncpy(p,str,min);
  p += min;
  for (i = min; i<dispSize; i++,p++) *p = ' ';
  if ((cursor>=0) && (cursor<=dispSize)) {
    ews->flags |= BRLAPI_EWF_CURSOR;
    *((uint32_t *) p) = htonl(cursor);
    p += sizeof(cursor);
  }
  pthread_mutex_lock(&brlapi_fd_mutex);
  res=brlapi_writePacket(fd,BRLPACKET_EXTWRITE,packet,sizeof(ews->flags)+(p-&ews->data));
  pthread_mutex_unlock(&brlapi_fd_mutex);
  return res;
}

/* Function : brlapi_writeDots */
/* Writes dot-matrix to the braille display */
int brlapi_writeDots(const unsigned char *dots)
{
  int res;
  uint32_t size = brlx * brly;
  brlapi_extWriteStruct ews;
  if (size == 0) {
    brlapi_errno=BRLERR_INVALID_PARAMETER;
    return -1;
  }
  ews.displayNumber = -1;
  ews.regionBegin = 0; ews.regionEnd = 0;
  ews.text = malloc(size);
  if (ews.text==NULL) {
    brlapi_errno = BRLERR_NOMEM;
    return -1;
  }
  ews.attrOr = malloc(size);
  if (ews.attrOr==NULL) {
    free(ews.text);
    brlapi_errno = BRLERR_NOMEM;
    return -1;
  }
  memset(ews.text, 0, size);
  memcpy(ews.attrOr, dots, size);
  ews.attrAnd = NULL;
  ews.cursor = 0;
  res = brlapi_extWriteBrl(&ews);
  free(ews.text);
  free(ews.attrOr);
  return res;
}

/* Function : brlapi_extWrite */
/* Extended writes on braille displays */
int brlapi_extWriteBrl(const brlapi_extWriteStruct *s)
{
  int dispSize = brlx * brly;
  uint32_t rbeg, rend, strLen;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  extWriteStruct *ews = (extWriteStruct *) packet;
  unsigned char *p = &ews->data;
  int res;
  ews->flags = 0;
  if ((1<=s->regionBegin) && (s->regionBegin<=dispSize) && (1<=s->regionEnd) && (s->regionEnd<=dispSize)) {
    if (s->regionBegin>s->regionEnd) return 0;
    rbeg = s->regionBegin; rend = s->regionEnd;
    ews->flags |= BRLAPI_EWF_REGION;
    *((uint32_t *) p) = htonl(rbeg); p += sizeof(uint32_t);
    *((uint32_t *) p) = htonl(rend); p += sizeof(uint32_t);
  } else {
    rbeg = 1; rend = dispSize;
  }
  strLen = (rend-rbeg) + 1;
  if (s->text) {
    ews->flags |= BRLAPI_EWF_TEXT;
    memcpy(p, s->text, strLen);
    p += strLen;
  }
  if (s->attrAnd) {
    ews->flags |= BRLAPI_EWF_ATTR_AND;
    memcpy(p, s->attrAnd, strLen);
    p += strLen;
  }
  if (s->attrOr) {
    ews->flags |= BRLAPI_EWF_ATTR_OR;
    memcpy(p, s->attrOr, strLen);
    p += strLen;
  }
  if ((s->cursor>=0) && (s->cursor<=dispSize)) {
    ews->flags |= BRLAPI_EWF_CURSOR;
    *((uint32_t *) p) = htonl(s->cursor);
    p += sizeof(uint32_t);
  }
  pthread_mutex_lock(&brlapi_fd_mutex);
  res = brlapi_writePacket(fd,BRLPACKET_EXTWRITE,packet,sizeof(ews->flags)+(p-&ews->data));
  pthread_mutex_unlock(&brlapi_fd_mutex);
  return res;
}

/* Function : packetReady */
/* Tests wether a packet is ready on file descriptor fd */
/* Returns -1 if an error occurs, 0 if no packet is ready, 1 if there is a */
/* packet ready to be read */
static int packetReady(int fd)
{
  fd_set set;
  struct timeval timeout;
  memset(&timeout, 0, sizeof(timeout));
  FD_ZERO(&set);
  FD_SET(fd, &set);
  return select(fd+1, &set, NULL, NULL, &timeout);
}

/* Function : brlapi_readKey */
/* Reads a key from the braille keyboard */
int brlapi_readKey(int block, brl_keycode_t *code)
{
  ssize_t res;

  pthread_mutex_lock(&stateMutex);
  if (!(state & STCONTROLLINGTTY)) {
    pthread_mutex_unlock(&stateMutex);
    brlapi_errno = BRLERR_ILLEGAL_INSTRUCTION;
    return -1;
  }
  pthread_mutex_unlock(&stateMutex);

  pthread_mutex_lock(&keybuf_mutex);
  if (keybuf_nb>0) {
    *code=keybuf[keybuf_next];
    keybuf_next=(keybuf_next+1)%BRL_KEYBUF_SIZE;
    keybuf_nb--;
    pthread_mutex_unlock(&keybuf_mutex);
    return 1;
  }
  pthread_mutex_unlock(&keybuf_mutex);

  pthread_mutex_lock(&brlapi_fd_mutex);
  if (!block) {
    res = packetReady(fd);
    if (res<=0) {
      pthread_mutex_unlock(&brlapi_fd_mutex);
      return res;
    }
  }
  res=brlapi_waitForPacket(BRLPACKET_KEY, code, sizeof(*code));
  pthread_mutex_unlock(&brlapi_fd_mutex);
  if (res<0) return -1;
  *code = ntohl(*code);
  return 1;
}

/* Function : ignore_unignore_key_range */
/* Common tasks for ignoring and unignoring key ranges */
/* what = 0 for ignoring !0 for unignoring */
static int ignore_unignore_key_range(int what, brl_keycode_t x, brl_keycode_t y)
{
  brl_keycode_t ints[2] = { htonl(x), htonl(y) };

  return brlapi_writePacketWaitForAck(fd,(what ? BRLPACKET_UNIGNOREKEYRANGE : BRLPACKET_IGNOREKEYRANGE),ints,sizeof(ints));
}

/* Function : brlapi_ignoreKeyRange */
int brlapi_ignoreKeyRange(brl_keycode_t x, brl_keycode_t y)
{
  return ignore_unignore_key_range(0,x,y);
}

/* Function : brlapi_unignoreKeyRange */
int brlapi_unignoreKeyRange(brl_keycode_t x, brl_keycode_t y)
{
  return ignore_unignore_key_range(!0,x,y);
}

/* Function : ignore_unignore_key_set */
/* Common tasks for ignoring and unignoring key sets */
/* what = 0 for ignoring !0 for unignoring */
static int ignore_unignore_key_set(int what, const brl_keycode_t *s, uint32_t n)
{
  size_t size = n*sizeof(brl_keycode_t);
  if (size>BRLAPI_MAXPACKETSIZE) {
    brlapi_errno = BRLERR_INVALID_PARAMETER;
    return -1;
  }
  return brlapi_writePacketWaitForAck(fd,(what ? BRLPACKET_UNIGNOREKEYSET : BRLPACKET_IGNOREKEYSET),s,size);
}

/* Function : brlapi_ignoreKeySet */
int brlapi_ignoreKeySet(const brl_keycode_t *s, uint32_t n)
{
  return ignore_unignore_key_set(0,s,n);
}

/* Function : brlapi_unignoreKeySet */
int brlapi_unignoreKeySet(const brl_keycode_t *s, uint32_t n)
{
  return ignore_unignore_key_set(!0,s,n);
}

/* Error code handling */

/* brlapi_errlist: error messages */
const char *brlapi_errlist[] = {
  "Success",                            /* BRLERR_SUCESS */
  "Not enough memory",                  /* BRLERR_NOMEM */
  "Tty Busy",                           /* BRLERR_TTYBUSY */
  "Raw mode busy",                      /* BRLERR_RAWMODEBUSY */
  "Unknown instruction",                /* BRLERR_UNKNOWN_INSTRUCTION */
  "Illegal instruction",                /* BRLERR_ILLEGAL_INSTRUCTION */
  "Invalid parameter",                  /* BRLERR_INVALID_PARAMETER */
  "Invalid packet",                     /* BRLERR_INVALID_PACKET */
  "Raw mode not supported by driver",   /* BRLERR_RAWNOTSUPP */
  "Key codes not supported by driver",  /* BRLERR_KEYSNOTSUPP */
  "Connection refused",                 /* BRLERR_CONNREFUSED */
  "Operation not supported",            /* BRLERR_OPNOTSUPP */
  "getaddrinfo error",                  /* BRLERR_GAIERR */
  "libc error",                         /* BRLERR_LIBCERR */
  "couldn't find out tty number",       /* BRLERR_UNKNOWNTTY */
  "bad protocol version",               /* BRLERR_PROTOCOL_VERSION */
  "unexpected end of file",             /* BRLERR_EOF */
  "too many levels of recursion",       /* BRLERR_TOORECURSE */
};

/* brlapi_nerr: last error number */
const int brlapi_nerr = (sizeof(brlapi_errlist)/sizeof(char*)) - 1;

/* brlapi_strerror: return error message */
const char *brlapi_strerror(int err)
{
  if (err>=brlapi_nerr)
    return "Unknown error";
  else if (err==BRLERR_GAIERR)
    return gai_strerror(gai_error);
  else if (err==BRLERR_LIBCERR)
    return strerror(brlapi_libcerrno);
  else
    return brlapi_errlist[err];
}

/* brlapi_perror: error message printing */
void brlapi_perror(const char *s)
{
  fprintf(stderr,"%s: %s\n",s,brlapi_strerror(brlapi_errno));
}

/* XXX functions mustn't use brlapi_errno after this since it #undefs it XXX */

#ifdef brlapi_errno
#undef brlapi_errno
#endif

int brlapi_errno;
static int pthread_errno_ok;

/* we need a per-thread errno variable, thanks to pthread_keys */
static pthread_key_t errno_key;

/* the key must be created at most once */
static pthread_once_t errno_key_once = PTHREAD_ONCE_INIT;

/* We need to declare these with __attribute__((weak)) to determine at runtime 
 * whether libpthread is used or not. We can't rely on the functions prototypes,
 * hence the use of typeof
 */
#define WEAK_REDEFINE(name) extern typeof(name) name __attribute__((weak))
WEAK_REDEFINE(pthread_key_create);
WEAK_REDEFINE(pthread_once);
WEAK_REDEFINE(pthread_getspecific);
WEAK_REDEFINE(pthread_setspecific);

static void errno_key_free(void *key)
{
  free(key);
}

static void errno_key_alloc(void)
{
  pthread_errno_ok=!pthread_key_create(&errno_key, errno_key_free);
}

/* how to get per-thread errno variable. This will be called by the macro
 * brlapi_errno */
int *brlapi_errno_location(void)
{
  int *errnop;
  if (pthread_once && pthread_key_create) {
    pthread_once(&errno_key_once, errno_key_alloc);
    if (pthread_errno_ok) {
      if ((errnop=(int *) pthread_getspecific(errno_key)))
        /* normal case */
        return errnop;
      else
        /* on the first time, must allocate it */
        if ((errnop=malloc(sizeof(*errnop))) && !pthread_setspecific(errno_key,errnop))
          return errnop;
      }
    }
  /* fall-back: shared errno :/ */
  return &brlapi_errno;
}

brlapi_errorHandler_t brlapi_setErrorHandler(brlapi_errorHandler_t new)
{
  brlapi_errorHandler_t tmp;
  pthread_mutex_lock(&brlapi_errorHandler_mutex);
  tmp = brlapi_errorHandler;
  if (new!=NULL) brlapi_errorHandler = new;
  pthread_mutex_unlock(&brlapi_errorHandler_mutex);
  return tmp;
}

void brlapi_defaultErrorHandler(int err, brl_type_t type, const void *packet, size_t size)
{
  unsigned char *c;
  fprintf(stderr, "Error: %s on %s request:\n",brlapi_strerror(err),brlapi_packetType(type));
  if (size>16) size=16;
  for (c=0;c<(unsigned char *)packet+size;c++) fprintf(stderr,"%2x ", *c);
  fprintf(stderr,"\n");
  exit(1);
}

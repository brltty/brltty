/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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
#include "brldefs.h"
#include "api_common.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b))? (a): (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b) (((a) > (b))? (a): (b))
#endif /* MAX */

/* for remembering getaddrinfo error code */
static int gai_error;

/* Some useful global variables */
static uint32_t brlx = 0;
static uint32_t brly = 0;
static int fd = -1; /* Descriptor of the socket connected to BrlApi */

pthread_mutex_t brlapi_fd_mutex = PTHREAD_MUTEX_INITIALIZER; /* to protect concurrent fd access */

static int deliver_mode = -1; /* How keypresses are delivered (codes | commands) */

/* key presses buffer, for when key presses are received instead of
 * acknowledgements for instance
 *
 * every function must hence be able to read at least sizeof(brl_keycode_t) */

static brl_keycode_t keybuf[BRL_KEYBUF_SIZE];
static unsigned keybuf_next;
static unsigned keybuf_nb;
static pthread_mutex_t keybuf_mutex = PTHREAD_MUTEX_INITIALIZER; /* to protect concurrent key buffering */

static int handle_keypress(brl_type_t type, uint32_t *code, size_t size) {
  brl_keycode_t *keycode = (brl_keycode_t *) code;
  /* if not a key press, let the caller get it */
  if (type!=BRLPACKET_KEY && type!=BRLPACKET_COMMAND) return 0;

  /* if a key press, but not of the expected type, ignore it */
  if ((type==BRLPACKET_KEY && deliver_mode == BRLCOMMANDS)
      || (type==BRLPACKET_COMMAND && deliver_mode == BRLKEYCODES)) return 1;

  /* if size is not enough, ignore it */
  if (size<sizeof(brl_keycode_t)) return 1;

  pthread_mutex_lock(&keybuf_mutex);
  if (keybuf_nb>=BRL_KEYBUF_SIZE) {
    pthread_mutex_unlock(&keybuf_mutex);
    syslog(LOG_WARNING,"lost key 0x%x !\n",*keycode);
    return 1;
  }

  keybuf[(keybuf_next+keybuf_nb++)%BRL_KEYBUF_SIZE]=ntohl(*keycode);
  pthread_mutex_unlock(&keybuf_mutex);
  return 1;
}

/* brlapi_waitForAck */
/* Wait for an acknowledgement, must be called with brlapi_fd_mutex locked */
static int brlapi_waitForAck()
{
  brl_type_t type;
  ssize_t res;
  uint32_t code;
  while (1) {
    if ((res=brlapi_readPacket(fd,&type,&code,sizeof(code)))<0) {
      if (res==-2) brlapi_errno=BRLERR_EOF;
      return -1;
    }
    if (handle_keypress(type,&code,res)) continue;
    switch (type) {
      case BRLPACKET_ACK: return 0;
      case BRLPACKET_ERROR: {
        brlapi_errno=ntohl(code);
        return -1;
      }
      /* default is ignored, but logged */
      default:
        syslog(LOG_ERR,"(brlapi_waitForAck) Received unknown packet of type %u and size %d\n",type,res);
    }
  }
}

/* brlapi_writePacketWaitForAck */
/* write a packet and wait for an acknowledgement */
static ssize_t brlapi_writePacketWaitForAck(int fd, brl_type_t type, const void *buf, size_t size)
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
  int err, authKeyLength;

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
  return fd;
}

/* brlapi_closeConnection */
/* Cleanly close the socket */
void brlapi_closeConnection()
{
  pthread_mutex_lock(&brlapi_fd_mutex);
  brlapi_writePacket(fd, BRLPACKET_BYE, NULL, 0);
  brlapi_waitForAck();
  close(fd);
  fd = -1;
  pthread_mutex_unlock(&brlapi_fd_mutex);
}

/* brlapi_getRaw */
/* Switch to Raw mode */
int brlapi_getRaw()
{
  const uint32_t magic = htonl(BRLRAW_MAGIC);
  return brlapi_writePacketWaitForAck(fd, BRLPACKET_GETRAW, &magic, sizeof(magic));
}

/* brlapi_leaveRaw */
/* Leave Raw mode */
int brlapi_leaveRaw()
{
  return brlapi_writePacketWaitForAck(fd, BRLPACKET_LEAVERAW, NULL, 0);
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
  brl_type_t type;
  pthread_mutex_lock(&brlapi_fd_mutex);
  while (1) {
    if ((res=brlapi_readPacket(fd, &type, buf, size))<0) {
      pthread_mutex_unlock(&brlapi_fd_mutex);
      return res;
    }
    if (type==BRLPACKET_PACKET) {
      pthread_mutex_unlock(&brlapi_fd_mutex);
      return res;
    }
    if (type==BRLPACKET_ERROR) {
      pthread_mutex_unlock(&brlapi_fd_mutex);
      brlapi_errno=ntohl(*((uint32_t*)buf));
      return -1;
    }
    /* other packets are ignored, but logged */
    syslog(LOG_ERR,"(RecvRaw) Received unknown packet of type %u and size %d\n",type,res);
  }
}

/* Function : brlapi_getDriverId */
/* Identify the driver used by brltty */
int brlapi_getDriverId(unsigned char *id, size_t n)
{
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  brl_type_t type;
  ssize_t res;
  uint32_t *code = (uint32_t *) packet;
  pthread_mutex_lock(&brlapi_fd_mutex);
  if (brlapi_writePacket(fd, BRLPACKET_GETDRIVERID, NULL, 0)<0) {
    pthread_mutex_unlock(&brlapi_fd_mutex);
    return -1;
  }
  while (1) {
    if ((res=brlapi_readPacket(fd,&type, packet, sizeof(packet)))<0) {
      if (res==-2) brlapi_errno=BRLERR_EOF;
      pthread_mutex_unlock(&brlapi_fd_mutex);
      return -1;
    }
    if (handle_keypress(type,code,res)) continue;
    switch (type) {
      case BRLPACKET_GETDRIVERID: {
        pthread_mutex_unlock(&brlapi_fd_mutex);
        return snprintf(id, n, "%s", packet);
      }
      case BRLPACKET_ERROR: {
        pthread_mutex_unlock(&brlapi_fd_mutex);
        brlapi_errno=ntohl(*code);
        return -1;
      }
      /* default is ignored, but logged */
      default:
        syslog(LOG_ERR,"(brlapi_getDriverId) Received unknown packet of type %u and size %d\n",type,res);
    }
  }
}

/* Function : brlapi_getDriverName */
/* Name of the driver used by brltty */
int brlapi_getDriverName(unsigned char *name, size_t n)
{
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  brl_type_t type;
  ssize_t res;
  uint32_t *code = (uint32_t *) packet;
  pthread_mutex_lock(&brlapi_fd_mutex);
  if (brlapi_writePacket(fd, BRLPACKET_GETDRIVERNAME, NULL, 0)<0) {
    pthread_mutex_unlock(&brlapi_fd_mutex);
    return -1;
  }
  while (1) {
    if ((res=brlapi_readPacket(fd, &type, packet, sizeof(packet)))<0) {
      if (res==-2) brlapi_errno=BRLERR_EOF;
      pthread_mutex_unlock(&brlapi_fd_mutex);
      return -1;
    }
    if (handle_keypress(type,code,res)) continue;
    switch (type) {
      case BRLPACKET_GETDRIVERNAME: {
        pthread_mutex_unlock(&brlapi_fd_mutex);
        return snprintf(name, n, "%s", packet);
      }
      case BRLPACKET_ERROR: {
        pthread_mutex_unlock(&brlapi_fd_mutex);
        brlapi_errno=ntohl(*code);
        return -1;
      }
      /* default is ignored, but logged */
      default:
        syslog(LOG_ERR,"(GetDriverName) Received unknown packet of type %u and size %d\n",type,res);
    }
  }
}

/* Function : brlapi_getDisplaySize */
/* Returns the size of the braille display */
int brlapi_getDisplaySize(unsigned int *x, unsigned int *y)
{
  brl_type_t type;
  uint32_t DisplaySize[2];
  ssize_t res;

  if (brlx*brly) {
    *x = brlx;
    *y = brly;
    return 0;
  }
  pthread_mutex_lock(&brlapi_fd_mutex);
  if ((res=brlapi_writePacket(fd, BRLPACKET_GETDISPLAYSIZE, NULL, 0))<0) {
    pthread_mutex_unlock(&brlapi_fd_mutex);
    return res;
  }
  while (1) {
    if ((res=brlapi_readPacket(fd,&type,&DisplaySize[0],sizeof(DisplaySize)))<0) {
      if (res==-2) brlapi_errno=BRLERR_EOF;
      pthread_mutex_unlock(&brlapi_fd_mutex);
      return -1;
    }
    if (handle_keypress(type,&DisplaySize[0],res)) continue;
    switch (type) {
      case BRLPACKET_GETDISPLAYSIZE: {
        pthread_mutex_unlock(&brlapi_fd_mutex);
        brlx = ntohl(DisplaySize[0]);
        brly = ntohl(DisplaySize[1]);
        *x = brlx;
        *y = brly;
        return 0;
      }
      case BRLPACKET_ERROR: {
        pthread_mutex_unlock(&brlapi_fd_mutex);
        brlapi_errno=ntohl(DisplaySize[0]);
        return -1;
      }
      /* default is ignored, but logged */
      default:
        syslog(LOG_ERR,"(brlapi_getDisplaySize) Received unknown packet of type %u and size %d\n",type,res);
    }
  }
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
  if ((env = getenv("WINDOW")) && sscanf(env, "%u", &tty) == 1) return tty;
  if ((env = getenv("WINDOWID")) && sscanf(env, "%u", &tty) == 1) return tty;
  if ((env = getenv("CONTROLVT")) && sscanf(env, "%u", &tty) == 1) return tty;
  return -1;
}

/* Function : brlapi_getTty */
/* Takes control of a tty */
int brlapi_getTty(int tty, int how)
{
  int truetty = -1;
  uint32_t uints[2];
  int res;

  /* Check if "how" is valid */
  if ((how!=BRLKEYCODES) && (how!=BRLCOMMANDS)) {
    brlapi_errno=BRLERR_INVALID_PARAMETER;
    return -1;
  }

  /* Get dimensions of braille display */
  if ((res=brlapi_getDisplaySize(&brlx,&brly))<0) return res; /* failed, we stop here */

  /* Then determine of which tty to take control */
  if (tty<=0) truetty = brlapi_getControllingTty(); else truetty = tty;
  if (truetty<0) { brlapi_errno=BRLERR_UNKNOWNTTY; return truetty; }

  /* OK, Now we know where we are, so get the effective control of the terminal! */
  uints[0] = htonl(truetty);
  uints[1] = htonl(how);
  if ((res=brlapi_writePacketWaitForAck(fd,BRLPACKET_GETTTY,&uints[0],sizeof(uints)))<0)
    return res;

  deliver_mode = how;
  pthread_mutex_lock(&keybuf_mutex);
  keybuf_next = keybuf_nb = 0;
  pthread_mutex_unlock(&keybuf_mutex);

  return truetty;
}

/* Function : brlapi_leaveTty */
/* Gives back control of our tty to brltty */
int brlapi_leaveTty()
{
  brlx = 0; brly = 0; deliver_mode = -1;
  return brlapi_writePacketWaitForAck(fd,BRLPACKET_LEAVETTY,NULL,0);
}

/* Function : brlapi_writeBrl */
/* Writes a string to the braille display */
int brlapi_writeBrl(int cursor, const unsigned char *str)
{
  int dispSize = brlx * brly;
  uint32_t min, i;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  extWriteStruct *ews = (extWriteStruct *) packet;
  unsigned char *p = &ews->data;
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
  return brlapi_writePacketWaitForAck(fd,BRLPACKET_EXTWRITE,packet,sizeof(ews->flags)+(p-&ews->data));
}

/* Function : brlapi_writeBrlDots */
/* Writes dot-matrix to the braille display */
int brlapi_writeBrlDots(const unsigned char *dots)
{
  unsigned char disp[BRLAPI_MAXPACKETSIZE];
  uint32_t size = brlx * brly;
  if ((size == 0) || (size > sizeof(disp))) {
    brlapi_errno=BRLERR_INVALID_PARAMETER;
    return -1;
  }
  memcpy(disp,dots,size);
  return brlapi_writePacketWaitForAck(fd,BRLPACKET_WRITEDOTS,disp,size);
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
  return brlapi_writePacketWaitForAck(fd,BRLPACKET_EXTWRITE,packet,sizeof(ews->flags)+(p-&ews->data));
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
  int res;
  brl_type_t type;

  if (deliver_mode!=BRLKEYCODES) {
    brlapi_errno=BRLERR_INVALID_PARAMETER;
    return -1;
  }
  while (1) {
    /* if a key press was already received, just return it */
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
    res=brlapi_readPacket(fd, &type, code, sizeof(*code));
    pthread_mutex_unlock(&brlapi_fd_mutex);
    if (res<0) {
      if (res==-2) brlapi_errno=BRLERR_EOF;
      return -1;
    }
    if (type==BRLPACKET_KEY) {
      *code = ntohl(*code);
      return 1;
    }
    if (type==BRLPACKET_ERROR)
      syslog(LOG_ERR,"(ReadKey) Received error %d\n",ntohl(*code));
    else
    /* other packets are ignored, but logged */
      syslog(LOG_ERR,"(ReadKey) Received unknown packet of type %u and size %d\n",type,res);
  }
}

/* Function : brlapi_readCommand */
/* Reads a command from the braille keyboard */
int brlapi_readCommand(int block, brl_keycode_t *code)
{
  int res;
  brl_type_t type;

  if (deliver_mode!=BRLCOMMANDS) {
    brlapi_errno=BRLERR_INVALID_PARAMETER;
    return -1;
  }
  while (1) {
    /* if a key press was already received, just return it */
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
    res=brlapi_readPacket(fd, &type, code, sizeof(*code));
    pthread_mutex_unlock(&brlapi_fd_mutex);
    if (res<0) {
      if (res==-2) brlapi_errno=BRLERR_EOF;
      return -1;
    }
    if (type==BRLPACKET_COMMAND) {
      *code = ntohl(*code);
      return 1;
    }
    if (type==BRLPACKET_ERROR)
      syslog(LOG_ERR,"(ReadCommand) Received error %d\n",ntohl(*code));
    else
    /* other packets are ignored, but logged */
      syslog(LOG_ERR,"(ReadCommand) Received unknown packet of type %u and size %d\n",type,res);
  }
}

/* Function : Mask_Unmask */
/* Common tasks for masking and unmasking keys */
/* what = 0 for masing, !0 for unmasking */
static int Mask_Unmask(int what, brl_keycode_t x, brl_keycode_t y)
{
  brl_keycode_t ints[2] = { htonl(x), htonl(y) };

  return brlapi_writePacketWaitForAck(fd,(what ? BRLPACKET_UNMASKKEYS : BRLPACKET_MASKKEYS),ints,sizeof(ints));
}

/* Function : brlapi_ignoreKeys */
int brlapi_ignoreKeys(brl_keycode_t x, brl_keycode_t y)
{
  return Mask_Unmask(0,x,y);
}

/* Function : brlapi_unignoreKeys */
int brlapi_unignoreKeys(brl_keycode_t x, brl_keycode_t y)
{
  return Mask_Unmask(!0,x,y);
}

/* Error code handling */

/* brlapi_errlist: error messages */
const char *brlapi_errlist[] = {
  "Success",                            /* BRLERR_SUCESS */
  "Not enough memory",                  /* BRLERR_NOMEM */
  "Busy Tty",                           /* BRLERR_TTYBUSY */
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
};

/* brlapi_nerr: last error number */
const int brlapi_nerr = (sizeof(brlapi_errlist)/sizeof(char*)) - 1;

/* brlapi_strerror: return error message */
const char *brlapi_strerror(void)
{
  int err=brlapi_errno;
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
  fprintf(stderr,"%s: %s\n",s,brlapi_strerror());
}

#ifdef brlapi_errno
#undef brlapi_errno
#endif

int brlapi_errno;
static int pthread_errno_ok;

/* we need a per-thread errno variable, thanks to pthread_keys */
static pthread_key_t errno_key;

/* the key must be created at most once */
static pthread_once_t errno_key_once = PTHREAD_ONCE_INIT;

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

/* XXX functions mustn't use brlapi_errno after this one since it was #undef'ed XXX */

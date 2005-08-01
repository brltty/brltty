/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2005 by
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
 * Please see the file COPYING-API for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* api_client.c handles connection with BrlApi */

#include "prologue.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <wchar.h>
#ifndef WINDOWS
#include <langinfo.h>
#endif /* WINDOWS */
#include <locale.h>

#ifdef WINDOWS
#include <ws2tcpip.h>
#include <w32api.h>

#ifdef __MINGW32__
#include "win_pthread.h"
#endif /* __MINGW32__ */

#define syslog(level,fmt,...) fprintf(stderr,#level ": " fmt, __VA_ARGS__)
#define setSocketError() do { errno = WSAGetLastError(); } while (0)
#else /* WINDOWS */
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>
#include <syslog.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */

#define setSocketError()
#endif /* WINDOWS */

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

#define BRLAPI(fun) brlapi_ ## fun
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

#ifdef WINDOWS
static WSADATA wsadata;
#endif /* WINDOWS */

/* Some useful global variables */
static unsigned int brlx = 0;
static unsigned int brly = 0;
static int fd = -1; /* Descriptor of the socket connected to BrlApi */

pthread_mutex_t brlapi_fd_mutex = PTHREAD_MUTEX_INITIALIZER; /* to protect concurrent fd access */
static int state = 0;
static pthread_mutex_t stateMutex = PTHREAD_MUTEX_INITIALIZER;

/** key presses buffer size
 *
 * key presses won't be lost provided no more than BRL_KEYBUF_SIZE key presses
 * are done between two calls to brlapi_read* if a call to another function is
 * done in the meanwhile (which needs somewhere to put them before being able
 * to get responses from the server)
 */
#define BRL_KEYBUF_SIZE 256

/* key presses buffer, for when key presses are received instead of
 * acknowledgements for instance
 *
 * every function must hence be able to read at least sizeof(brl_keycode_t) */
 
static brl_keycode_t keybuf[BRL_KEYBUF_SIZE];
static unsigned keybuf_next;
static unsigned keybuf_nb;
static pthread_mutex_t keybuf_mutex = PTHREAD_MUTEX_INITIALIZER; /* to protect concurrent key buffering */

static brlapi_exceptionHandler_t brlapi_exceptionHandler = brlapi_defaultExceptionHandler;
static pthread_mutex_t brlapi_exceptionHandler_mutex = PTHREAD_MUTEX_INITIALIZER;

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
/* Calls the exception handler if an exception is encountered */
static ssize_t brlapi_waitForPacket(brl_type_t expectedPacketType, void *packet, size_t size)
{
  unsigned char *localPacket[BRLAPI_MAXPACKETSIZE];
  int st;
  brl_type_t type;
  ssize_t res;
  brl_keycode_t *code = (brl_keycode_t *) localPacket;
  errorPacket_t *errorPacket = (errorPacket_t *) localPacket;
  int hdrSize = sizeof(errorPacket->code)+sizeof(errorPacket->type);

readNextPacket:
  res = brlapi_readPacketHeader(fd, &type);
  if (res<0) goto out;
  if (type==expectedPacketType) {
    res = brlapi_readPacketContent(fd,res, packet, size);
    if (res<0) goto out;
    return res;
  }
  
  if ((res = brlapi_readPacketContent(fd, res, localPacket, sizeof(localPacket)))<0) goto out;
  if (type==BRLPACKET_ERROR) {
    brlapi_errno = ntohl(*code);
    return -1;
  }
  if (type==BRLPACKET_EXCEPTION) {
    size_t esize;
    if (res<hdrSize) esize = 0; else esize = res-hdrSize;
    brlapi_exceptionHandler(ntohl(errorPacket->code), ntohl(errorPacket->type), &errorPacket->packet, esize);
    goto readNextPacket;
  }
  pthread_mutex_lock(&stateMutex);
  st = state;
  pthread_mutex_unlock(&stateMutex);
  if ((type==BRLPACKET_KEY) && (st & STCONTROLLINGTTY) && (res==sizeof(brl_keycode_t))) {
    handle_keycode(ntohl(*code));
    goto readNextPacket;
  }
  syslog(LOG_ERR,"(brlapi_waitForPacket) Received unexpected packet of type %s and size %d\n",brlapi_packetType(type),res);
  goto readNextPacket;

out:
  if (res==-2) brlapi_errno=BRLERR_EOF;
  return -1;
}

/* brlapi_waitForAck */
/* Wait for an acknowledgement, must be called with brlapi_fd_mutex locked */
static int brlapi_waitForAck(void)
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

/* Function: tryHostName */
/* Tries to connect to the given hostname. */
static int tryHostName(char *hostName) {
  int addrfamily;
  char *hostname = NULL;
  char *port;

#ifdef WINDOWS
  if (WSAStartup(
#ifdef HAVE_GETADDRINFO
	  MAKEWORD(2,0),
#else /* HAVE_GETADDRINFO */
	  MAKEWORD(1,1),
#endif /* HAVE_GETADDRINFO */
	  &wsadata))
    return -1;
#endif /* WINDOWS */

  addrfamily = brlapi_splitHost(hostName,&hostname,&port);

#ifdef PF_LOCAL
  if (addrfamily == PF_LOCAL) {
    int lpath = strlen(BRLAPI_SOCKETPATH),lport;
    lport = strlen(port);
#ifdef WINDOWS
    {
      char path[lpath+lport+1];
      memcpy(path,BRLAPI_SOCKETPATH,lpath);
      memcpy(path+lpath,port,lport+1);
      while ((HANDLE) (fd = (int)CreateFile(path,GENERIC_READ|GENERIC_WRITE,
	      FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL))
	  == INVALID_HANDLE_VALUE) {
	if (GetLastError() != ERROR_PIPE_BUSY) {
	  brlapi_errfun="CreateFile";
	  goto outlibc;
	}
	WaitNamedPipe(path,NMPWAIT_WAIT_FOREVER);
      }
    }
#else /* WINDOWS */
    {
      struct sockaddr_un sa;
      if (lpath+lport+1>sizeof(sa.sun_path)) {
	brlapi_libcerrno=ENAMETOOLONG;
	brlapi_errfun="path";
	brlapi_errno = BRLERR_LIBCERR;
	goto out;
      }
      if ((fd = socket(PF_LOCAL, SOCK_STREAM, 0))<0) {
	brlapi_errfun="socket";
	setSocketError();
	goto outlibc;
      }
      sa.sun_family = AF_LOCAL;
      memcpy(sa.sun_path,BRLAPI_SOCKETPATH,lpath);
      memcpy(sa.sun_path+lpath,port,lport+1);
      if (connect(fd, (struct sockaddr *) &sa, sizeof(sa))<0) {
	brlapi_errfun="connect";
	setSocketError();
	goto outlibc;
      }
    }
#endif /* WINDOWS */
  } else {
#else /* PF_LOCAL */
  if (0) {} else {
#endif /* PF_LOCAL */

#ifdef HAVE_GETADDRINFO

    struct addrinfo *res,*cur;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    brlapi_gaierrno = getaddrinfo(hostname, port, &hints, &res);
    if (brlapi_gaierrno) {
      brlapi_errno=BRLERR_GAIERR;
      goto out;
    }
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
      brlapi_errno=BRLERR_CONNREFUSED;
      goto out;
    }

#else /* HAVE_GETADDRINFO */

    struct sockaddr_in addr;
    struct hostent *he;

    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    if (!port)
      addr.sin_port = htons(BRLAPI_SOCKETPORTNUM);
    else {
      char *c;
      addr.sin_port = htons(strtol(port, &c, 0));
      if (*c) {
	struct servent *se;
	
	if (!(se = getservbyname(port,"tcp"))) {
	  brlapi_gaierrno = h_errno;
          brlapi_errno=BRLERR_GAIERR;
	  goto out;
	}
	addr.sin_port = se->s_port;
      }
    }

    if (!hostname) {
      addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    } else if ((addr.sin_addr.s_addr = inet_addr(hostname)) == htonl(INADDR_NONE)) {
      if (!(he = gethostbyname(hostname))) {
	brlapi_gaierrno = h_errno;
	brlapi_errno = BRLERR_GAIERR;
	goto out;
      }
      if (he->h_addrtype != AF_INET) {
#ifdef EAFNOSUPPORT
        errno = EAFNOSUPPORT;
#else /* EAFNOSUPPORT */
        errno = EINVAL;
#endif /* EAFNOSUPPORT */
	brlapi_errfun = "gethostbyname";
	goto outlibc;
      }
      if (he->h_length > sizeof(addr.sin_addr)) {
        errno = EINVAL;
	brlapi_errfun = "gethostbyname";
	goto outlibc;
      }
      memcpy(&addr.sin_addr,he->h_addr,he->h_length);
    }
    
    fd = socket(addr.sin_family, SOCK_STREAM, 0);
    if (fd<0) {
      brlapi_errfun = "socket";
      setSocketError();
      goto outlibc;
    }
    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr))<0) {
      brlapi_errfun = "connect";
      setSocketError();
      goto outlibc;
    }

#endif /* HAVE_GETADDRINFO */

  }
  free(hostname);
  free(port);
  return 0;

outlibc:
  brlapi_errno = BRLERR_LIBCERR;
  brlapi_libcerrno = errno;
  if (fd>=0) {
    close(fd);
    fd = -1;
  }
out:
  free(hostname);
  free(port);
  return -1;
}

/* Function: tryAuthKey */
/* Tries to authenticate to server with the given key. */
static int tryAuthKey(authStruct *auth, size_t authKeyLength) {
  if ((brlapi_writePacket(fd, BRLPACKET_AUTHKEY, auth, sizeof(auth->protocolVersion)+authKeyLength))<0)
    return -1;
  if ((brlapi_waitForAck())<0)
    return -1;
  return 0;
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
  size_t authKeyLength;

  brlapi_settings_t settings = { BRLAPI_DEFAUTHPATH, ":0" };
  brlapi_settings_t envsettings = { getenv("BRLAPI_AUTHPATH"), getenv("BRLAPI_HOSTNAME") };

  /* Here update settings with the parameters from misc sources (files, env...) */
  updateSettings(&settings, &envsettings);
  updateSettings(&settings, clientSettings);
  if (usedSettings!=NULL) updateSettings(usedSettings, &settings);

  pthread_mutex_lock(&brlapi_fd_mutex);

retry:
  if (tryHostName(settings.hostName)<0) {
    brlapi_error_t error = brlapi_error;
    if (tryHostName(settings.hostName="127.0.0.1:0")<0
#ifdef AF_INET6
      && tryHostName(settings.hostName="::1:0")<0
#endif /* AF_INET6 */
      ) {
      brlapi_error = error;
      goto out;
    }
    if (usedSettings) usedSettings->hostName = settings.hostName;
  }

  auth->protocolVersion = htonl(BRLAPI_PROTOCOL_VERSION);

  /* try with an empty key as well, in case no authentication is needed */
  if (!settings.authKey) {
    if (tryAuthKey(auth,0)<0)
      goto outfd;
    if (usedSettings) usedSettings->authKey = "none";
  } else if (brlapi_loadAuthKey(settings.authKey,&authKeyLength,(void *) &auth->key)<0) {
    brlapi_error_t error = brlapi_error;
    if (tryAuthKey(auth,0)<0) {
      brlapi_error = error;
      goto outfd;
    }
    if (usedSettings) usedSettings->authKey = "none";
  } else if (tryAuthKey(auth,authKeyLength)<0) {
    if (brlapi_errno != BRLERR_CONNREFUSED)
      goto outfd;
    if (fd>=0) {
      close(fd);
      fd = -1;
    }
    settings.authKey = NULL;
    goto retry;
  }
  pthread_mutex_unlock(&brlapi_fd_mutex);

  pthread_mutex_lock(&stateMutex);
  state = STCONNECTED;
  pthread_mutex_unlock(&stateMutex);
  return fd;

outfd:
  if (fd>=0) {
    close(fd);
    fd = -1;
  }
out:
  pthread_mutex_unlock(&brlapi_fd_mutex);
  return -1;
}

/* brlapi_closeConnection */
/* Cleanly close the socket */
void brlapi_closeConnection(void)
{
  pthread_mutex_lock(&stateMutex);
  state = 0;
  pthread_mutex_unlock(&stateMutex);
  pthread_mutex_lock(&brlapi_fd_mutex);
  close(fd);
  fd = -1;
  pthread_mutex_unlock(&brlapi_fd_mutex);
#ifdef WINDOWS
  WSACleanup();
#endif /* WINDOWS */
}

/* brlapi_getRaw */
/* Switch to Raw mode */
int brlapi_getRaw(const char *driver)
{
  int res;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  getRawPacket_t *rawPacket = (getRawPacket_t *) packet;
  unsigned int n = strlen(driver);
  if (n>BRLAPI_MAXNAMELENGTH) {
    brlapi_errno = BRLERR_INVALID_PARAMETER;
    return -1;
  }
  rawPacket->magic = htonl(BRLRAW_MAGIC);
  rawPacket->nameLength = n;
  memcpy(&rawPacket->name, driver, n);
  res = brlapi_writePacketWaitForAck(fd, BRLPACKET_GETRAW, packet, sizeof(uint32_t)+1+n);
  if (res!=-1) {
    pthread_mutex_lock(&stateMutex);
    state |= STRAW;
    pthread_mutex_unlock(&stateMutex);
  }
  return res;
}

/* brlapi_leaveRaw */
/* Leave Raw mode */
int brlapi_leaveRaw(void)
{
  int res = brlapi_writePacketWaitForAck(fd, BRLPACKET_LEAVERAW, NULL, 0);
  pthread_mutex_lock(&stateMutex);
  state &= ~STRAW;
  pthread_mutex_unlock(&stateMutex);
  return res;
}

/* brlapi_sendRaw */
/* Send a Raw Packet */
ssize_t brlapi_sendRaw(const void *buf, size_t size)
{
  ssize_t res;
  pthread_mutex_lock(&brlapi_fd_mutex);
  res=brlapi_writePacket(fd, BRLPACKET_PACKET, buf, size);
  pthread_mutex_unlock(&brlapi_fd_mutex);
  return res;
}

/* brlapi_recvRaw */
/* Get a Raw packet */
ssize_t brlapi_recvRaw(void *buf, size_t size)
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
int brlapi_getDriverId(char *id, size_t n)
{
  ssize_t res = brlapi_request(BRLPACKET_GETDRIVERID, id, n);
  if ((res>0) && (res<=n)) id[res-1] = '\0';
  return res; 
}

/* Function : brlapi_getDriverName */
/* Name of the driver used by brltty */
int brlapi_getDriverName(char *name, size_t n)
{
  ssize_t res = brlapi_request(BRLPACKET_GETDRIVERNAME, name, n);
  if ((res>0) && (res<=n)) name[res-1] = '\0';
  return res;
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

/* Function : getControllingTty */
/* Returns the number of the caller's controlling terminal */
/* -1 if error or unknown */
static int getControllingTty(void)
{
  int tty;
  const char *env;

  /*if ((env = getenv("WINDOW")) && sscanf(env, "%u", &tty) == 1) return tty;*/
  if ((env = getenv("WINDOWID")) && sscanf(env, "%u", &tty) == 1) return tty;
  if ((env = getenv("CONTROLVT")) && sscanf(env, "%u", &tty) == 1) return tty;

#ifdef WINDOWS
#if (_WIN32_WINNT >= Windows2000)
  /* really good guess */
  if ((tty = (int) GetConsoleWindow())) return tty;
#endif /* _WIN32_WINNT */
  if ((tty = (int) GetActiveWindow()) || (tty = (int) GetFocus())) {
    /* good guess, but need to get back up to parent window */
    HWND root = GetDesktopWindow();
    HWND tmp = (HWND) tty;
    while (1) {
      tmp = GetParent(tmp);
      if (!tmp || tmp == root) return tty;
      tty = (int) tmp;
    }
  }
  /* poor guess: assumes that focus is here */
  if ((tty = (int) GetForegroundWindow())) return tty;
#endif /* WINDOWS */

#ifdef linux
  {
    FILE *stream;

    if ((stream = fopen("/proc/self/stat", "r"))) {
      int vt;
      int ok = fscanf(stream, "%*d %*s %*c %*d %*d %*d %d", &tty) == 1;
      fclose(stream);

      if (ok && (major(tty) == TTY_MAJOR)) {
        vt = minor(tty);
        if ((vt >= 1) && (vt <= MAXIMUM_VIRTUAL_CONSOLE)) return vt;
      }
    }
  }
#endif /* linux */

  return -1;
}

/* Function : brlapi_getTty */
/* Takes control of a tty */
int brlapi_getTty(int tty, const char *how)
{
  /* Determine which tty to take control of */
  if (tty<=0) tty = getControllingTty();
  /* 0 can be a valid screen WINDOW
  0xffffffff can not be a valid WINDOWID (top 3 bits guaranteed to be zero) */
  if (tty<0) { brlapi_errno=BRLERR_UNKNOWNTTY; return -1; }
  
  if (brlapi_getTtyPath(&tty, 1, how)) return -1;

  return tty;
}

/* Function : brlapi_getTtyPath */
/* Takes control of a tty path */
int brlapi_getTtyPath(int *ttys, int nttys, const char *how)
{
  int res;
  unsigned char packet[BRLAPI_MAXPACKETSIZE], *p;
  uint32_t *nbTtys = (uint32_t *) packet, *t = nbTtys+1;
  char *ttytreepath,*ttytreepathstop;
  int ttypath;
  unsigned int n;

  if (brlapi_getDisplaySize(&brlx, &brly)<0) return -1;
  
  /* Clear key buffer before taking the tty, just in case... */
  pthread_mutex_lock(&keybuf_mutex);
  keybuf_next = keybuf_nb = 0;
  pthread_mutex_unlock(&keybuf_mutex);

  /* OK, Now we know where we are, so get the effective control of the terminal! */
  *nbTtys = 0;
  ttytreepath = getenv("WINDOWSPATH");
  if (ttytreepath)
  for(; *ttytreepath && t-(nbTtys+1)<=BRLAPI_MAXPACKETSIZE/sizeof(uint32_t);
    *t++ = htonl(ttypath), (*nbTtys)++, ttytreepath = ttytreepathstop+1) {
    ttypath=strtol(ttytreepath,&ttytreepathstop,0);
    /* TODO: log it out. check overflow/underflow & co */
    if (ttytreepathstop==ttytreepath) break;
  }

  for (n=0; n<nttys; n++) *t++ = htonl(ttys[n]);
  (*nbTtys)+=nttys;

  *nbTtys = htonl(*nbTtys);
  p = (unsigned char *) t;
  if (how==NULL) n = 0; else n = strlen(how);
  if (n>BRLAPI_MAXNAMELENGTH) {
    brlapi_errno = BRLERR_INVALID_PARAMETER;
    return -1;
  }
  *p = n;
  p++;
  memcpy(p, how, n);
  p += n;
  if ((res=brlapi_writePacketWaitForAck(fd,BRLPACKET_GETTTY,packet,(p-packet)))<0)
    return res;

  pthread_mutex_lock(&stateMutex);
  state |= STCONTROLLINGTTY;
  pthread_mutex_unlock(&stateMutex);

  return 0;
}

/* Function : brlapi_leaveTty */
/* Gives back control of our tty to brltty */
int brlapi_leaveTty(void)
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
  int res;
  utty = htonl(tty);
  pthread_mutex_lock(&brlapi_fd_mutex);
  res = brlapi_writePacket(fd, BRLPACKET_SETFOCUS, &utty, sizeof(utty));
  pthread_mutex_unlock(&brlapi_fd_mutex);
  return res;
}

/* Function : brlapi_writeText */
/* Writes a string to the braille display */
int brlapi_writeText(int cursor, const char *str)
{
  int dispSize = brlx * brly;
  unsigned int min, i;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  writeStruct *ws = (writeStruct *) packet;
  unsigned char *p = &ws->data;
  char *locale;
  int res;
  if ((dispSize == 0) || (dispSize > BRLAPI_MAXPACKETSIZE/4)) {
    brlapi_errno=BRLERR_INVALID_PARAMETER;
    return -1;
  }
  locale = setlocale(LC_CTYPE,NULL);
  ws->flags = 0;
  if (str) {
    size_t len;
    uint32_t *size;
    ws->flags |= BRLAPI_WF_TEXT;
    size = (uint32_t *) p;
    p += sizeof(*size);
    len = strlen(str);
    if (locale && strcmp(locale,"C")) {
      mbstate_t ps;
      size_t eaten;
      memset(&ps,0,sizeof(ps));
      for (min=0;min<dispSize;min++) {
	eaten = mbrlen(str,len,&ps);
	switch(eaten) {
	  case (size_t)(-2):
	    errno = EILSEQ;
	  case (size_t)(-1):
	    brlapi_libcerrno = errno;
	    brlapi_errfun = "mbrlen";
	    brlapi_errno = BRLERR_LIBCERR;
	    return -1;
	  case 0:
	    goto endcount;
	}
	memcpy(p, str, eaten);
	p += eaten;
	str += eaten;
	len -= eaten;
      }
endcount:
      for (i = min; i<dispSize; i++) p += wcrtomb((char *)p, L' ', &ps);
    } else {
      min = MIN(len, dispSize);
      memcpy(p, str, min);
      p += min;
      memset(p, ' ', dispSize-min);
      p += dispSize-min;
    }
    *size = htonl((p-(unsigned char *)(size+1)));
  }
  if ((cursor>=0) && (cursor<=dispSize)) {
    ws->flags |= BRLAPI_WF_CURSOR;
    *((uint32_t *) p) = htonl(cursor);
    p += sizeof(uint32_t);
  } else if (cursor!=-1) {
    brlapi_errno = BRLERR_INVALID_PARAMETER;
    return -1;
  }

  if (locale && strcmp(locale,"C")) {
    /* not default locale, tell charset to server */
#ifdef WINDOWS
    UINT CP;
    if ((CP = GetACP() || (CP = GetOEMCP()))) {
      *p++ = 'C';
      *p++ = 'P';
      p += sprintf(p, "%d", CP);
    }
#else /* WINDOWS */
    char *lang = nl_langinfo(CODESET);
    size_t len = strlen(lang);
    ws->flags |= BRLAPI_WF_CHARSET;
    *p++ = len;
    memcpy(p, lang, len);
    p += len;
#endif /* WINDOWS */
  }
  ws->flags = htonl(ws->flags);
  pthread_mutex_lock(&brlapi_fd_mutex);
  res = brlapi_writePacket(fd,BRLPACKET_WRITE,packet,sizeof(ws->flags)+(p-&ws->data));
  pthread_mutex_unlock(&brlapi_fd_mutex);
  return res;
}

/* Function : brlapi_writeDots */
/* Writes dot-matrix to the braille display */
int brlapi_writeDots(const unsigned char *dots)
{
  int res;
  unsigned int size = brlx * brly;
  brlapi_writeStruct ws;
  if (size == 0) {
    brlapi_errno=BRLERR_INVALID_PARAMETER;
    return -1;
  }
  ws.displayNumber = -1;
  ws.regionBegin = 0; ws.regionSize = 0;
  ws.text = malloc(size);
  if (ws.text==NULL) {
    brlapi_errno = BRLERR_NOMEM;
    return -1;
  }
  ws.attrOr = malloc(size);
  if (ws.attrOr==NULL) {
    free(ws.text);
    brlapi_errno = BRLERR_NOMEM;
    return -1;
  }
  memset(ws.text, 0, size);
  memcpy(ws.attrOr, dots, size);
  ws.attrAnd = NULL;
  ws.cursor = 0;
  ws.charset = NULL;
  res = brlapi_write(&ws);
  free(ws.text);
  free(ws.attrOr);
  return res;
}

/* Function : brlapi_write */
/* Extended writes on braille displays */
int brlapi_write(const brlapi_writeStruct *s)
{
  int dispSize = brlx * brly;
  unsigned int rbeg = s->regionBegin, rsiz = s->regionSize, strLen;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  writeStruct *ws = (writeStruct *) packet;
  unsigned char *p = &ws->data;
  int res;
  ws->flags = 0;
  if (s==NULL) goto send;
  if ((1<=rbeg) && (rbeg<=dispSize) && (1<=rbeg+rsiz-1) && (rbeg+rsiz-1<=dispSize)) {
    if (rsiz == 0) return 0;
    ws->flags |= BRLAPI_WF_REGION;
    *((uint32_t *) p) = htonl(rbeg); p += sizeof(uint32_t);
    *((uint32_t *) p) = htonl(rsiz); p += sizeof(uint32_t);
  } else {
    rbeg = 1; rsiz = dispSize;
  }
  if (s->text) {
    strLen = rsiz;
    *((uint32_t *) p) = htonl(strLen); p += sizeof(uint32_t);
    ws->flags |= BRLAPI_WF_TEXT;
    memcpy(p, s->text, strLen);
    p += strLen;
  }
  if (s->attrAnd) {
    ws->flags |= BRLAPI_WF_ATTR_AND;
    memcpy(p, s->attrAnd, rsiz);
    p += rsiz;
  }
  if (s->attrOr) {
    ws->flags |= BRLAPI_WF_ATTR_OR;
    memcpy(p, s->attrOr, rsiz);
    p += rsiz;
  }
  if ((s->cursor>=0) && (s->cursor<=dispSize)) {
    ws->flags |= BRLAPI_WF_CURSOR;
    *((uint32_t *) p) = htonl(s->cursor);
    p += sizeof(uint32_t);
  } else if (s->cursor!=-1) {
    brlapi_errno = BRLERR_INVALID_PARAMETER;
    return -1;    
  }
  if (s->charset && *s->charset) {
    strLen = strlen(s->charset);
    *p++ = strLen;
    ws->flags |= BRLAPI_WF_CHARSET;
    strcpy((char *)p, s->charset);
    p += strLen;
  }
  send:
  ws->flags = htonl(ws->flags);
  pthread_mutex_lock(&brlapi_fd_mutex);
  res = brlapi_writePacket(fd,BRLPACKET_WRITE,packet,sizeof(ws->flags)+(p-&ws->data));
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
static int ignore_unignore_key_set(int what, const brl_keycode_t *s, unsigned int n)
{
  size_t size;
  if (n>BRLAPI_MAXKEYSETSIZE) {
    brlapi_errno = BRLERR_INVALID_PARAMETER;
    return -1;
  }
  size = n*sizeof(brl_keycode_t);
  return brlapi_writePacketWaitForAck(fd,(what ? BRLPACKET_UNIGNOREKEYSET : BRLPACKET_IGNOREKEYSET),s,size);
}

/* Function : brlapi_ignoreKeySet */
int brlapi_ignoreKeySet(const brl_keycode_t *s, unsigned int n)
{
  return ignore_unignore_key_set(0,s,n);
}

/* Function : brlapi_unignoreKeySet */
int brlapi_unignoreKeySet(const brl_keycode_t *s, unsigned int n)
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
  "Connection refused",                 /* BRLERR_CONNREFUSED */
  "Operation not supported",            /* BRLERR_OPNOTSUPP */
  "getaddrinfo error",                  /* BRLERR_GAIERR */
  "libc error",                         /* BRLERR_LIBCERR */
  "Couldn't find out tty number",       /* BRLERR_UNKNOWNTTY */
  "Bad protocol version",               /* BRLERR_PROTOCOL_VERSION */
  "Unexpected end of file",             /* BRLERR_EOF */
  "Key file is empty",                  /* BRLERR_EMPTYKEY */
  "Driver error",                       /* BRLERR_DRIVERERROR */
};

/* brlapi_nerr: last error number */
const int brlapi_nerr = (sizeof(brlapi_errlist)/sizeof(char*)) - 1;

/* brlapi_strerror: return error message */
const char *brlapi_strerror(const brlapi_error_t *error)
{
  static char errmsg[128];
  if (error->brlerrno>=brlapi_nerr)
    return "Unknown error";
  else if (error->brlerrno==BRLERR_GAIERR) {
    snprintf(errmsg,sizeof(errmsg),"resolve: "
#if defined(HAVE_GETADDRINFO) && defined(HAVE_GAI_STRERROR)
	"%s\n", gai_strerror(error->exterrno.gaierrno)
#elif defined(HAVE_HSTRERROR) && !defined(WINDOWS)
	"%s\n", hstrerror(error->exterrno.gaierrno)
#else
	"%x\n", error->exterrno.gaierrno
#endif
	);
    return errmsg;
  }
  else if (error->brlerrno==BRLERR_LIBCERR) {
    snprintf(errmsg,sizeof(errmsg),"%s: %s", error->errfun, strerror(error->exterrno.libcerrno));
    return errmsg;
  } else
    return brlapi_errlist[error->brlerrno];
}

/* brlapi_perror: error message printing */
void brlapi_perror(const char *s)
{
  fprintf(stderr,"%s: %s\n",s,brlapi_strerror(&brlapi_error));
}

/* XXX functions mustn't use brlapi_errno after this since it #undefs it XXX */

#ifdef brlapi_error
#undef brlapi_error
#endif /* brlapi_error */

brlapi_error_t brlapi_error;
static int pthread_error_ok;

/* we need a per-thread errno variable, thanks to pthread_keys */
static pthread_key_t error_key;

/* the key must be created at most once */
static pthread_once_t error_key_once = PTHREAD_ONCE_INIT;

#ifndef WINDOWS
/* We need to declare these with __attribute__((weak)) to determine at runtime 
 * whether libpthread is used or not. We can't rely on the functions prototypes,
 * hence the use of typeof
 */
#define WEAK_REDEFINE(name) extern typeof(name) name __attribute__((weak))
WEAK_REDEFINE(pthread_key_create);
WEAK_REDEFINE(pthread_once);
WEAK_REDEFINE(pthread_getspecific);
WEAK_REDEFINE(pthread_setspecific);
#endif /* WINDOWS */

static void error_key_free(void *key)
{
  free(key);
}

static void error_key_alloc(void)
{
  pthread_error_ok=!pthread_key_create(&error_key, error_key_free);
}

/* how to get per-thread errno variable. This will be called by the macro
 * brlapi_errno */
brlapi_error_t *brlapi_error_location(void)
{
  brlapi_error_t *errorp;
#ifndef WINDOWS
  if (pthread_once && pthread_key_create) {
#endif /* WINDOWS */
    pthread_once(&error_key_once, error_key_alloc);
    if (pthread_error_ok) {
      if ((errorp=(brlapi_error_t *) pthread_getspecific(error_key)))
        /* normal case */
        return errorp;
      else
        /* on the first time, must allocate it */
        if ((errorp=malloc(sizeof(*errorp))) && !pthread_setspecific(error_key,errorp))
          return errorp;
    }
#ifndef WINDOWS
  }
#endif /* WINDOWS */
  /* fall-back: shared errno :/ */
  return &brlapi_error;
}

brlapi_exceptionHandler_t brlapi_setExceptionHandler(brlapi_exceptionHandler_t new)
{
  brlapi_exceptionHandler_t tmp;
  pthread_mutex_lock(&brlapi_exceptionHandler_mutex);
  tmp = brlapi_exceptionHandler;
  if (new!=NULL) brlapi_exceptionHandler = new;
  pthread_mutex_unlock(&brlapi_exceptionHandler_mutex);
  return tmp;
}

int brlapi_strexception(char *buf, size_t n, int err, brl_type_t type, const void *packet, size_t size)
{
  int chars = 16; /* Number of bytes to dump */
  char hexString[3*chars+1];
  int i, nbChars = MIN(chars, size);
  char *p = hexString;
  brlapi_error_t error = { .brlerrno = err };
  for (i=0; i<nbChars; i++)
    p += sprintf(p, "%02x ", ((char *) packet)[i]);
  p--; /* Don't keep last space */
  *p = '\0';
  return snprintf(buf, n, "%s on %s request of size %d (%s)",
    brlapi_strerror(&error), brlapi_packetType(type), size, hexString);
}

void brlapi_defaultExceptionHandler(int err, brl_type_t type, const void *packet, size_t size)
{
  char str[0X100];
  brlapi_strexception(str,0X100, err, type, packet, size);
  fprintf(stderr, "BrlAPI exception: %s\n", str);
  abort();
}

/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2006 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
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

#include <stdio.h>
#include <stddef.h>
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

#ifdef __MINGW32__
#include "win_pthread.h"
#else /* __MINGW32__ */
#include <pthread.h>
#include <semaphore.h>
#endif /* __MINGW32__ */

#include "misc.h"

#define syslog(level,fmt,...) fprintf(stderr,#level ": " fmt, ## __VA_ARGS__)

#else /* WINDOWS */
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>
#include <semaphore.h>
#include <syslog.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */

#define setSocketErrno()
#endif /* WINDOWS */

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif /* HAVE_ALLOCA_H */

#ifdef linux
#include <linux/major.h>
#include <linux/tty.h>
#include <linux/vt.h>
#define MAXIMUM_VIRTUAL_CONSOLE MAX_NR_CONSOLES
#endif /* linux */

#ifdef __OpenBSD__
#define MAXIMUM_VIRTUAL_CONSOLE 16
#endif /* __OpenBSD__ */

#ifndef MAXIMUM_VIRTUAL_CONSOLE
#define MAXIMUM_VIRTUAL_CONSOLE 1
#endif /* MAXIMUM_VIRTUAL_CONSOLE */

#define BRLAPI_NO_DEPRECATED
#include "brlapi.h"
#include "brlapi_protocol.h"

#define BRLAPI(fun) brlapi_ ## fun
#include "brlapi_common.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b))? (a): (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b) (((a) > (b))? (a): (b))
#endif /* MAX */

/* API states */
#define STCONNECTED 1
#define STRAW 2
#define STSUSPEND 4
#define STCONTROLLINGTTY 8

#ifdef WINDOWS
static WSADATA wsadata;

static void* GetProc(const char *library, const char *fun) {
  HMODULE module;
  void *ret;
  if (!(module = LoadLibrary(library))) return (void*)(-1);
  if ((ret = GetProcAddress(module, fun))) return ret;
  FreeLibrary(module);
  return (void*)(-1);
}

#define CHECKPROC(library, name) \
  	(name##Proc && name##Proc != (void*)(-1))
#define CHECKGETPROC(library, name) \
  	(name##Proc != (void*)(-1) && (name##Proc || (name##Proc = GetProc(library,#name)) != (void*)(-1)))
#define WIN_PROC_STUB(name) typeof(name) (*name##Proc);


static WIN_PROC_STUB(GetConsoleWindow);

static WIN_PROC_STUB(wcslen);

static WIN_PROC_STUB(getaddrinfo);
#define getaddrinfo(host,port,hints,res) getaddrinfoProc(host,port,hints,res)
static WIN_PROC_STUB(freeaddrinfo);
#define freeaddrinfo(res) freeaddrinfoProc(res)
#endif /* WINDOWS */

/** key presses buffer size
 *
 * key presses won't be lost provided no more than BRL_KEYBUF_SIZE key presses
 * are done between two calls to brlapi_read* if a call to another function is
 * done in the meanwhile (which needs somewhere to put them before being able
 * to get responses from the server)
*/
#define BRL_KEYBUF_SIZE 256

struct brlapi_handle_t { /* Connection-specific information */
  unsigned int brlx;
  unsigned int brly;
  brlapi_fileDescriptor fileDescriptor; /* Descriptor of the socket connected to BrlApi */
  int addrfamily; /* Address family of the socket */
  /* to protect concurrent fd write operations */
  pthread_mutex_t fileDescriptor_mutex;
  /* to protect concurrent fd requests */
  pthread_mutex_t req_mutex;
  /* to protect concurrent key reading */
  pthread_mutex_t key_mutex;
  /* to protect concurrent fd key events and request answers, also protects the
   * key buffer */
  /* Only two threads might want to take it: one that already got
  * brlapi_req_mutex, or one that got key_mutex */
  pthread_mutex_t read_mutex;
  /* when someone is already reading (reading==1), put expected type,
   * address/size of buffer, and address of return value, then wait on semaphore,
   * the buffer gets filled, altRes too */
  int reading;
  brlapi_type_t altExpectedPacketType;
  unsigned char *altPacket;
  size_t altSize;
  ssize_t *altRes;
  sem_t *altSem;
  int state;
  pthread_mutex_t state_mutex;
  /* key presses buffer, for when key presses are received instead of
   * acknowledgements for instance
   *
   * every function must hence be able to read at least sizeof(brlapi_keyCode_t) */
  brlapi_keyCode_t keybuf[BRL_KEYBUF_SIZE];
  unsigned keybuf_next;
  unsigned keybuf_nb;
  union {
    brlapi_exceptionHandler_t exceptionHandler;
    brlapi__exceptionHandler_t exceptionHandlerWithHandle;
  };
  pthread_mutex_t exceptionHandler_mutex;
};

/* Function brlapi_getHandleSize */
size_t brlapi_getHandleSize(void)
{
  return sizeof(brlapi_handle_t);
}

static brlapi_handle_t defaultHandle;

/* brlapi_initializeHandle */
/* Initialize a BrlAPI handle */
static void brlapi_initializeHandle(brlapi_handle_t *handle)
{
  pthread_mutexattr_t mattr;
  pthread_mutexattr_init(&mattr);
  handle->brlx = 0;
  handle->brly = 0;
  handle->fileDescriptor = INVALID_FILE_DESCRIPTOR;
  handle->addrfamily = 0;
  pthread_mutex_init(&handle->fileDescriptor_mutex, &mattr);
  pthread_mutex_init(&handle->req_mutex, &mattr);
  pthread_mutex_init(&handle->key_mutex, &mattr);
  pthread_mutex_init(&handle->read_mutex, &mattr);
  handle->reading = 0;
  handle->altExpectedPacketType = 0;
  handle->altPacket = NULL;
  handle->altSize = 0;
  handle->altRes = NULL;
  handle->altSem = NULL;
  handle->state = 0;
  pthread_mutex_init(&handle->state_mutex, &mattr);
  memset(handle->keybuf, 0, sizeof(handle->keybuf));
  handle->keybuf_next = 0;
  handle->keybuf_nb = 0;
  handle->exceptionHandler = brlapi_defaultExceptionHandler;
  pthread_mutex_init(&handle->exceptionHandler_mutex, &mattr);
}

/* brlapi_doWaitForPacket */
/* Waits for the specified type of packet: must be called with brlapi_req_mutex locked */
/* If the right packet type arrives, returns its size */
/* Returns -1 if a non-fatal error is encountered */
/* Returns -2 on end of file */
/* Returns -3 if the available packet was not for us */
/* Calls the exception handler if an exception is encountered */
static ssize_t brlapi__doWaitForPacket(brlapi_handle_t *handle, brlapi_type_t expectedPacketType, void *packet, size_t size)
{
  static unsigned char localPacket[BRLAPI_MAXPACKETSIZE];
  brlapi_type_t type;
  ssize_t res;
  uint32_t *code = (uint32_t *) localPacket;
  brlapi_errorPacket_t *errorPacket = (brlapi_errorPacket_t *) localPacket;

  res = brlapi_readPacketHeader(handle->fileDescriptor, &type);
  if (res<0) return res; /* reports EINTR too */
  if (type==expectedPacketType)
    /* For us, just read */
    return brlapi_readPacketContent(handle->fileDescriptor, res, packet, size);

  /* Not for us. For alternate reader? */
  pthread_mutex_lock(&handle->read_mutex);
  if (handle->altSem && type==handle->altExpectedPacketType) {
    /* Yes, put packet content there */
    *handle->altRes = res = brlapi_readPacketContent(handle->fileDescriptor, res, handle->altPacket, handle->altSize);
    sem_post(handle->altSem);
    handle->altSem = NULL;
    pthread_mutex_unlock(&handle->read_mutex);
    if (res < 0) return res;
    return -3;
  }
  /* No alternate reader, read it locally... */
  if ((res = brlapi_readPacketContent(handle->fileDescriptor, res, localPacket, sizeof(localPacket))) < 0) {
    pthread_mutex_unlock(&handle->read_mutex);
    return res;
  }
  if ((type==BRLAPI_PACKET_KEY) && (handle->state & STCONTROLLINGTTY) && (res==sizeof(brlapi_keyCode_t))) {
    /* keypress, buffer it */
    if (handle->keybuf_nb>=BRL_KEYBUF_SIZE) {
      syslog(LOG_WARNING,"lost key: 0X%8"PRIx32"%8"PRIx32"\n",ntohl(code[0]),ntohl(code[1]));
    } else {
      handle->keybuf[(handle->keybuf_next+handle->keybuf_nb++)%BRL_KEYBUF_SIZE]=ntohl(*code);
    }
    pthread_mutex_unlock(&handle->read_mutex);
    return -3;
  }
  pthread_mutex_unlock(&handle->read_mutex);

  /* else this is an error */

  if (type==BRLAPI_PACKET_ERROR) {
    brlapi_errno = ntohl(errorPacket->code);
    return -1;
  }
  if (type==BRLAPI_PACKET_EXCEPTION) {
    size_t esize;
    int hdrSize = sizeof(errorPacket->code)+sizeof(errorPacket->type);
    if (res<hdrSize) esize = 0; else esize = res-hdrSize;
    if (handle==&defaultHandle) defaultHandle.exceptionHandler(ntohl(errorPacket->code), ntohl(errorPacket->type), &errorPacket->packet, esize);
    else handle->exceptionHandlerWithHandle(handle, ntohl(errorPacket->code), ntohl(errorPacket->type), &errorPacket->packet, esize);
    return -3;
  }
  syslog(LOG_ERR,"(brlapi_waitForPacket) Received unexpected packet of type %s and size %ld\n",brlapi_packetType(type),(long)res);
  return -3;
}

/* brlapi_waitForPacket */
/* same as brlapi_doWaitForPacket, but sleeps instead of reading if another
 * thread is already reading. Never returns -2. If loop is 1, never returns -3.
 */
static ssize_t brlapi__waitForPacket(brlapi_handle_t *handle, brlapi_type_t expectedPacketType, void *packet, size_t size, int loop) {
  int doread = 0;
  ssize_t res;
  sem_t sem;
again:
  pthread_mutex_lock(&handle->read_mutex);
  if (!handle->reading) doread = handle->reading = 1;
  else {
    if (handle->altSem) {
      syslog(LOG_ERR,"third call to brlapi_waitForPacket !");
      brlapi_errno = BRLAPI_ERROR_ILLEGAL_INSTRUCTION;
      return -1;
    }
    handle->altExpectedPacketType = expectedPacketType;
    handle->altPacket = packet;
    handle->altSize = size;
    handle->altRes = &res;
    sem_init(&sem, 0, 0);
    handle->altSem = &sem;
  }
  pthread_mutex_unlock(&handle->read_mutex);
  if (doread) {
    do {
      res = brlapi__doWaitForPacket(handle, expectedPacketType, packet, size);
    } while (loop && res == -1 && brlapi_errno == BRLAPI_ERROR_LIBCERR && (
	  brlapi_libcerrno == EINTR ||
#ifdef EWOULDBLOCK
	  brlapi_libcerrno == EWOULDBLOCK ||
#endif /* EWOULDBLOCK */
	  brlapi_libcerrno == EAGAIN));
    pthread_mutex_lock(&handle->read_mutex);
    if (handle->altSem) {
      *handle->altRes = -3; /* no packet for him */
      sem_post(handle->altSem);
      handle->altSem = NULL;
    }
    handle->reading = 0;
    pthread_mutex_unlock(&handle->read_mutex);
  } else {
    sem_wait(&sem);
    sem_destroy(&sem);
    if (res < 0 && (res != -3 || loop)) goto again;
      /* reader hadn't any packet for us, or error */
  }
  if (res==-2) {
    res = -1;
    brlapi_errno=BRLAPI_ERROR_EOF;
  }
  return res;
}

/* brlapi_waitForAck */
/* Wait for an acknowledgement, must be called with brlapi_req_mutex locked */
static int brlapi__waitForAck(brlapi_handle_t *handle)
{
  return brlapi__waitForPacket(handle, BRLAPI_PACKET_ACK, NULL, 0, 1);
}

/* brlapi_writePacketWaitForAck */
/* write a packet and wait for an acknowledgement */
static int brlapi__writePacketWaitForAck(brlapi_handle_t *handle, brlapi_type_t type, const void *buf, size_t size)
{
  ssize_t res;
  pthread_mutex_lock(&handle->req_mutex);
  if ((res=brlapi_writePacket(handle->fileDescriptor, type,buf,size))<0) {
    pthread_mutex_unlock(&handle->req_mutex);
    return res;
  }
  res=brlapi__waitForAck(handle);
  pthread_mutex_unlock(&handle->req_mutex);
  return res;
}

/* Function: tryHost */
/* Tries to connect to the given host. */
static int tryHost(brlapi_handle_t *handle, char *hostAndPort) {
  char *host = NULL;
  char *port;
  SocketDescriptor sockfd = -1;

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

  handle->addrfamily = brlapi_expandHost(hostAndPort,&host,&port);

#if defined(PF_LOCAL)
  if (handle->addrfamily == PF_LOCAL) {
    int lpath = strlen(BRLAPI_SOCKETPATH),lport;
    lport = strlen(port);
#ifdef WINDOWS
    {
      char path[lpath+lport+1];
      memcpy(path,BRLAPI_SOCKETPATH,lpath);
      memcpy(path+lpath,port,lport+1);
      while ((handle->fileDescriptor = CreateFile(path,GENERIC_READ|GENERIC_WRITE,
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
	brlapi_errno = BRLAPI_ERROR_LIBCERR;
	goto out;
      }
      if ((handle->fileDescriptor = socket(PF_LOCAL, SOCK_STREAM, 0))<0) {
        brlapi_errfun="socket";
        setSocketErrno();
        goto outlibc;
      }
      sa.sun_family = AF_LOCAL;
      memcpy(sa.sun_path,BRLAPI_SOCKETPATH,lpath);
      memcpy(sa.sun_path+lpath,port,lport+1);
      if (connect(handle->fileDescriptor, (struct sockaddr *) &sa, sizeof(sa))<0) {
        brlapi_errfun="connect";
        setSocketErrno();
        goto outlibc;
      }
    }
#endif /* WINDOWS */
  } else {
#else /* PF_LOCAL */
  if (0) {} else {
#endif /* PF_LOCAL */

#ifdef WINDOWS
    if (CHECKGETPROC("ws2_32.dll",getaddrinfo)
	&& CHECKGETPROC("ws2_32.dll",freeaddrinfo)) {
#endif /* WINDOWS */
#if defined(HAVE_GETADDRINFO) || defined(WINDOWS)

    struct addrinfo *res,*cur;
    struct addrinfo hints;
    SocketDescriptor sockfd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    brlapi_gaierrno = getaddrinfo(host, port, &hints, &res);
    if (brlapi_gaierrno) {
      brlapi_errno=BRLAPI_ERROR_GAIERR;
      brlapi_libcerrno=errno;
      goto out;
    }
    for(cur = res; cur; cur = cur->ai_next) {
      sockfd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
      if (sockfd<0) continue;
      if (connect(sockfd, cur->ai_addr, cur->ai_addrlen)<0) {
        closeSocketDescriptor(sockfd);
        continue;
      }
      break;
    }
    freeaddrinfo(res);
    if (!cur) {
      brlapi_errno=BRLAPI_ERROR_CONNREFUSED;
      goto out;
    }

#endif /* HAVE_GETADDRINFO */
#ifdef WINDOWS
    } else {
#endif /* WINDOWS */
#if !defined(HAVE_GETADDRINFO) || defined(WINDOWS)

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
          brlapi_errno=BRLAPI_ERROR_GAIERR;
	  goto out;
	}
	addr.sin_port = se->s_port;
      }
    }

    if (!host) {
      addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    } else if ((addr.sin_addr.s_addr = inet_addr(host)) == htonl(INADDR_NONE)) {
      if (!(he = gethostbyname(host))) {
	brlapi_gaierrno = h_errno;
	brlapi_errno = BRLAPI_ERROR_GAIERR;
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
    
    sockfd = socket(addr.sin_family, SOCK_STREAM, 0);
    if (sockfd<0) {
      brlapi_errfun = "socket";
      setSocketErrno();
      goto outlibc;
    }
    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr))<0) {
      brlapi_errfun = "connect";
      setSocketErrno();
      goto outlibc;
    }

#endif /* !HAVE_GETADDRINFO */
#ifdef WINDOWS
    }
#endif /* WINDOWS */

    handle->fileDescriptor = (FileDescriptor) sockfd;
  }
  free(host);
  free(port);
  return 0;

outlibc:
  brlapi_errno = BRLAPI_ERROR_LIBCERR;
  brlapi_libcerrno = errno;
  if (sockfd>=0)
    closeSocketDescriptor(sockfd);
out:
  free(host);
  free(port);
  return -1;
}

/* Function : updateSettings */
/* Updates the content of a brlapi_connectionSettings_t structure according to */
/* another structure of the same type */
static void updateSettings(brlapi_connectionSettings_t *s1, const brlapi_connectionSettings_t *s2)
{
  if (s2==NULL) return;
  if ((s2->auth) && (*s2->auth))
    s1->auth = s2->auth;
  if ((s2->host) && (*s2->host))
    s1->host = s2->host;
}

/* Function: brlapi_openConnection
 * Creates a socket to connect to BrlApi */
brlapi_fileDescriptor brlapi__openConnection(brlapi_handle_t *handle, const brlapi_connectionSettings_t *clientSettings, brlapi_connectionSettings_t *usedSettings)
{
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  unsigned char serverPacket[BRLAPI_MAXPACKETSIZE];
  brlapi_authClientStruct_t *auth = (brlapi_authClientStruct_t *) packet;
  brlapi_authServerStruct_t *authServer = (brlapi_authServerStruct_t *) serverPacket;
  uint32_t *type;
  int authServerLen;

  brlapi_connectionSettings_t settings = { BRLAPI_DEFAUTH, ":0" };
  brlapi_connectionSettings_t envsettings = { getenv("BRLAPI_AUTH"), getenv("BRLAPI_HOST") };

  /* Here update settings with the parameters from misc sources (files, env...) */
  updateSettings(&settings, &envsettings);
  updateSettings(&settings, clientSettings);
  if (usedSettings!=NULL) updateSettings(usedSettings, &settings);
  brlapi_initializeHandle(handle);

  if (tryHost(handle, settings.host)<0) {
    brlapi_error_t error = brlapi_error;
    if (tryHost(handle, settings.host="127.0.0.1:0")<0
#ifdef AF_INET6
      && tryHost(handle, settings.host="::1:0")<0
#endif /* AF_INET6 */
      ) {
      brlapi_error = error;
      goto out;
    }
    if (usedSettings) usedSettings->host = settings.host;
  }

  if ((authServerLen = brlapi__waitForPacket(handle, BRLAPI_PACKET_AUTH, serverPacket, sizeof(serverPacket), 1)) < 0)
    return INVALID_FILE_DESCRIPTOR;

  if (authServer->protocolVersion != htonl(BRLAPI_PROTOCOL_VERSION)) {
    brlapi_errno = BRLAPI_ERROR_PROTOCOL_VERSION;
    return INVALID_FILE_DESCRIPTOR;
  }

  auth->protocolVersion = htonl(BRLAPI_PROTOCOL_VERSION);

  for (type = &authServer->type[0]; (void*) type < (void*) &serverPacket[authServerLen]; type++) {
    auth->type = *type;
    switch (ntohl(*type)) {
      case BRLAPI_AUTH_NONE:
        if (brlapi_writePacket(handle->fileDescriptor, BRLAPI_PACKET_AUTH, auth,
	    sizeof(auth->protocolVersion)+sizeof(auth->type)) < 0)
	  goto outfd;
	if (usedSettings) usedSettings->auth = "none";
	break;
      case BRLAPI_AUTH_KEY: {
        size_t authKeyLength;
	int res;
        if (brlapi_loadAuthKey(settings.auth, &authKeyLength, (void *) &auth->key) < 0)
	  continue;
        res = brlapi_writePacket(handle->fileDescriptor, BRLAPI_PACKET_AUTH, auth,
	  sizeof(auth->protocolVersion)+sizeof(auth->type)+authKeyLength);
	memset(&auth->key, 0, authKeyLength);
	if (res < 0)
	  goto outfd;
	if (usedSettings) usedSettings->auth = settings.auth;
	break;
      }
      default:
        /* unsupported authorization type */
	continue;
    }
    if ((brlapi__waitForAck(handle)) == 0) goto done;
  }

  /* no suitable authorization method */
  brlapi_errno = BRLAPI_ERROR_CONNREFUSED;
outfd:
  closeFileDescriptor(handle->fileDescriptor);
  handle->fileDescriptor = INVALID_FILE_DESCRIPTOR;
out:
  return INVALID_FILE_DESCRIPTOR;

done:
  pthread_mutex_lock(&handle->state_mutex);
  handle->state = STCONNECTED;
  pthread_mutex_unlock(&handle->state_mutex);
  return handle->fileDescriptor;
}

brlapi_fileDescriptor brlapi_openConnection(const brlapi_connectionSettings_t *clientSettings, brlapi_connectionSettings_t *usedSettings)
{
  return brlapi__openConnection(&defaultHandle, clientSettings, usedSettings);
}

/* brlapi_closeConnection */
/* Cleanly close the socket */
void brlapi__closeConnection(brlapi_handle_t *handle)
{
  pthread_mutex_lock(&handle->state_mutex);
  handle->state = 0;
  pthread_mutex_unlock(&handle->state_mutex);
  pthread_mutex_lock(&handle->fileDescriptor_mutex);
  closeFileDescriptor(handle->fileDescriptor);
  handle->fileDescriptor = INVALID_FILE_DESCRIPTOR;
  pthread_mutex_unlock(&handle->fileDescriptor_mutex);
#ifdef WINDOWS
  WSACleanup();
#endif /* WINDOWS */
}

void brlapi_closeConnection(void)
{
  brlapi__closeConnection(&defaultHandle);
}

/* brlapi_getDriverSpecific */
/* Switch to device specific mode */
static int brlapi__getDriverSpecific(brlapi_handle_t *handle, const char *driver, brlapi_type_t type, int st)
{
  int res;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  brlapi_getDriverSpecificModePacket_t *driverPacket = (brlapi_getDriverSpecificModePacket_t *) packet;
  unsigned int n = strlen(driver);
  if (n>BRLAPI_MAXNAMELENGTH) {
    brlapi_errno = BRLAPI_ERROR_INVALID_PARAMETER;
    return -1;
  }
  pthread_mutex_lock(&defaultHandle.state_mutex);
  if ((defaultHandle.state & st)) {
    brlapi_errno = BRLAPI_ERROR_ILLEGAL_INSTRUCTION;
    res = -1;
    goto out;
  }
  driverPacket->magic = htonl(BRLAPI_DEVICE_MAGIC);
  driverPacket->nameLength = n;
  memcpy(&driverPacket->name, driver, n);
  res = brlapi__writePacketWaitForAck(handle, type, packet, sizeof(uint32_t)+1+n);
  if (res!=-1)
    handle->state |= st;
out:
  pthread_mutex_unlock(&handle->state_mutex);
  return res;
}

/* brlapi_leaveDriverSpecific */
/* Leave device specific mode */
static int brlapi__leaveDriverSpecific(brlapi_handle_t *handle, brlapi_type_t type, int st)
{
  int res;
  pthread_mutex_lock(&handle->state_mutex);
  if (!(handle->state & st)) {
    brlapi_errno = BRLAPI_ERROR_ILLEGAL_INSTRUCTION;
    res = -1;
    goto out;
  }
  res = brlapi__writePacketWaitForAck(handle, type, NULL, 0);
  if (!res)
    handle->state &= ~st;
out:
  pthread_mutex_unlock(&handle->state_mutex);
  return res;
}

/* brlapi_enterRawMode */
/* Switch to Raw mode */
int brlapi__enterRawMode(brlapi_handle_t *handle, const char *driver)
{
  return brlapi__getDriverSpecific(handle, driver, BRLAPI_PACKET_ENTERRAWMODE, STRAW);
}

int brlapi_enterRawMode(const char *driver)
{
  return brlapi__enterRawMode(&defaultHandle, driver);
}

/* brlapi_leaveRawMode */
/* Leave Raw mode */
int brlapi__leaveRawMode(brlapi_handle_t *handle)
{
  return brlapi__leaveDriverSpecific(handle, BRLAPI_PACKET_LEAVERAWMODE, STRAW);
}

int brlapi_leaveRawMode(void)
{
  return brlapi__leaveRawMode(&defaultHandle);
}

/* brlapi_sendRaw */
/* Send a Raw Packet */
ssize_t brlapi__sendRaw(brlapi_handle_t *handle, const void *buf, size_t size)
{
  ssize_t res;
  pthread_mutex_lock(&handle->fileDescriptor_mutex);
  res=brlapi_writePacket(handle->fileDescriptor, BRLAPI_PACKET_PACKET, buf, size);
  pthread_mutex_unlock(&handle->fileDescriptor_mutex);
  return res;
}

ssize_t brlapi_sendRaw(const void *buf, size_t size)
{
  return brlapi__sendRaw(&defaultHandle, buf, size);
}

/* brlapi_recvRaw */
/* Get a Raw packet */
ssize_t brlapi__recvRaw(brlapi_handle_t *handle, void *buf, size_t size)
{
  ssize_t res;
  if (!(handle->state & STRAW)) {
    brlapi_errno = BRLAPI_ERROR_ILLEGAL_INSTRUCTION;
    return -1;
  }
  res = brlapi__waitForPacket(handle, BRLAPI_PACKET_PACKET, buf, size, 0);
  if (res == -3) {
    brlapi_libcerrno = EINTR;
    brlapi_errno = BRLAPI_ERROR_LIBCERR;
    brlapi_errfun = "waitForPacket";
    res = -1;
  }
  return res;
}

ssize_t brlapi_recvRaw(void *buf, size_t size)
{
  return brlapi__recvRaw(&defaultHandle, buf, size);
}

/* brlapi_suspendDriver */
int brlapi__suspendDriver(brlapi_handle_t *handle, const char *driver)
{
  return brlapi__getDriverSpecific(handle, driver, BRLAPI_PACKET_SUSPENDDRIVER, STSUSPEND);
}

int brlapi_suspendDriver(const char *driver)
{
  return brlapi__suspendDriver(&defaultHandle, driver);
}

/* brlapi_resumeDriver */
int brlapi__resumeDriver(brlapi_handle_t *handle)
{
  return brlapi__leaveDriverSpecific(handle, BRLAPI_PACKET_RESUMEDRIVER, STSUSPEND);
}

int brlapi_resumeDriver(void)
{
  return brlapi__resumeDriver(&defaultHandle);
}

/* Function brlapi_request */
/* Sends a request to the API and waits for the answer */
/* The answer is put in the given packet */
static ssize_t brlapi__request(brlapi_handle_t *handle, brlapi_type_t request, void *packet, size_t size)
{
  ssize_t res;
  pthread_mutex_lock(&handle->req_mutex);
  res = brlapi_writePacket(handle->fileDescriptor, request, NULL, 0);
  if (res==-1) {
    pthread_mutex_unlock(&handle->req_mutex);
    return -1;
  }
  res = brlapi__waitForPacket(handle, request, packet, size, 1);
  pthread_mutex_unlock(&handle->req_mutex);
  return res;
}

/* Function : brlapi_getDriverId */
/* Identify the driver used by brltty */
int brlapi__getDriverId(brlapi_handle_t *handle, char *id, size_t n)
{
  ssize_t res = brlapi__request(handle, BRLAPI_PACKET_GETDRIVERID, id, n);
  if ((res>0) && (res<=n)) id[res-1] = '\0';
  return res; 
}

int brlapi_getDriverId(char *id, size_t n)
{
  return brlapi__getDriverId(&defaultHandle, id, n);
}

/* Function : brlapi_getDriverName */
/* Name of the driver used by brltty */
int brlapi__getDriverName(brlapi_handle_t *handle, char *name, size_t n)
{
  ssize_t res = brlapi__request(handle, BRLAPI_PACKET_GETDRIVERNAME, name, n);
  if ((res>0) && (res<=n)) name[res-1] = '\0';
  return res;
}

int brlapi_getDriverName(char *name, size_t n)
{
  return brlapi__getDriverName(&defaultHandle, name, n);
}

/* Function : brlapi_getDisplaySize */
/* Returns the size of the braille display */
int brlapi__getDisplaySize(brlapi_handle_t *handle, unsigned int *x, unsigned int *y)
{
  uint32_t displaySize[2];
  ssize_t res;

  if (handle->brlx*handle->brly) { *x = handle->brlx; *y = handle->brly; return 0; }
  res = brlapi__request(handle, BRLAPI_PACKET_GETDISPLAYSIZE, displaySize, sizeof(displaySize));
  if (res==-1) { return -1; }
  handle->brlx = ntohl(displaySize[0]);
  handle->brly = ntohl(displaySize[1]);
  *x = handle->brlx; *y = handle->brly;
  return 0;
}

int brlapi_getDisplaySize(unsigned int *x, unsigned int *y)
{
  return brlapi__getDisplaySize(&defaultHandle, x, y);
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
  if (CHECKGETPROC("kernel32.dll", GetConsoleWindow))
    /* really good guess */
    if ((tty = (int) GetConsoleWindowProc())) return tty;
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

/* Function : brlapi_enterTtyMode */
/* Takes control of a tty */
int brlapi__enterTtyMode(brlapi_handle_t *handle, int tty, const char *driverName)
{
  /* Determine which tty to take control of */
  if (tty<0) tty = getControllingTty();
  /* 0 can be a valid screen WINDOW
  0xffffffff can not be a valid WINDOWID (top 3 bits guaranteed to be zero) */
  if (tty<0) { brlapi_errno=BRLAPI_ERROR_UNKNOWNTTY; return -1; }
  
  if (brlapi__enterTtyModeWithPath(handle, &tty, 1, driverName)) return -1;

  return tty;
}

int brlapi_enterTtyMode(int tty, const char *how)
{
  return brlapi__enterTtyMode(&defaultHandle, tty, how);
}

/* Function : brlapi_enterTtyModeWithPath */
/* Takes control of a tty path */
int brlapi__enterTtyModeWithPath(brlapi_handle_t *handle, int *ttys, int nttys, const char *driverName)
{
  int res;
  unsigned char packet[BRLAPI_MAXPACKETSIZE], *p;
  uint32_t *nbTtys = (uint32_t *) packet, *t = nbTtys+1;
  char *ttytreepath,*ttytreepathstop;
  int ttypath;
  unsigned int n;

  pthread_mutex_lock(&handle->state_mutex);
  if ((handle->state & STCONTROLLINGTTY)) {
    pthread_mutex_unlock(&handle->state_mutex);
    brlapi_errno = BRLAPI_ERROR_ILLEGAL_INSTRUCTION;
    return -1;
  }

  if (brlapi__getDisplaySize(handle, &handle->brlx, &handle->brly)<0) return -1;
  
  /* Clear key buffer before taking the tty, just in case... */
  pthread_mutex_lock(&handle->read_mutex);
  handle->keybuf_next = handle->keybuf_nb = 0;
  pthread_mutex_unlock(&handle->read_mutex);

  /* OK, Now we know where we are, so get the effective control of the terminal! */
  *nbTtys = 0;
  ttytreepath = getenv("WINDOWPATH");
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
  if (driverName==NULL) n = 0; else n = strlen(driverName);
  if (n>BRLAPI_MAXNAMELENGTH) {
    brlapi_errno = BRLAPI_ERROR_INVALID_PARAMETER;
    return -1;
  }
  *p = n;
  p++;
  memcpy(p, driverName, n);
  p += n;
  if ((res=brlapi__writePacketWaitForAck(handle,BRLAPI_PACKET_ENTERTTYMODE,packet,(p-packet))) == 0)
    handle->state |= STCONTROLLINGTTY;
  pthread_mutex_unlock(&handle->state_mutex);
  return res;
}

int brlapi_enterTtyModeWithPath(int *ttys, int nttys, const char *how)
{
  return brlapi__enterTtyModeWithPath(&defaultHandle, ttys, nttys, how);
}

/* Function : brlapi_leaveTtyMode */
/* Gives back control of our tty to brltty */
int brlapi__leaveTtyMode(brlapi_handle_t *handle)
{
  int res;
  pthread_mutex_lock(&handle->state_mutex);
  if (!(handle->state & STCONTROLLINGTTY)) {
    brlapi_errno = BRLAPI_ERROR_ILLEGAL_INSTRUCTION;
    res = -1;
    goto out;
  }
  handle->brlx = 0; handle->brly = 0;
  res = brlapi__writePacketWaitForAck(handle,BRLAPI_PACKET_LEAVETTYMODE,NULL,0);
  handle->state &= ~STCONTROLLINGTTY;
out:
  pthread_mutex_unlock(&handle->state_mutex);
  return res;
}

int brlapi_leaveTtyMode(void)
{
  return brlapi__leaveTtyMode(&defaultHandle);
}

/* Function : brlapi_setFocus */
/* sends the current focus to brltty */
int brlapi__setFocus(brlapi_handle_t *handle, int tty)
{
  uint32_t utty;
  int res;
  utty = htonl(tty);
  pthread_mutex_lock(&handle->fileDescriptor_mutex);
  res = brlapi_writePacket(handle->fileDescriptor, BRLAPI_PACKET_SETFOCUS, &utty, sizeof(utty));
  pthread_mutex_unlock(&handle->fileDescriptor_mutex);
  return res;
}

int brlapi_setFocus(int tty)
{
  return brlapi__setFocus(&defaultHandle, tty);
}

#ifdef WINDOWS
static size_t getCharset(unsigned char *buffer, int wide)
#else /* WINDOWS */
static size_t getCharset(unsigned char *buffer)
#endif /* WINDOWS */
{
  unsigned char *p = buffer;
  char *locale;
  unsigned int len;
  locale = setlocale(LC_CTYPE,NULL);
#ifdef WINDOWS
#ifdef WORDS_BIGENDIAN
#define WIN_WCHAR_T "UCS-2BE"
#else /* WORDS_BIGENDIAN */
#define WIN_WCHAR_T "UCS-2LE"
#endif /* WORDS_BIGENDIAN */
  if (CHECKPROC("ntdll.dll", wcslen) && wide) {
    *p++ = strlen(WIN_WCHAR_T);
    strcpy(p, WIN_WCHAR_T);
    p += strlen(WIN_WCHAR_T);
  } else
#endif /* WINDOWS */
  if (locale && strcmp(locale,"C")) {
    /* not default locale, tell charset to server */
#ifdef WINDOWS
    UINT CP;
    if ((CP = GetACP() || (CP = GetOEMCP()))) {
      len = sprintf(p+3, "%d", CP);
      *p++ = 2 + len;
      *p++ = 'C';
      *p++ = 'P';
      p += len;
    }
#else /* WINDOWS */
    char *lang = nl_langinfo(CODESET);
    len = strlen(lang);
    *p++ = len;
    memcpy(p, lang, len);
    p += len;
#endif /* WINDOWS */
  }
  return p-buffer;
}

/* Function : brlapi_writeText */
/* Writes a string to the braille display */
#ifdef WINDOWS
int brlapi__writeTextWin(brlapi_handle_t *handle, int cursor, const void *str, int wide)
#else /* WINDOWS */
int brlapi__writeText(brlapi_handle_t *handle, int cursor, const char *str)
#endif /* WINDOWS */
{
  int dispSize = handle->brlx * handle->brly;
  unsigned int min;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  brlapi_writeStructPacket_t *ws = (brlapi_writeStructPacket_t *) packet;
  unsigned char *p = &ws->data;
  char *locale;
  int res;
  size_t len;
  if ((dispSize == 0) || (dispSize > BRLAPI_MAXPACKETSIZE/4)) {
    brlapi_errno=BRLAPI_ERROR_INVALID_PARAMETER;
    return -1;
  }
  locale = setlocale(LC_CTYPE,NULL);
  ws->flags = BRLAPI_WF_REGION;
  *((uint32_t *) p) = htonl(1); p += sizeof(uint32_t);
  *((uint32_t *) p) = htonl(dispSize); p += sizeof(uint32_t);
  if (str) {
    uint32_t *size;
    ws->flags |= BRLAPI_WF_TEXT;
    size = (uint32_t *) p;
    p += sizeof(*size);
#ifdef WINDOWS
    if (CHECKGETPROC("ntdll.dll", wcslen) && wide)
      len = sizeof(wchar_t) * wcslenProc(str);
    else
#endif /* WINDOWS */
      len = strlen(str);
    if (locale && strcmp(locale,"C")) {
      mbstate_t ps;
      size_t eaten;
      unsigned i;
      memset(&ps,0,sizeof(ps));
      for (min=0;min<dispSize;min++) {
	eaten = mbrlen(str,len,&ps);
	switch(eaten) {
	  case (size_t)(-2):
	    errno = EILSEQ;
	  case (size_t)(-1):
	    brlapi_libcerrno = errno;
	    brlapi_errfun = "mbrlen";
	    brlapi_errno = BRLAPI_ERROR_LIBCERR;
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
    } else
#ifdef WINDOWS
    if (wide) {
      int extra;
      min = MIN(len, sizeof(wchar_t) * dispSize);
      extra = dispSize - min / sizeof(wchar_t);
      memcpy(p, str, min);
      p += min;
      wmemset((wchar_t *) p, L' ', extra);
      p += sizeof(wchar_t) * extra;
    } else
#endif /* WINDOWS */
    {
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
    brlapi_errno = BRLAPI_ERROR_INVALID_PARAMETER;
    return -1;
  }

  if ((len = getCharset(p
#ifdef WINDOWS
  	, wide
#endif /* WINDOWS */
	))) {
    ws->flags |= BRLAPI_WF_CHARSET;
    p += len;
  }

  ws->flags = htonl(ws->flags);
  pthread_mutex_lock(&handle->fileDescriptor_mutex);
  res = brlapi_writePacket(handle->fileDescriptor,BRLAPI_PACKET_WRITE,packet,sizeof(ws->flags)+(p-&ws->data));
  pthread_mutex_unlock(&handle->fileDescriptor_mutex);
  return res;
}

#ifdef WINDOWS
int brlapi_writeTextWin(int cursor, const void *str, int wide)
{
  return brlapi__writeTextWin(&defaultHandle, cursor, str, wide);
}
#else /* WINDOWS */
int brlapi_writeText(int cursor, const char *str)
{
  return brlapi__writeText(&defaultHandle, cursor, str);
}
#endif /* WINDOWS */

/* Function : brlapi_writeDots */
/* Writes dot-matrix to the braille display */
int brlapi__writeDots(brlapi_handle_t *handle, const unsigned char *dots)
{
  int res;
  unsigned int size = handle->brlx * handle->brly;
  brlapi_writeStruct_t ws = BRLAPI_WRITESTRUCT_INITIALIZER;
  if (size == 0) {
    brlapi_errno=BRLAPI_ERROR_INVALID_PARAMETER;
    return -1;
  }
  {
    char text[size+1];
    unsigned char attrAnd[size], attrOr[size];
    memset(text, ' ', size);
    text[size] = 0;
    ws.regionBegin = 1;
    ws.regionSize = size;
    ws.text = text;
    memcpy(attrOr, dots, size);
    ws.attrOr = attrOr;
    memset(attrAnd, 0, size);
    ws.attrAnd = attrAnd;
    ws.cursor = 0;
    res = brlapi__write(handle,&ws);
  }
  return res;
}

int brlapi_writeDots(const unsigned char *dots)
{
  return brlapi__writeDots(&defaultHandle, dots);
}

/* Function : brlapi_write */
/* Extended writes on braille displays */
#ifdef WINDOWS
int brlapi__writeWin(brlapi_handle_t *handle, const brlapi_writeStruct_t *s, int wide)
#else /* WINDOWS */
int brlapi__write(brlapi_handle_t *handle, const brlapi_writeStruct_t *s)
#endif /* WINDOWS */
{
  int dispSize = handle->brlx * handle->brly;
  unsigned int rbeg, rsiz, strLen;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  brlapi_writeStructPacket_t *ws = (brlapi_writeStructPacket_t *) packet;
  unsigned char *p = &ws->data;
  unsigned char *end = &packet[BRLAPI_MAXPACKETSIZE];
  int res;
  ws->flags = 0;
  if (s==NULL) goto send;
  rbeg = s->regionBegin;
  rsiz = s->regionSize;
  if (rbeg || rsiz) {
    if (rsiz == 0) return 0;
    ws->flags |= BRLAPI_WF_REGION;
    *((uint32_t *) p) = htonl(rbeg); p += sizeof(uint32_t);
    *((uint32_t *) p) = htonl(rsiz); p += sizeof(uint32_t);
  } else {
    /* DEPRECATED */
    rbeg = 1; rsiz = dispSize;
  }
  if (s->text) {
    if (s->textSize != -1)
      strLen = s->textSize;
    else
#ifdef WINDOWS
      if (CHECKGETPROC("ntdll.dll", wcslen) && wide)
	strLen = sizeof(wchar_t) * wcslenProc((wchar_t *) s->text);
      else
#endif /* WINDOWS */
	strLen = strlen(s->text);
    *((uint32_t *) p) = htonl(strLen); p += sizeof(uint32_t);
    ws->flags |= BRLAPI_WF_TEXT;
    if (p + strLen > end) {
      brlapi_errno = BRLAPI_ERROR_INVALID_PARAMETER;
      return -1;
    }
    memcpy(p, s->text, strLen);
    p += strLen;
  }
  if (s->attrAnd) {
    ws->flags |= BRLAPI_WF_ATTR_AND;
    if (p + rsiz > end) {
      brlapi_errno = BRLAPI_ERROR_INVALID_PARAMETER;
      return -1;
    }
    memcpy(p, s->attrAnd, rsiz);
    p += rsiz;
  }
  if (s->attrOr) {
    ws->flags |= BRLAPI_WF_ATTR_OR;
    if (p + rsiz > end) {
      brlapi_errno = BRLAPI_ERROR_INVALID_PARAMETER;
      return -1;
    }
    memcpy(p, s->attrOr, rsiz);
    p += rsiz;
  }
  if ((s->cursor>=0) && (s->cursor<=dispSize)) {
    ws->flags |= BRLAPI_WF_CURSOR;
    if (p + sizeof(uint32_t) > end) {
      brlapi_errno = BRLAPI_ERROR_INVALID_PARAMETER;
      return -1;
    }
    *((uint32_t *) p) = htonl(s->cursor);
    p += sizeof(uint32_t);
  } else if (s->cursor!=-1) {
    brlapi_errno = BRLAPI_ERROR_INVALID_PARAMETER;
    return -1;    
  }
  if (s->charset) {
    if (!*s->charset) {
      if ((strLen = getCharset(p
    #ifdef WINDOWS
	    , wide
    #endif /* WINDOWS */
	    ))) {
	ws->flags |= BRLAPI_WF_CHARSET;
	p += strLen;
      }
    } else {
      strLen = strlen(s->charset);
      *p++ = strLen;
      ws->flags |= BRLAPI_WF_CHARSET;
      if (p + strLen > end) {
	brlapi_errno = BRLAPI_ERROR_INVALID_PARAMETER;
	return -1;
      }
      memcpy(p, s->charset, strLen);
      p += strLen;
    }
  }
send:
  ws->flags = htonl(ws->flags);
  pthread_mutex_lock(&handle->fileDescriptor_mutex);
  res = brlapi_writePacket(handle->fileDescriptor,BRLAPI_PACKET_WRITE,packet,sizeof(ws->flags)+(p-&ws->data));
  pthread_mutex_unlock(&handle->fileDescriptor_mutex);
  return res;
}

#ifdef WINDOWS
int brlapi_writeWin(const brlapi_writeStruct_t *s, int wide)
{
  return brlapi__writeWin(&defaultHandle, s, wide);
}
#else /* WINDOWS */
int brlapi_write(const brlapi_writeStruct_t *s)
{
  return brlapi__write(&defaultHandle, s);
}
#endif /* WINDOWS */

/* Function : packetReady */
/* Tests wether a packet is ready on file descriptor fd */
/* Returns -1 if an error occurs, 0 if no packet is ready, 1 if there is a */
/* packet ready to be read */
static int packetReady(brlapi_handle_t *handle)
{
#ifdef WINDOWS
  if (handle->addrfamily == PF_LOCAL) {
    DWORD avail;
    if (!PeekNamedPipe(handle->fileDescriptor, NULL, 0, NULL, &avail, NULL)) {
      brlapi_errfun = "packetReady";
      brlapi_errno = BRLAPI_ERROR_LIBCERR;
      brlapi_libcerrno = errno;
      return -1;
    }
    return avail!=0;
  } else {
    SOCKET fd = (SOCKET) handle->fileDescriptor;
#else /* WINDOWS */
  int fd = handle->fileDescriptor;
#endif /* WINDOWS */
  fd_set set;
  struct timeval timeout;
  memset(&timeout, 0, sizeof(timeout));
  FD_ZERO(&set);
  FD_SET(fd, &set);
  return select(fd+1, &set, NULL, NULL, &timeout);
#ifdef WINDOWS
  }
#endif /* WINDOWS */
}

/* Function : brlapi_readKey */
/* Reads a key from the braille keyboard */
int brlapi__readKey(brlapi_handle_t *handle, int block, brlapi_keyCode_t *code)
{
  ssize_t res;
  uint32_t buf[2];

  pthread_mutex_lock(&handle->state_mutex);
  if (!(handle->state & STCONTROLLINGTTY)) {
    pthread_mutex_unlock(&handle->state_mutex);
    brlapi_errno = BRLAPI_ERROR_ILLEGAL_INSTRUCTION;
    return -1;
  }
  pthread_mutex_unlock(&handle->state_mutex);

  pthread_mutex_lock(&handle->read_mutex);
  if (handle->keybuf_nb>0) {
    *code=handle->keybuf[handle->keybuf_next];
    handle->keybuf_next=(handle->keybuf_next+1)%BRL_KEYBUF_SIZE;
    handle->keybuf_nb--;
    pthread_mutex_unlock(&handle->read_mutex);
    return 1;
  }
  pthread_mutex_unlock(&handle->read_mutex);

  pthread_mutex_lock(&handle->key_mutex);
  if (!block) {
    res = packetReady(handle);
    if (res<=0) {
      if (res<0)
	brlapi_errno = BRLAPI_ERROR_LIBCERR;
      pthread_mutex_unlock(&handle->key_mutex);
      return res;
    }
  }
  res=brlapi__waitForPacket(handle,BRLAPI_PACKET_KEY, buf, sizeof(buf), 0);
  pthread_mutex_unlock(&handle->key_mutex);
  if (res == -3) {
    if (!block) return 0;
    brlapi_libcerrno = block?EINTR:EAGAIN;
    brlapi_errno = BRLAPI_ERROR_LIBCERR;
    brlapi_errfun = "waitForPacket";
    return -1;
  }
  if (res < 0) return -1;
  *code = ((brlapi_keyCode_t)ntohl(buf[0]) << 32) | ntohl(buf[1]);
  return 1;
}

int brlapi_readKey(int block, brlapi_keyCode_t *code)
{
  return brlapi__readKey(&defaultHandle, block, code) ;
}

/* Function : ignore_accept_key_range */
/* Common tasks for ignoring and unignoring key ranges */
/* what = 0 for ignoring !0 for unignoring */
static int ignore_accept_key_range(brlapi_handle_t *handle, int what, brlapi_keyCode_t x, brlapi_keyCode_t y)
{
  uint32_t ints[4] = {
    htonl(x >> 32), htonl(x & 0xffffffff),
    htonl(y >> 32), htonl(y & 0xffffffff),
  };

  return brlapi__writePacketWaitForAck(handle,(what ? BRLAPI_PACKET_ACCEPTKEYRANGE : BRLAPI_PACKET_IGNOREKEYRANGE),ints,sizeof(ints));
}

/* Function : brlapi_ignoreKeyRange */
int brlapi__ignoreKeyRange(brlapi_handle_t *handle, brlapi_keyCode_t x, brlapi_keyCode_t y)
{
  return ignore_accept_key_range(handle, 0,x,y);
}

int brlapi_ignoreKeyRange(brlapi_keyCode_t x, brlapi_keyCode_t y)
{
  return brlapi__ignoreKeyRange(&defaultHandle, x, y);
}

/* Function : brlapi_acceptKeyRange */
int brlapi__acceptKeyRange(brlapi_handle_t *handle, brlapi_keyCode_t x, brlapi_keyCode_t y)
{
  return ignore_accept_key_range(handle, !0,x,y);
}

int brlapi_acceptKeyRange(brlapi_keyCode_t x, brlapi_keyCode_t y)
{
  return brlapi__acceptKeyRange(&defaultHandle, x, y);
}

/* Function : ignore_accept_key_set */
/* Common tasks for ignoring and unignoring key sets */
/* what = 0 for ignoring !0 for unignoring */
static int ignore_accept_key_set(brlapi_handle_t *handle, int what, const brlapi_keyCode_t *s, unsigned int n)
{
  uint32_t buf[2*n];
  int i;
  if (n>BRLAPI_MAXKEYSETSIZE) {
    brlapi_errno = BRLAPI_ERROR_INVALID_PARAMETER;
    return -1;
  }
  for (i=0;i<n;i++) {
    buf[2*i+0] = htonl(s[i] >> 32);
    buf[2*i+1] = htonl(s[i] & 0xffffffff);
  }
  return brlapi__writePacketWaitForAck(handle,(what ? BRLAPI_PACKET_ACCEPTKEYSET : BRLAPI_PACKET_IGNOREKEYSET),buf,sizeof(buf));
}

/* Function : brlapi_ignoreKeySet */
int brlapi__ignoreKeySet(brlapi_handle_t *handle, const brlapi_keyCode_t *s, unsigned int n)
{
  return ignore_accept_key_set(handle, 0,s,n);
}

int brlapi_ignoreKeySet(const brlapi_keyCode_t *s, unsigned int n)
{
  return brlapi__ignoreKeySet(&defaultHandle, s, n);
}

/* Function : brlapi_acceptKeySet */
int brlapi__acceptKeySet(brlapi_handle_t *handle, const brlapi_keyCode_t *s, unsigned int n)
{
  return ignore_accept_key_set(handle, !0,s,n);
}

int brlapi_acceptKeySet(const brlapi_keyCode_t *s, unsigned int n)
{
  return brlapi__acceptKeySet(&defaultHandle, s, n);
}

/* Error code handling */

/* brlapi_errlist: error messages */
const char *brlapi_errlist[] = {
  "Success",                            /* BRLAPI_ERROR_SUCESS */
  "Not enough memory",                  /* BRLAPI_ERROR_NOMEM */
  "Tty Busy",                           /* BRLAPI_ERROR_TTYBUSY */
  "Device busy",                        /* BRLAPI_ERROR_DEVICEBUSY */
  "Unknown instruction",                /* BRLAPI_ERROR_UNKNOWN_INSTRUCTION */
  "Illegal instruction",                /* BRLAPI_ERROR_ILLEGAL_INSTRUCTION */
  "Invalid parameter",                  /* BRLAPI_ERROR_INVALID_PARAMETER */
  "Invalid packet",                     /* BRLAPI_ERROR_INVALID_PACKET */
  "Connection refused",                 /* BRLAPI_ERROR_CONNREFUSED */
  "Operation not supported",            /* BRLAPI_ERROR_OPNOTSUPP */
  "getaddrinfo error",                  /* BRLAPI_ERROR_GAIERR */
  "libc error",                         /* BRLAPI_ERROR_LIBCERR */
  "Couldn't find out tty number",       /* BRLAPI_ERROR_UNKNOWNTTY */
  "Bad protocol version",               /* BRLAPI_ERROR_PROTOCOL_VERSION */
  "Unexpected end of file",             /* BRLAPI_ERROR_EOF */
  "Key file is empty",                  /* BRLAPI_ERROR_EMPTYKEY */
  "Driver error",                       /* BRLAPI_ERROR_DRIVERERROR */
};

/* brlapi_nerr: last error number */
const int brlapi_nerr = (sizeof(brlapi_errlist)/sizeof(char*)) - 1;

/* brlapi_strerror: return error message */
const char *brlapi_strerror(const brlapi_error_t *error)
{
  static char errmsg[128];
  if (error->brlerrno>=brlapi_nerr)
    return "Unknown error";
  else if (error->brlerrno==BRLAPI_ERROR_GAIERR) {
#if defined(EAI_SYSTEM)
    if (error->gaierrno == EAI_SYSTEM)
      snprintf(errmsg,sizeof(errmsg),"resolve: %s", strerror(error->libcerrno));
    else
#endif /* EAI_SYSTEM */
      snprintf(errmsg,sizeof(errmsg),"resolve: "
#if defined(HAVE_GETADDRINFO)
#if defined(HAVE_GAI_STRERROR)
	"%s\n", gai_strerror(error->gaierrno)
#else
	"%d\n", error->gaierrno
#endif
#elif defined(HAVE_HSTRERROR) && !defined(WINDOWS)
	"%s\n", hstrerror(error->gaierrno)
#else
	"%x\n", error->gaierrno
#endif
	);
    return errmsg;
  }
  else if (error->brlerrno==BRLAPI_ERROR_LIBCERR) {
    snprintf(errmsg,sizeof(errmsg),"%s: %s", error->errfun, strerror(error->libcerrno));
    return errmsg;
  } else
    return brlapi_errlist[error->brlerrno];
}

/* brlapi_perror: error message printing */
void brlapi_perror(const char *s)
{
  fprintf(stderr,"%s: %s\n",s,brlapi_strerror(&brlapi_error));
}

typedef struct {
  brlapi_keyCode_t code;
  const char *name;
} brlapi_keyEntry_t;

static const brlapi_keyEntry_t brlapi_keyTable[] = {
#include "brlapi_keytab.auto.h"

  {.code = 0X0000U, .name = "LATIN1"},
  {.code = 0X0100U, .name = "LATIN2"},
  {.code = 0X0200U, .name = "LATIN3"},
  {.code = 0X0300U, .name = "LATIN4"},
  {.code = 0X0400U, .name = "KATAKANA"},
  {.code = 0X0500U, .name = "ARABIC"},
  {.code = 0X0600U, .name = "CYRILLIC"},
  {.code = 0X0700U, .name = "GREEK"},
  {.code = 0X0800U, .name = "TECHNICAL"},
  {.code = 0X0900U, .name = "SPECIAL"},
  {.code = 0X0A00U, .name = "PUBLISHING"},
  {.code = 0X0B00U, .name = "APL"},
  {.code = 0X0C00U, .name = "HEBREW"},
  {.code = 0X0D00U, .name = "THAI"},
  {.code = 0X0E00U, .name = "KOREAN"},
  {.code = 0X1200U, .name = "LATIN8"},
  {.code = 0X1300U, .name = "LATIN9"},
  {.code = 0X1400U, .name = "ARMENIAN"},
  {.code = 0X1500U, .name = "GEORGIAN"},
  {.code = 0X1600U, .name = "CAUCASUS"},
  {.code = 0X1E00U, .name = "VIETNAMESE"},
  {.code = 0X2000U, .name = "CURRENCY"},
  {.code = 0XFD00U, .name = "3270"},
  {.code = 0XFE00U, .name = "XKB"},
  {.code = 0X01000000U, .name = "UNICODE"},

  {.name = NULL}
};

static int
brlapi_getArgumentWidth (brlapi_keyCode_t keyCode) {
  brlapi_keyCode_t code = keyCode & BRLAPI_KEY_CODE_MASK;

  switch (keyCode & BRLAPI_KEY_TYPE_MASK) {
    default: break;

    case BRLAPI_KEY_TYPE_SYM:
      switch (code & 0XFF000000U) {
        default: break;

        case 0X00000000U:
          switch (code & 0XFF0000U) {
            default: break;
            case 0X000000U: return 8;
          }
          break;

        case 0X01000000U: return 24;
      }
      break;

    case BRLAPI_KEY_TYPE_CMD:
      switch (code & BRLAPI_KEY_CMD_BLK_MASK) {
        default: return 16;
        case 0: return 0;
      }
      break;
  }

  brlapi_errno = BRLAPI_ERROR_INVALID_PARAMETER;
  return -1;
}

int
brlapi_expandKeyCode (brlapi_keyCode_t keyCode, brlapi_expandedKeyCode_t *ekc) {
  int argumentWidth = brlapi_getArgumentWidth(keyCode);

  if (argumentWidth != -1) {
    unsigned int argumentMask = (1 << argumentWidth) - 1;
    brlapi_keyCode_t type = keyCode & BRLAPI_KEY_TYPE_MASK;
    brlapi_keyCode_t code = keyCode & BRLAPI_KEY_CODE_MASK;

    ekc->type = type;
    ekc->command = (code & ~argumentMask);
    ekc->argument = code & argumentMask;
    ekc->flags = (keyCode & BRLAPI_KEY_FLAGS_MASK) >> BRLAPI_KEY_FLAGS_SHIFT;

    return 0;
  }

  return -1;
}

const char *
brlapi_getKeyName (unsigned int command, unsigned int argument) {
  unsigned int code = command | argument;
  const brlapi_keyEntry_t *key = brlapi_keyTable;

  while (key->name) {
    if ((command == key->code) || (code == key->code)) return key->name;
    ++key;
  }

  brlapi_errno = BRLAPI_ERROR_INVALID_PARAMETER;
  return NULL;
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

/* We need to declare these as weak external references to determine at runtime 
 * whether libpthread is used or not. We also can't rely on the functions prototypes.
 */
#if defined(WINDOWS)

#elif defined(__GNUC__)
#define WEAK_REDEFINE(name) extern typeof(name) name __attribute__((weak))
WEAK_REDEFINE(pthread_key_create);
WEAK_REDEFINE(pthread_once);
WEAK_REDEFINE(pthread_getspecific);
WEAK_REDEFINE(pthread_setspecific);

#elif defined(__sun__)
#pragma weak pthread_key_create
#pragma weak pthread_once
#pragma weak pthread_getspecific
#pragma weak pthread_setspecific

#endif /* weak external references */

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
        if ((errorp=malloc(sizeof(*errorp))) && !pthread_setspecific(error_key,errorp)) {
	  memset(errorp,0,sizeof(*errorp));
          return errorp;
	}
    }
#ifndef WINDOWS
  }
#endif /* WINDOWS */
  /* fall-back: shared errno :/ */
  return &brlapi_error;
}

brlapi__exceptionHandler_t brlapi__setExceptionHandler(brlapi_handle_t *handle, brlapi__exceptionHandler_t new)
{
  brlapi__exceptionHandler_t tmp;
  pthread_mutex_lock(&handle->exceptionHandler_mutex);
  tmp = handle->exceptionHandlerWithHandle;
  if (new!=NULL) handle->exceptionHandlerWithHandle = new;
  pthread_mutex_unlock(&handle->exceptionHandler_mutex);
  return tmp;
}

brlapi_exceptionHandler_t brlapi_setExceptionHandler(brlapi_exceptionHandler_t new)
{
  brlapi_exceptionHandler_t tmp;
  pthread_mutex_lock(&defaultHandle.exceptionHandler_mutex);
  tmp = defaultHandle.exceptionHandler;
  if (new!=NULL) defaultHandle.exceptionHandler = new;
  pthread_mutex_unlock(&defaultHandle.exceptionHandler_mutex);
  return tmp;
}

int brlapi__strexception(brlapi_handle_t *handle, char *buf, size_t n, int err, brlapi_type_t type, const void *packet, size_t size)
{
  int chars = 16; /* Number of bytes to dump */
  char hexString[3*chars+1];
  int i, nbChars = MIN(chars, size);
  char *p = hexString;
  brlapi_error_t error = { .brlerrno = err };
  for (i=0; i<nbChars; i++)
    p += sprintf(p, "%02x ", ((unsigned char *) packet)[i]);
  p--; /* Don't keep last space */
  *p = '\0';
  return snprintf(buf, n, "%s on %s request of size %d (%s)",
    brlapi_strerror(&error), brlapi_packetType(type), (int)size, hexString);
}

int brlapi_strexception(char *buf, size_t n, int err, brlapi_type_t type, const void *packet, size_t size)
{
  return brlapi__strexception(&defaultHandle, buf, n, err, type, packet, size);
}

void brlapi__defaultExceptionHandler(brlapi_handle_t *handle, int err, brlapi_type_t type, const void *packet, size_t size)
{
  char str[0X100];
  brlapi_strexception(str,0X100, err, type, packet, size);
  fprintf(stderr, "BrlAPI exception: %s\nYou may want to add option -l 7 to the brltty command line for getting debugging information in the system log\n", str);
  abort();
}

void brlapi_defaultExceptionHandler(int err, brlapi_type_t type, const void *packet, size_t size)
{
  brlapi__defaultExceptionHandler(&defaultHandle, err, type, packet, size);
}

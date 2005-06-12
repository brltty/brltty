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

/* api_common.c: Shared definitions */

#include "prologue.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef WINDOWS
#include <ws2tcpip.h>
#include <io.h>
#else /* WINDOWS */
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif /* WINDOWS */

#include "api.h"
#include "api_protocol.h"
#include "api_common.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b))? (a): (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b) (((a) > (b))? (a): (b))
#endif /* MAX */

#ifndef WINDOWS
#define get_osfhandle(fd) (fd)
#endif /* WINDOWS */
#ifdef __MINGW32__
#define get_osfhandle(fd) _get_osfhandle(fd)
#endif /* __MINGW32__ */

#define LibcError(function) \
  brlapi_errno=BRLERR_LIBCERR; \
  brlapi_libcerrno = errno; \
  brlapi_errfun = function;

/* brlapi_writeFile */
/* Writes a buffer to a file */
static ssize_t brlapi_writeFile(int fd, const void *buf, size_t size)
{
  size_t n;
#ifdef WINDOWS
  DWORD res=0;
#else /* WINDOWS */
  ssize_t res=0;
#endif /* WINDOWS */
  for (n=0;n<size;n+=res) {
#ifdef WINDOWS
    OVERLAPPED overl = {0,0,0,0,CreateEvent(NULL,FALSE,FALSE,NULL)};
    if ((!WriteFile((HANDLE) fd,buf+n,size-n,&res,&overl)
      && GetLastError() != ERROR_IO_PENDING) ||
      !GetOverlappedResult((HANDLE) fd, &overl, &res, TRUE)) {
      errno = GetLastError();
      CloseHandle(overl.hEvent);
      res = -1;
    }
    CloseHandle(overl.hEvent);
#else /* WINDOWS */
    res=send(fd,buf+n,size-n,0);
#endif /* WINDOWS */
    if ((res<0) &&
        (errno!=EINTR) &&
#ifdef EWOULDBLOCK
        (errno!=EWOULDBLOCK) &&
#endif /* EWOULDBLOCK */
        (errno!=EAGAIN)) { /* EAGAIN shouldn't happen, but who knows... */
      brlapi_libcerrno=errno;
      brlapi_errfun="write in writeFile";
      brlapi_errno=BRLERR_LIBCERR;
      return res;
    }
  }
  return 0;
}

/* brlapi_readFile */
/* Reads a buffer from a file */
static ssize_t brlapi_readFile(int fd, void *buf, size_t size)
{
  size_t n;
#ifdef WINDOWS
  DWORD res=0;
#else /* WINDOWS */
  ssize_t res=0;
#endif /* WINDOWS */
  for (n=0;n<size && res>=0;n+=res) {
#ifdef WINDOWS
    OVERLAPPED overl = {0,0,0,0,CreateEvent(NULL,FALSE,FALSE,NULL)};
    if ((!ReadFile((HANDLE) fd,buf+n,size-n,&res,&overl)
      && GetLastError() != ERROR_IO_PENDING) ||
      !GetOverlappedResult((HANDLE) fd, &overl, &res, TRUE)) {
      errno = GetLastError();
      CloseHandle(overl.hEvent);
      res = -1;
    }
    CloseHandle(overl.hEvent);
#else /* WINDOWS */
    res=read(fd,buf+n,size-n);
#endif /* WINDOWS */
    if (res==0)
      /* Unexpected end of file ! */
      return n;
    if ((res<0) &&
        (errno!=EINTR) &&
#ifdef EWOULDBLOCK
        (errno!=EWOULDBLOCK) &&
#endif /* EWOULDBLOCK */
        (errno!=EAGAIN)) { /* EAGAIN shouldn't happen, but who knows... */
      LibcError("read in readFile");
      return -1;
    }
  }
  return n;
}

/* brlapi_writePacket */
/* Write a packet on the socket */
ssize_t brlapi_writePacket(int fd, brl_type_t type, const void *buf, size_t size)
{
  uint32_t header[2] = { htonl(size), htonl(type) };
  ssize_t res;

  /* first send packet header (size+type) */
  if ((res=brlapi_writeFile(fd,&header[0],sizeof(header)))<0)
    return res;

  /* eventually data */
  if (size && buf)
    if ((res=brlapi_writeFile(fd,buf,size))<0)
      return res;

  return 0;
}

/* brlapi_readPacketHeader */
/* Read a packet's header and return packet's size */
ssize_t brlapi_readPacketHeader(int fd, brl_type_t *packetType)
{
  uint32_t header[2];
  ssize_t res;
  if ((res=brlapi_readFile(fd,header,sizeof(header))) != sizeof(header)) {
    if (res<0) {
      LibcError("read in brlapi_readPacketHeader");
      return -1;    
    } else return -2;
  }
  *packetType = ntohl(header[1]);
  return ntohl(header[0]);
}

/* brlapi_readPacketContent */
/* Read a packet's content into the given buffer */
/* If the packet is too large, the buffer is filled with the */
/* beginning of the packet, the rest of the packet being discarded */
/* Returns packet size, -1 on failure, -2 on EOF */
ssize_t brlapi_readPacketContent(int fd, size_t packetSize, void *buf, size_t bufSize)
{
  ssize_t res;
  char foo[BRLAPI_MAXPACKETSIZE];
  if ((res = brlapi_readFile(fd,buf,MIN(bufSize,packetSize))) <0) goto out;
  if (res<packetSize) return -2; /* pkt smaller than announced => EOF */
  if (packetSize>bufSize) {
    size_t discard = packetSize-bufSize;
    for (res=0; res<discard / sizeof(foo); res++)
      brlapi_readFile(fd,foo,sizeof(foo));
    brlapi_readFile(fd,foo,discard % sizeof(foo));
  }
  return packetSize;

out:
  LibcError("read in brlapi_readPacket");
  return -1;
}

/* brlapi_readPacket */
/* Read a packet */
/* Returns packet's size, -2 if EOF, -1 on error */
/* If the packet is larger than the supplied buffer, then */
/* the packet is truncated to buffer's size, like in the recv system call */
/* with option MSG_TRUNC (rest of the pcket is read but discarded) */
ssize_t brlapi_readPacket(int fd, brl_type_t *packetType, void *buf, size_t size)
{
  ssize_t res = brlapi_readPacketHeader(fd, packetType);
  if (res<0) return res;
  return brlapi_readPacketContent(fd, res, buf, size);
}

/* Function : brlapi_loadAuthKey */
/* Loads an authentication key from the given file */
/* It is stored in auth, and its size in authLength */
/* If the file is too big, non-existant or unreadable, returns -1 */
int brlapi_loadAuthKey(const char *filename, size_t *authlength, void *auth)
{
  int fd;
  off_t stsize;
  struct stat statbuf;
  if (stat(filename, &statbuf)<0) {
    LibcError("stat in loadAuthKey");
    return -1;
  }
  
  if (statbuf.st_size==0) {
    brlapi_errno = BRLERR_EMPTYKEY;
    brlapi_errfun = "brlapi_laudAuthKey";
    return -1;
  }

  stsize = MIN(statbuf.st_size, BRLAPI_MAXPACKETSIZE-sizeof(uint32_t));

  if ((fd = open(filename, O_RDONLY)) <0) {
    LibcError("open in loadAuthKey");
    return -1;
  }

  *authlength = brlapi_readFile(get_osfhandle(fd), auth, stsize);

  if (*authlength!=(size_t)stsize) {
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

/* Function: brlapi_splitHost
 * splits host into hostname & port */
int brlapi_splitHost(const char *host, char **hostname, char **port) {
  const char *c;
  if (!host || !*host) {
#ifdef PF_LOCAL
    *hostname = NULL;
    *port = strdup("0");
    return PF_LOCAL;
#else /* PF_LOCAL */
    *hostname = strdup("127.0.0.1");
    *port = strdup(BRLAPI_SOCKETPORT);
    return PF_UNSPEC;
#endif /* PF_LOCAL */
  } else if ((c = strrchr(host,':'))) {
    if (c != host) {
      int porti = atoi(c+1);
      if (porti>=(1<<16)-BRLAPI_SOCKETPORTNUM) porti=0;
      *hostname = malloc(c-host+1);
      memcpy(*hostname, host, c-host);
      (*hostname)[c-host] = 0;
      *port = malloc(6);
      snprintf(*port,6,"%u",BRLAPI_SOCKETPORTNUM+porti);
      return PF_UNSPEC;
    } else {
#ifdef PF_LOCAL
      *hostname = NULL;
      *port = strdup(c+1);
      return PF_LOCAL;
#else /* PF_LOCAL */
      int porti = atoi(c+1);
      if (porti>=(1<<16)-BRLAPI_SOCKETPORTNUM) porti=0;
      *hostname = strdup("127.0.0.1");
      *port = malloc(6);
      snprintf(*port,6,"%u",BRLAPI_SOCKETPORTNUM+porti);
      return PF_UNSPEC;
#endif /* PF_LOCAL */
    }
  } else {
    *hostname = strdup(host);
    *port = strdup(BRLAPI_SOCKETPORT);
    return PF_UNSPEC;
  }
}

typedef struct {
  brl_type_t type;
  const char *name;
} brlapi_packetType_t;

brlapi_packetType_t brlapi_packetTypes[] = {
  { BRLPACKET_AUTHKEY, "Auth" },
  { BRLPACKET_GETDRIVERID, "GetDriverId" },
  { BRLPACKET_GETDRIVERNAME, "GetDriverName" },
  { BRLPACKET_GETDISPLAYSIZE, "GetDisplaySize" },
  { BRLPACKET_GETTTY, "GetTty" },
  { BRLPACKET_LEAVETTY, "LeaveTty" },
  { BRLPACKET_KEY, "Key" },
  { BRLPACKET_IGNOREKEYRANGE, "IgnoreKeyRange" },
  { BRLPACKET_IGNOREKEYSET, "IggnoreKeySet" },
  { BRLPACKET_UNIGNOREKEYRANGE, "UnignoreKeyRange" },
  { BRLPACKET_UNIGNOREKEYSET, "UnignoreKeySet" },
  { BRLPACKET_WRITE, "Write" },
  { BRLPACKET_GETRAW, "GetRaw" },
  { BRLPACKET_LEAVERAW, "LeaveRaw" },
  { BRLPACKET_PACKET, "Packet" },
  { BRLPACKET_ACK, "Ack" },
  { BRLPACKET_ERROR, "Error" },
  { BRLPACKET_EXCEPTION, "Exception" },
  { 0, NULL }
};

const char *brlapi_packetType(brl_type_t ptype)
{
  brlapi_packetType_t *p;
  for (p = brlapi_packetTypes; p->type; p++)
    if (ptype==p->type) return p->name;
  return "Unknown";
}

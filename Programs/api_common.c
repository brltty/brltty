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
 * Please see the file COPYING-API for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* api_common.c: communication via the socket */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "api.h"
#include "api_protocol.h"

int brlapi_libcerrno;
const char *brlapi_libcerrfun;

/* brlapi_writeFile */
/* Writes a buffer to a file */
static ssize_t brlapi_writeFile(int fd, const void *buf, size_t size)
{
  size_t n;
  ssize_t res=0;
  for (n=0;n<size;n+=res) {
    res=write(fd,buf+n,size-n);
    if ((res<0) && (errno!=EINTR) && (errno!=EAGAIN) && (errno!=EWOULDBLOCK)) { /* EAGAIN shouldn't happen, but who knows... */
      brlapi_libcerrno=errno;
      brlapi_libcerrfun="write in writeFile";
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
  ssize_t res=0;
  for (n=0;n<size && res>=0;n+=res) {
    res=read(fd,buf+n,size-n);
    if (res==0)
      /* Unexpected end of file ! */
      return n;
    if ((res<0) && (errno!=EINTR) && (errno!=EAGAIN) && (errno!=EWOULDBLOCK)) { /* EAGAIN shouldn't happen, but who knows... */
      brlapi_libcerrno=errno;
      brlapi_libcerrfun="read in readFile";
      brlapi_errno=BRLERR_LIBCERR;
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

/* brlapi_readPacket */
/* Read a packet */
/* Returns packet's size, -2 if EOF, -1 on error */
ssize_t brlapi_readPacket(int fd, brl_type_t *type, void *buf, size_t size)
{
  uint32_t header[2];
  size_t n;
  ssize_t res;
  static unsigned char foo[BRLAPI_MAXPACKETSIZE];

  /* first read packet header (size+type) */
  if ((res=brlapi_readFile(fd,&header[0],sizeof(header))) != sizeof(header)) {
    /* If there is a real error, we return -1 */
    /* if we got not enough bytes for the header, we assume end of file */
    /* is reached and we return -2 */
    if (res<0) return -1; else return -2;
  }
  n = ntohl(header[0]);
  *type = ntohl(header[1]);

  /* if buf is NULL, let's use our fake buffer */
  if (buf == NULL) {
    if (n>BRLAPI_MAXPACKETSIZE) {
      brlapi_libcerrno=EFBIG;
      brlapi_libcerrfun="read in readPacket";
      brlapi_errno=BRLERR_LIBCERR;
      return -1;
    }
    buf=foo;
  } else
    /* check that his buffer is large enough */
    if (n>size) {
      brlapi_libcerrno=EFBIG;
      brlapi_libcerrfun="read in readPacket";
      brlapi_errno=BRLERR_LIBCERR;
      return -1;
    }

  if ((res=brlapi_readFile(fd,buf,n)) != n) {
    if (res<0) return -1; else return -2;
  }

  return n;
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
    brlapi_libcerrno=errno;
    brlapi_libcerrfun="stat in loadAuthKey";
    brlapi_errno=BRLERR_LIBCERR;
    return -1;
  }

  if ((stsize = statbuf.st_size)>BRLAPI_MAXPACKETSIZE) {
    brlapi_libcerrno=EFBIG;
    brlapi_libcerrfun="stat in loadAuthKey";
    brlapi_errno=BRLERR_LIBCERR;
    return -1;
  }

  if ((fd = open(filename, O_RDONLY)) <0) {
    brlapi_libcerrno=errno;
    brlapi_libcerrfun="open in loadAuthKey";
    brlapi_errno=BRLERR_LIBCERR;
    return -1;
  }

  *authlength = brlapi_readFile(fd, auth, stsize);

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
    *hostname = NULL;
    *port = strdup("0");
    return PF_LOCAL;
  } else if ((c = strrchr(host,':'))) {
    if (c != host) {
      int porti = atoi(c+1);
      if (porti>=(1<<16)-BRLAPI_SOCKETPORTNUM) porti=0;
      *hostname = (char *)malloc(c-host+1);
      memcpy(*hostname, host, c-host);
      (*hostname)[c-host] = 0;
      *port = (char *)malloc(6);
      snprintf(*port,6,"%u",BRLAPI_SOCKETPORTNUM+porti);
      return PF_UNSPEC;
    } else {
      *hostname = NULL;
      *port = strdup(c+1);
      return PF_LOCAL;
    }
  } else {
    *hostname = strdup(host);
    *port = strdup(BRLAPI_SOCKETPORT);
    return PF_UNSPEC;
  }
}

typedef struct {
  brl_type_t type;
  char *name;
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
  { 0, NULL }
};

const char *brlapi_packetType(brl_type_t ptype)
{
  brlapi_packetType_t *p;
  for (p = brlapi_packetTypes; p->type; p++)
    if (ptype==p->type) return p->name;
  return "Unknown";
}

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
    if ((res<0) && (errno!=EINTR) && (errno!=EAGAIN)) { /* EAGAIN shouldn't happen, but who knows... */
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
    if ((res<0) && (errno!=EINTR) && (errno!=EAGAIN)) { /* EAGAIN shouldn't happen, but who knows... */
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
      /* brlapi_writePacket(fd,PACKET_BYE,NULL,0); */
      brlapi_libcerrno=EFBIG;
      brlapi_libcerrfun="read in readPacket";
      brlapi_errno=BRLERR_LIBCERR;
      return -1;
    }
    buf=foo;
  } else
    /* check that his buffer is large enough */
    if (n>size) {
      /* brlapi_writePacket(fd,PACKET_BYE,NULL,0); */
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
void brlapi_splitHost(const char *host, char **hostname, char **port) {
  const char *c;
  if (!host || !*host) {
    *hostname=NULL;
    *port=strdup(BRLAPI_SOCKETPORT);
  } else if ((c = strchr(host,':'))) {
    if (c != host) {
      *hostname = (char *)malloc(c-host+1);
      memcpy(*hostname, host, c-host);
      *hostname[c-host] = 0;
    }
    else
      *hostname = NULL;
    *port = strdup(c+1);
  } else {
    *hostname = strdup(host);
    *port = strdup(BRLAPI_SOCKETPORT);
  }
}

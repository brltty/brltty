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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

#include "brlapi.h"

/* brlapi_writeFile */
/* Writes a buffer to a file */
/* Returns 0 if success, -1 if failure */
int brlapi_writeFile(int fd, const unsigned char *buf, size_t size)
{
 int n;
 int res=0;
 for (n=0;n<size;n+=res)
 {
  res=write(fd,buf+n,size-n);
  if (res==0) {
   fprintf(stderr,"Couldn't write !\n");
   close(fd);
   return -1;
  }
  if ((res<0) && (errno!=EINTR) && (errno!=EAGAIN)) /* EAGAIN shouldn't happen, but who knows... */
  {
   perror("Socket error while writing, can't recover");
   close(fd);	/* for safety */
   return res;
  }
 }
 return 0;
}

/* brlapi_readFile */
/* Reads a buffer from a file */
/* Returns the number of bytes read; */
/* Returns 0 if the end of file is reached before size bytes are read; */
/* Returns -1 if a read() system call fails */
int brlapi_readFile(int fd, unsigned char *buf, size_t size)
{
 int n;
 int res=0;
 for (n=0;n<size && res>=0;n+=res)
 {
  res=read(fd,buf+n,size-n);
  if (res==0) {
   fprintf(stderr,"Unexpected end of file !\n");
   close(fd);
   return 0;
  }
  if ((res<0) && (errno!=EINTR) && (errno!=EAGAIN)) /* EAGAIN shouldn't happen, but who knows... */
  {
   perror("Socket error while reading, can't recover");
   close(fd);	/* for safety */
   return -1;
  }
 }
 return size;
}

/* brlapi_writePacket */
/* Write a packet on the socket */
int brlapi_writePacket(int fd, size_t size, brl_type_t type, const unsigned char *buf)
{
 uint32_t header[2];
 int res;
 
 /* first send packet header (size+type) */
 header[0] = htonl(size);
 header[1] = htonl(type);
 if ((res=brlapi_writeFile(fd,(unsigned char *) &header[0],sizeof(header)))==-1)
 {
  perror("writing packet header");
  return res;
 }

 /* eventually data */
 if (size && buf)
 if ((res=brlapi_writeFile(fd,buf,size)) == -1)
 {
  perror("writing data");
  return res;
 }
 return 0;
}

/* brlapi_readPacket */
/* Read a packet */
/* Returns packet's size, -2 if EOF, -1 on error */
int brlapi_readPacket(int fd, size_t size, brl_type_t *type, unsigned char *buf)
{
 uint32_t header[2]; 
 int n,res;
 static unsigned char foo[BRLAPI_MAXPACKETSIZE];

 /* first read packet header (size+type) */
 if ((res=brlapi_readFile(fd,(unsigned char *) &header[0],sizeof(header))) != sizeof(header))
 {
  if (res<0) perror("reading packet size and type");
  /* If there is a real error, we return -1 */
  /* if we got not enough bytes for the header, we assume end of file */
  /* is reached and we return -2 */
  if (res<0) return -1; else return -2;
 }
 n = ntohl(header[0]);
 *type = ntohl(header[1]);

 /* if buf is NULL, let's use our fake buffer */
 if (buf == NULL)
 {
  if (n>BRLAPI_MAXPACKETSIZE)
  {
   fprintf(stderr,"packet really too big : %d\n",n);
   /* brlapi_writePacket(fd,0,PACKET_BYE,NULL); */
   errno=0;
   return -1;
  }
  buf=foo;
 } else
 /* check that his buffer is large enough */
 if (n>size)
 {
  fprintf(stderr,"buffer too small : %d while %d are needed !\n", size, n);
  /* brlapi_writePacket(fd,0,PACKET_BYE,NULL); */
  errno=0;
  return -1;
 }

 if ((res=brlapi_readFile(fd,buf,n)) != n)
 {
  /* perror("reading data"); */
  if (res<0) return -1; else return -2;
 }
 return n;
}

/* Fonction : brlapi_loadAuthKey */
/* Loads an authentication key from the given file */
/* It is stored in auth, and its size in authLength */
/* If the file is too big, non-existant or unreadable, returns -1 */
int brlapi_loadAuthKey(const char *filename, int *authlength, void *auth)
{
 int res, fd;
 struct stat statbuf;
 if (stat(filename, &statbuf)<0)
 {
  /* LogPrint(LOG_ERROR,"stat : %s",strerror(errno)); */
  return -1;
 }
 if ((res = statbuf.st_size)>BRLAPI_MAXPACKETSIZE)
 {
  /* LogPrint(LOG_ERROR,"Key file too big"); */
  return -1;
 }
 if ((fd = open(filename, O_RDONLY)) <0) 
 {
  /* LogPrint(LOG_ERROR,"open : %s",strerror(errno)); */
  return -1;
 }
 *authlength = read(fd, auth, res);
 if (*authlength!=res)
 {
  /* LogPrint(LOG_ERROR,"read failed"); */
  close(fd);
  return -1;
 } 
 close(fd);
 return 0;
}

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

#ifndef BRLTTY_INCLUDED_API_COMMON
#define BRLTTY_INCLUDED_API_COMMON

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* api_common.h - private declarations shared by both server & client */

#include <unistd.h>

/* brlapi_writeFile */
/* Exactly write a buffer in a file */
ssize_t brlapi_writeFile(int fd, const unsigned char *buf, size_t size);

/* brlapi_readFile */
/* Exactly read a buffer from a file */
ssize_t brlapi_readFile(int fd, unsigned char *buf, size_t size);

/* brlapi_libcerrno */
/* saves the libc errno */
int brlapi_libcerrno;

/* brlapi_liberrfun */
/* saves the libc function which generated an error */
const char *brlapi_libcerrfun;

/* brlapi_splitHost */
/* splits host into hostname & port, returns address family to use */
int brlapi_splitHost(const char *host, char **hostname, char **port);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_API_COMMON */

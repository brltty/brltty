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

#ifndef _API_COMMON_H
#define _API_COMMON_H

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
/* splits host into hostname & port */
void brlapi_splitHost(const char *host, char **hostname, char **port);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _API_COMMON_H */

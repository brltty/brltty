/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2005-2018 by
 *   Alexis Robert <alexissoft@free.fr>
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* bindings.c provides initialized variables and brlapi exception handler to
 * the Python bindings */


#include "brlapi.h"

/* a kludge to get around a broken MinGW <pthread.h> header */
#ifdef __MINGW32__
#ifdef __struct_timespec_defined
#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
#endif /* _TIMESPEC_DEFINED */
#endif /* __struct_timespec_defined */
#endif /* __MINGW32__ */

#include <pthread.h>

#include <stdlib.h>
#include <string.h>

#include "bindings.h"

const brlapi_writeArguments_t brlapi_writeArguments_initialized = BRLAPI_WRITEARGUMENTS_INITIALIZER;

static pthread_once_t brlapi_protocolExceptionOnce = PTHREAD_ONCE_INIT;
static pthread_key_t brlapi_protocolExceptionKey;
static char *brlapi_protocolExceptionSingleThread;

#if defined(WINDOWS)

#elif defined(__GNUC__) || defined(__sun__)
#pragma weak pthread_once
#pragma weak pthread_key_create
#pragma weak pthread_getspecific
#pragma weak pthread_setspecific
#endif /* weak external references */

static void BRLAPI_STDCALL brlapi_pythonExceptionHandler(brlapi_handle_t *handle, int error, brlapi_packetType_t type, const void *packet, size_t size)
{
  char str[128];

  brlapi__strexception(handle, str, sizeof(str), error, type, packet, size);
#ifndef WINDOWS
  if (!(pthread_once && pthread_key_create))
    brlapi_protocolExceptionSingleThread = strdup(str);
  else
#endif
    pthread_setspecific(brlapi_protocolExceptionKey, strdup(str));
}

char *brlapi_protocolException(void)
{
  char *exception;
#ifndef WINDOWS
  if (!(pthread_once && pthread_key_create)) {
    exception = brlapi_protocolExceptionSingleThread;
    brlapi_protocolExceptionSingleThread = NULL;
    return exception;
  } else
#endif
  {
    exception = pthread_getspecific(brlapi_protocolExceptionKey);
    pthread_setspecific(brlapi_protocolExceptionKey, NULL);
    return exception;
  }
}

static void do_brlapi_protocolExceptionInit(void)
{
  pthread_key_create(&brlapi_protocolExceptionKey, free);
}

void brlapi_protocolExceptionInit(brlapi_handle_t *handle) {
#ifndef WINDOWS
  if (pthread_once && pthread_key_create)
#endif /* WINDOWS */
    pthread_once(&brlapi_protocolExceptionOnce, do_brlapi_protocolExceptionInit);

  brlapi__setExceptionHandler(handle, &brlapi_pythonExceptionHandler);
}

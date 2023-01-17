/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2005-2023 by
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
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* bindings.c provides initialized variables and brlapi exception handler to
 * the Python bindings */


/* include Python.h first in order to prevent a redefine of _POSIX_C_SOURCE */
#include <Python.h>

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

/* brlapi_python_parameter_callback */
/* This is called from brlapi functions called by Python. We pass the callback
 * data as such for brlapi.pyx' callback to translate them into python objects
 */
static void brlapi_python_parameter_callback(brlapi_param_t parameter, brlapi_param_subparam_t subparam, brlapi_param_flags_t flags, void *priv, const void *data, size_t len)
{
  brlapi_python_paramCallbackDescriptor_t *descr = priv;
  PyObject *arglist, *result;
  PyGILState_STATE gstate;
  brlapi_python_callbackData_t callbackData = {
    .parameter = parameter,
    .subparam = subparam,
    .flags = flags,
    .data = data,
    .len = len,
  };

  gstate = PyGILState_Ensure();

  arglist = Py_BuildValue("(L)", (long long) (uintptr_t) &callbackData);

  result = PyObject_CallObject(descr->callback, arglist);
  if (result != NULL)
    Py_DECREF(result);

  PyGILState_Release(gstate);
}

brlapi_python_paramCallbackDescriptor_t *brlapi_python_watchParameter(brlapi_handle_t *handle, brlapi_param_t param, brlapi_param_subparam_t subparam, brlapi_param_flags_t flags, PyObject *func)
{
  brlapi_python_paramCallbackDescriptor_t *descr;
  brlapi_paramCallbackDescriptor_t *brlapi_descr;
  PyObject *result;

  if (!PyCallable_Check(func)) {
    PyErr_SetString(PyExc_TypeError, "parameter must be callable");
    return NULL;
  }

  Py_INCREF(func);

  descr = malloc(sizeof(*descr));
  descr->callback = func;

  brlapi_descr = brlapi__watchParameter(handle, param, subparam, flags, brlapi_python_parameter_callback, descr, NULL, 0);

  if (!brlapi_descr) {
    free(descr);
    PyErr_SetString(PyExc_ValueError, "watching parameter failed");
    return NULL;
  }

  descr->brlapi_descr = brlapi_descr;

  return descr;
}

int brlapi_python_unwatchParameter(brlapi_handle_t *handle, brlapi_python_paramCallbackDescriptor_t *descr)
{
  int ret;

  ret = brlapi__unwatchParameter(handle, descr->brlapi_descr);
  Py_DECREF(descr->callback);
  free(descr);

  return ret;
}

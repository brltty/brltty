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

/* bindings.h provides initialized variables to the Python bindings */

#include "brlapi.h"

typedef struct {
  brlapi_paramCallbackDescriptor_t brlapi_descr;
  PyObject *callback;
} brlapi_python_paramCallbackDescriptor_t;

typedef struct {
  brlapi_param_t parameter;
  brlapi_param_subparam_t subparam;
  brlapi_param_flags_t flags;
  const void *data;
  size_t len;
} brlapi_python_callbackData_t;

extern const brlapi_writeArguments_t brlapi_writeArguments_initialized;
extern char *brlapi_protocolException(void);
extern void brlapi_protocolExceptionInit(brlapi_handle_t *handle);

extern brlapi_python_paramCallbackDescriptor_t *brlapi_python_watchParameter(brlapi_handle_t *handle, brlapi_param_t param, brlapi_param_subparam_t subparam, brlapi_param_flags_t flags, PyObject *args);
extern int brlapi_python_unwatchParameter(brlapi_handle_t *handle, brlapi_python_paramCallbackDescriptor_t *descr);

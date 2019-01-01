/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2019 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
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

#ifndef BRLAPI_INCLUDED_JAVA_BINDINGS
#define BRLAPI_INCLUDED_JAVA_BINDINGS

#include "common_java.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAVA_GET_CONSTRUCTOR(env,class,arguments) \
  ((*(env))->GetMethodID((env), (class), JAVA_CONSTRUCTOR_NAME, JAVA_SIG_CONSTRUCTOR(arguments)))

#define JAVA_GET_FIELD(env,type,object,field) \
  ((*(env))->Get ## type ## Field((env), (object), (field)))

#define JAVA_SET_FIELD(env,type,object,field,value) \
   ((*(env))->Set ## type ## Field((env), (object), (field), (value)))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLAPI_INCLUDED_JAVA_BINDINGS */

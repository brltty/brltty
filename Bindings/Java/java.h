/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2018 by
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
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLAPI_INCLUDED_JAVA_BINDINGS
#define BRLAPI_INCLUDED_JAVA_BINDINGS

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAVA_OBJECT(name) "java/lang/" name
#define JAVA_OBJECT_NULL_POINTER_EXCEPTION JAVA_OBJECT("NullPointerException")
#define JAVA_OBJECT_OUT_OF_MEMORY_ERROR JAVA_OBJECT("OutOfMemoryError")
#define JAVA_OBJECT_STRING JAVA_OBJECT("String")

#define JAVA_SIG_BYTE                      "B"
#define JAVA_SIG_CHAR                      "C"
#define JAVA_SIG_DOUBLE                    "D"
#define JAVA_SIG_FLOAT                     "F"
#define JAVA_SIG_INT                       "I"
#define JAVA_SIG_LONG                      "J"
#define JAVA_SIG_OBJECT(path)              "L" path ";"
#define JAVA_SIG_SHORT                     "S"
#define JAVA_SIG_VOID                      "V"
#define JAVA_SIG_BOOLEAN                   "Z"
#define JAVA_SIG_ARRAY(type)               "[" type
#define JAVA_SIG_METHOD(returns,arguments) "(" arguments ")" returns

#define JAVA_CONSTRUCTOR_NAME "<init>"
#define JAVA_SIG_CONSTRUCTOR(arguments) JAVA_SIG_METHOD(JAVA_SIG_VOID, arguments)

#define JAVA_SIG_STRING JAVA_SIG_OBJECT(JAVA_OBJECT_STRING)

#define JAVA_METHOD(object,name,returns) \
  JNIEXPORT returns JNICALL Java_ ## object ## _ ## name (JNIEnv *env

#define JAVA_INSTANCE_METHOD(object,name,returns,...) \
  JAVA_METHOD(object,name,returns), jobject this, ## __VA_ARGS__)

#define JAVA_STATIC_METHOD(object,name,returns,...) \
  JAVA_METHOD(object,name,returns), jclass class, ## __VA_ARGS__)

#define GET_FIELD(env,type,object,field) \
  (*(env))->Get ## type ## Field((env), (object), (field))

#define SET_FIELD(env,type,object,field,value) \
   (*(env))->Set ## type ## Field((env), (object), (field), (value));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLAPI_INCLUDED_JAVA_BINDINGS */

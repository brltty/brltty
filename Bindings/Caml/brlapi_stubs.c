/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2005-2007 by
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define CAML_NAME_SPACE /* Don't import old names */
#include <caml/mlvalues.h> /* definition of the value type, and conversion macros */
#include <caml/memory.h> /* miscellaneous memory-related functions and macros (for GC interface, in-place modification of structures, etc). */
#include <caml/alloc.h> /* allocation functions (to create structured Caml objects) */
#include <caml/fail.h> /* functions for raising exceptions */
#include <caml/callback.h> /* callback from C to Caml */
#include <caml/custom.h> /* operations on custom blocks */
#include <caml/intext.h> /* operations for writing user-defined serialization and deserialization functions for custom blocks */
#define BRLAPI_NO_DEPRECATED
#include <brlapi.h>

#ifndef MIN
#define MIN(x, y) (x<y)?(x):(y)
#endif /* MIN */

extern value unix_error_of_code (int errcode); /* TO BE REMOVED */

/* The following macros call a BrlAPI function */
/* The first one just calls thefunction, whereas */
/* thesecond one also checks thefunction's return code and raises */
/* an exception if this code is -1 */
/* The macros decide which version of a brlapi function should be called */
/* depending om whether the handle value is None or Some realHandle */

#define brlapi(function, ...) \
do { \
  if (Is_long(handle)) brlapi_ ## function (__VA_ARGS__); \
  else brlapi__ ## function ((brlapi_handle_t *) Data_custom_val(Field(handle, 0)), ## __VA_ARGS__); \
} while (0)

#define brlapiCheckError(function, ret, ...) \
do { \
  int res_; \
  if (Is_long(handle)) res_ = brlapi_ ##function (__VA_ARGS__); \
  else res_ = brlapi__ ##function ((brlapi_handle_t *) Data_custom_val(Field(handle, 0)), ## __VA_ARGS__); \
  if (res_==-1) raise_brlapi_error(#function); \
  if (ret!=NULL) (*(int *)ret) = res_; \
} while (0)

static int compareHandle(value h1, value h2)
{
  CAMLparam2(h1, h2);
  CAMLreturn(memcmp(Data_custom_val(h1), Data_custom_val(h2), brlapi_getHandleSize()));
}

static struct custom_operations customOperations = {
  .identifier = "BrlAPI handle",
  .finalize = custom_finalize_default,
  .compare = compareHandle,
  .hash = custom_hash_default, /* FIXME: provide a genuine hashing function */
  .serialize = custom_serialize_default,
  .deserialize = custom_deserialize_default,
};

/* Function : constrCamlError */
/* Converts a brlapi_error_t into its Caml representation */
static value constrCamlError(const brlapi_error_t *err)
{
  CAMLlocal1(v);
  switch (err->brlerrno) {
    case BRLAPI_ERROR_NOMEM: v = Val_int(0); break;
    case BRLAPI_ERROR_TTYBUSY: v = Val_int(1); break;
    case BRLAPI_ERROR_DEVICEBUSY: v = Val_int(2); break;
    case BRLAPI_ERROR_UNKNOWN_INSTRUCTION: v = Val_int(3); break;
    case BRLAPI_ERROR_ILLEGAL_INSTRUCTION: v = Val_int(4); break;
    case BRLAPI_ERROR_INVALID_PARAMETER: v = Val_int(5); break;
    case BRLAPI_ERROR_INVALID_PACKET: v = Val_int(6); break;
    case BRLAPI_ERROR_CONNREFUSED: v = Val_int(7); break;
    case BRLAPI_ERROR_OPNOTSUPP: v = Val_int(8); break;
    case BRLAPI_ERROR_GAIERR: {
      v = caml_alloc(1, 0);
      Store_field(v, 0, Val_int(err->gaierrno));
    }; break;
    case BRLAPI_ERROR_LIBCERR: {
      v = caml_alloc(1, 1);
      Store_field(v, 0, unix_error_of_code(err->libcerrno));
    }; break;
    case BRLAPI_ERROR_UNKNOWNTTY: v = Val_int(9); break;
    case BRLAPI_ERROR_PROTOCOL_VERSION: v = Val_int(10); break;
    case BRLAPI_ERROR_EOF: v = Val_int(11); break;
    case BRLAPI_ERROR_EMPTYKEY: v = Val_int(12); break; 
    case BRLAPI_ERROR_DRIVERERROR: v = Val_int(13); break;
    default: {
      v = caml_alloc(1, 2);
      Store_field(v, 0, Val_int(err->brlerrno));
    }
  }
  return v;
}

/* Function : raise_brlapi_err[Bor */
/* Raises the Brlapi_error exception */
static void raise_brlapi_error(char *fun)
{
  static value *exception = NULL;
  CAMLlocal1(res);
  if (exception==NULL) exception = caml_named_value("Brlapi_error");
  res = caml_alloc(3,0);
  Store_field(res, 0, *exception);
  Store_field(res, 1, constrCamlError(&brlapi_error));
  Store_field(res, 2, caml_copy_string(fun));
  caml_raise(res);
}

/* Function : raise_brlapi_exception */
/* Raises Brlapi_exception*/
static void raise_brlapi_exception(int err, brlapi_type_t type, const void *packet, size_t size)
{
  static value *exception = NULL;
  int i;
  CAMLlocal2(str, res);
  str = caml_alloc_string(size);
  for (i=0; i<size; i++) Byte(str, i) = ((char *) packet)[i];
  if (exception==NULL) exception = caml_named_value("Brlapi_exception");
  res = caml_alloc (4, 0);
  Store_field(res, 0, *exception);
  Store_field(res, 1, Val_int(err));
  Store_field(res, 2, caml_copy_int64(type));
  Store_field(res, 3, str);
  caml_raise(res);
}

/* function packDots */
/* Converts Caml dots in brltty dots */
static inline void packDots(value camlDots, unsigned char *dots, int size)
{
  unsigned int i;
  for (i=0; i<size; i++)
    dots[i] = Int_val(Field(camlDots, i));
}

CAMLprim value brlapiml_openConnection(value settings)
{
  CAMLparam1(settings);
  CAMLlocal2(s, pair);
  int res;
  brlapi_connectionSettings_t brlapiSettings;
  brlapiSettings.auth = String_val(Field(settings, 0));
  brlapiSettings.host = String_val(Field(settings, 1));
  res = brlapi_openConnection(&brlapiSettings, &brlapiSettings);
  if (res<0) raise_brlapi_error("openConnection");
  s = caml_alloc_tuple(2);
  Store_field(s, 0, caml_copy_string(brlapiSettings.auth));
  Store_field(s, 1, caml_copy_string(brlapiSettings.host));
  pair = caml_alloc_tuple(2);
  Store_field(pair, 0, Val_int(res));
  Store_field(pair, 1, s);
  CAMLreturn(pair);
}

CAMLprim value brlapiml_openConnectionHandle(value settings)
{
  CAMLparam1(settings);
  CAMLlocal1(handle);
  brlapi_connectionSettings_t brlapiSettings;
  brlapiSettings.auth = String_val(Field(settings, 0));
  brlapiSettings.host = String_val(Field(settings, 1));
  handle = caml_alloc_custom(&customOperations, brlapi_getHandleSize(), 0, 1);
  if (brlapi__openConnection(Data_custom_val(handle), &brlapiSettings, &brlapiSettings)<0) raise_brlapi_error("openConnectionHandle");
  CAMLreturn(handle);
}

CAMLprim value brlapiml_closeConnection(value handle, value unit)
{
  CAMLparam2(handle, unit);
  brlapi(closeConnection);
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_getDriverName(value handle, value unit)
{
  CAMLparam2(handle, unit);
  char name[BRLAPI_MAXNAMELENGTH];
  brlapiCheckError(getDriverName, NULL, name, sizeof(name));
  CAMLreturn(caml_copy_string(name));
}

CAMLprim value brlapiml_getDisplaySize(value handle, value unit)
{
  CAMLparam2(handle, unit);
  CAMLlocal1(size);
  unsigned int x, y;
  brlapiCheckError(getDisplaySize, NULL, &x, &y);
  size = caml_alloc_tuple(2);
  Store_field(size, 0, Val_int(x));
  Store_field(size, 1, Val_int(y));
  CAMLreturn(size);
}

CAMLprim value brlapiml_enterTtyMode(value handle, value tty, value driverName)
{
  CAMLparam3(handle, tty, driverName);
  int res;
  brlapiCheckError(enterTtyMode, &res, Int_val(tty), String_val(driverName));
  CAMLreturn(Val_int(res));
}

CAMLprim value brlapiml_enterTtyModeWithPath(value handle, value ttyPathCaml, value driverName)
{
  CAMLparam2(handle, ttyPathCaml);
  int i, size = Wosize_val(ttyPathCaml);
  int ttyPath[size];
  for (i=0; i<size; i++) ttyPath[i] = Int_val(Field(ttyPathCaml, i));
  brlapiCheckError(enterTtyModeWithPath, NULL, ttyPath, size, String_val(driverName));
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_leaveTtyMode(value handle, value unit)
{
  CAMLparam2(handle, unit);
  brlapi(leaveTtyMode);
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_setFocus(value handle, value tty)
{
  CAMLparam2(handle, tty);
  brlapiCheckError(setFocus, NULL, Int_val(tty));
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_writeText(value handle, value cursor, value text)
{
  CAMLparam3(handle, cursor, text);
 brlapiCheckError(writeText, NULL, Int_val(cursor), String_val(text));
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_writeDots(value handle, value camlDots)
{
  CAMLparam2(handle, camlDots);
  int size = Wosize_val(camlDots);
  unsigned char dots[size];
  packDots(camlDots, dots, size);
  brlapiCheckError(writeDots, NULL, dots);
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_write_(value handle, value writeStruct)
{
  CAMLparam2(handle, writeStruct);
  int andSize = Wosize_val(Field(writeStruct, 4));
  int orSize = Wosize_val(Field(writeStruct, 5));
  unsigned char attrAnd[andSize], attrOr[orSize];
  brlapi_writeStruct_t ws;
  ws.displayNumber = Val_int(Field(writeStruct, 0));
  ws.regionBegin = Val_int(Field(writeStruct, 1));
  ws.regionSize = Val_int(Field(writeStruct, 2));
  ws.text = String_val(Field(writeStruct, 3));
  packDots(Field(writeStruct, 4), attrAnd, andSize);
  ws.attrAnd = attrAnd;
  packDots(Field(writeStruct, 5), attrOr, orSize);
  ws.attrOr = attrOr;
  ws.cursor = Val_int(Field(writeStruct, 6));
  brlapiCheckError(write, NULL, &ws);
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_readKey(value handle, value unit)
{
  CAMLparam2(handle, unit);
  int res;
  brlapi_keyCode_t keyCode;
  CAMLlocal1(retVal);
  brlapiCheckError(readKey, &res, 0, &keyCode);
  if (res==0) CAMLreturn(Val_int(0));
  retVal = caml_alloc(1, 1);
  Store_field(retVal, 0, caml_copy_int64(keyCode));
  CAMLreturn(retVal);
}

CAMLprim value brlapiml_waitKey(value handle, value unit)
{
  CAMLparam2(handle, unit);
  brlapi_keyCode_t keyCode;
  brlapiCheckError(readKey, NULL, 1, &keyCode);
  CAMLreturn(caml_copy_int64(keyCode));
}

CAMLprim value brlapiml_expandKeyCode(value camlKeyCode)
{
  CAMLparam1(camlKeyCode);
  CAMLlocal2(key, result);
  tag_t keyType = 0;
  brlapi_expandedKeyCode_t ekc;
  brlapi_keyCode_t keyCode = Int64_val(camlKeyCode);
  if ((keyCode & BRLAPI_KEY_TYPE_MASK) == BRLAPI_KEY_TYPE_CMD) keyType=0;
  else if ((keyCode & BRLAPI_KEY_TYPE_MASK) == BRLAPI_KEY_TYPE_SYM) keyType=1;
  else raise_brlapi_error("expandKeyCode: unknown key type");
  if (brlapi_expandKeyCode(keyCode, &ekc)==-1)
    raise_brlapi_error("expandKeyCode");
  key = caml_alloc(1, keyType);
  if (keyType==0) Store_field(key, 0, Val_int(ekc.command));
  else Store_field(key, 0, caml_copy_int32(ekc.command));
  result = caml_alloc_tuple(3);
  Store_field(result, 0, key);
  Store_field(result, 1, Val_int(ekc.argument));
  Store_field(result, 2, caml_copy_int32(ekc.flags));
  CAMLreturn(result);
}

CAMLprim value brlapiml_ignoreKeyRange(value handle, value x, value y)
{
  CAMLparam3(handle, x, y);
  brlapiCheckError(ignoreKeyRange, NULL, Int64_val(x), Int64_val(y));;
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_acceptKeyRange(value handle, value x, value y)
{
  CAMLparam3(handle, x, y);
  brlapiCheckError(acceptKeyRange, NULL, Int64_val(x), Int64_val(y));
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_ignoreKeySet(value handle, value array)
{
  CAMLparam2(handle, array);
  int i, nbKeys = Wosize_val(array);
  brlapi_keyCode_t keySet[nbKeys];
  for (i=0; i<nbKeys; i++) keySet[i] = Int64_val(Field(array, i));
  brlapiCheckError(ignoreKeySet, NULL, keySet, nbKeys);
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_acceptKeySet(value handle, value array)
{
  CAMLparam2(handle, array);
  int i, nbKeys = Wosize_val(array);
  brlapi_keyCode_t keySet[nbKeys];
  for (i=0; i<nbKeys; i++) keySet[i] = Int64_val(Field(array, i));
  brlapiCheckError(acceptKeySet, NULL, keySet, nbKeys);
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_enterRawMode(value handle, value driverName)
{
  CAMLparam2(handle, driverName);
  brlapiCheckError(enterRawMode, NULL, String_val(driverName));
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_leaveRawMode(value handle, value unit)
{
  CAMLparam2(handle, unit);
  brlapi(leaveRawMode);
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_sendRaw(value handle, value str)
{
  CAMLparam2(handle, str);
  ssize_t res;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  ssize_t i, size = MIN(sizeof(packet), caml_string_length(str));
  for (i=0; i<size; i++) packet[i] = Byte(str, i);
  brlapiCheckError(sendRaw, &res, packet, size);
  CAMLreturn(Val_int(res));
}

CAMLprim value brlapiml_recvRaw(value handle, value unit)
{
  CAMLparam2(handle, unit);
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  ssize_t i, size;
  CAMLlocal1(str);
  brlapiCheckError(recvRaw, &size, packet, sizeof(packet));
  str = caml_alloc_string(size);
  for (i=0; i<size; i++) Byte(str, i) = packet[i];
  CAMLreturn(str);
}

CAMLprim value brlapiml_suspendDriver(value handle, value driverName)
{
  CAMLparam2(handle, driverName);
  brlapiCheckError(suspendDriver, NULL, String_val(driverName));
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_resumeDriver(value handle, value unit)
{
  CAMLparam2(handle, unit);
  brlapi(resumeDriver);
  CAMLreturn(Val_unit);
}

/* Function : setExceptionHandler */
/* Sets a handler that raises a Caml exception each time a brlapi */
/* exception occurs */
CAMLprim value brlapiml_setExceptionHandler(value unit)
{
  CAMLparam1(unit);
  brlapi_setExceptionHandler(raise_brlapi_exception);
  CAMLreturn(Val_unit);
}

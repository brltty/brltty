/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2005-2008 by
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 * All rights reserved.
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
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
#include "brlapi.h"
#include "brlapi_protocol.h"

#ifndef MIN
#define MIN(x, y) (x<y)?(x):(y)
#endif /* MIN */

extern value unix_error_of_code (int errcode); /* TO BE REMOVED */

/* The following macros call a BrlAPI function */
/* The first one just calls the function, whereas */
/* the second one also checks the function's return code and raises */
/* an exception if this code is -1 */
/* The macros decide which version of a brlapi function should be called */
/* depending on whether the handle value is None or Some realHandle */

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
  if (res_==-1) raise_brlapi_error(); \
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
  value camlError;
  camlError = caml_alloc_tuple(4);
  Store_field(camlError, 0, Val_int(err->brlerrno));
  Store_field(camlError, 1, Val_int(err->libcerrno));
  Store_field(camlError, 2, Val_int(err->gaierrno));
  if (err->errfun!=NULL)
    Store_field(camlError, 3, caml_copy_string(err->errfun));
  else
    Store_field(camlError, 3, caml_copy_string(""));
  return camlError;
}

CAMLprim value brlapiml_errorCode_of_error(value camlError)
{
  CAMLparam1(camlError);
  CAMLlocal1(result);
  switch (Int_val(Field(camlError, 0))) {
    case BRLAPI_ERROR_NOMEM: result = Val_int(0); break;
    case BRLAPI_ERROR_TTYBUSY: result = Val_int(1); break;
    case BRLAPI_ERROR_DEVICEBUSY: result = Val_int(2); break;
    case BRLAPI_ERROR_UNKNOWN_INSTRUCTION: result = Val_int(3); break;
    case BRLAPI_ERROR_ILLEGAL_INSTRUCTION: result = Val_int(4); break;
    case BRLAPI_ERROR_INVALID_PARAMETER: result = Val_int(5); break;
    case BRLAPI_ERROR_INVALID_PACKET: result = Val_int(6); break;
    case BRLAPI_ERROR_CONNREFUSED: result = Val_int(7); break;
    case BRLAPI_ERROR_OPNOTSUPP: result = Val_int(8); break;
    case BRLAPI_ERROR_GAIERR: {
      result = caml_alloc(1, 0);
      Store_field(result, 0, Val_int(Field(camlError, 2)));
    }; break;
    case BRLAPI_ERROR_LIBCERR: {
      result = caml_alloc(1, 1);
      Store_field(result, 0, unix_error_of_code(Int_val(Field(camlError, 1))));
    }; break;
    case BRLAPI_ERROR_UNKNOWNTTY: result = Val_int(9); break;
    case BRLAPI_ERROR_PROTOCOL_VERSION: result = Val_int(10); break;
    case BRLAPI_ERROR_EOF: result = Val_int(11); break;
    case BRLAPI_ERROR_EMPTYKEY: result = Val_int(12); break; 
    case BRLAPI_ERROR_DRIVERERROR: result = Val_int(13); break;
    case BRLAPI_ERROR_AUTHENTICATION: result = Val_int(14); break;
    default: {
      result = caml_alloc(1, 2);
      Store_field(result, 0, Val_int(Field(camlError, 0)));
    }
  }
  CAMLreturn(result);
}

/* Function : raise_brlapi_error */
/* Raises the Brlapi_error exception */
static void raise_brlapi_error(void)
{
  static value *exception = NULL;
  CAMLlocal1(res);
  if (exception==NULL) exception = caml_named_value("Brlapi_error");
  res = caml_alloc(2,0);
  Store_field(res, 0, *exception);
  Store_field(res, 1, constrCamlError(&brlapi_error));
  caml_raise(res);
}

/* Function : raise_brlapi_exception */
/* Raises Brlapi_exception */
static void BRLAPI_STDCALL raise_brlapi_exception(int err, brlapi_packetType_t type, const void *packet, size_t size)
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
  Store_field(res, 2, caml_copy_int32(type));
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
  if (res<0) raise_brlapi_error();
  s = caml_alloc_tuple(2);
  Store_field(s, 0, caml_copy_string(brlapiSettings.auth));
  Store_field(s, 1, caml_copy_string(brlapiSettings.host));
  pair = caml_alloc_tuple(2);
  Store_field(pair, 0, Val_int(res));
  Store_field(pair, 1, s);
  CAMLreturn(pair);
}

CAMLprim value brlapiml_openConnectionWithHandle(value settings)
{
  CAMLparam1(settings);
  CAMLlocal1(handle);
  brlapi_connectionSettings_t brlapiSettings;
  brlapiSettings.auth = String_val(Field(settings, 0));
  brlapiSettings.host = String_val(Field(settings, 1));
  handle = caml_alloc_custom(&customOperations, brlapi_getHandleSize(), 0, 1);
  if (brlapi__openConnection(Data_custom_val(handle), &brlapiSettings, &brlapiSettings)<0) raise_brlapi_error();
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
  CAMLparam3(handle, ttyPathCaml, driverName);
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

CAMLprim value brlapiml_write(value handle, value writeArguments)
{
  CAMLparam2(handle, writeArguments);
  int andSize = Wosize_val(Field(writeArguments, 4));
  int orSize = Wosize_val(Field(writeArguments, 5));
  unsigned char andMask[andSize], orMask[orSize];
  brlapi_writeArguments_t wa;
  wa.displayNumber = Val_int(Field(writeArguments, 0));
  wa.regionBegin = Val_int(Field(writeArguments, 1));
  wa.regionSize = Val_int(Field(writeArguments, 2));
  wa.text = String_val(Field(writeArguments, 3));
  packDots(Field(writeArguments, 4), andMask, andSize);
  wa.andMask = andMask;
  packDots(Field(writeArguments, 5), orMask, orSize);
  wa.orMask = orMask;
  wa.cursor = Val_int(Field(writeArguments, 6));
  wa.charset = String_val(Field(writeArguments, 7));
  brlapiCheckError(write, NULL, &wa);
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

#define brlapi__expandKeyCode(h,x,y) brlapi_expandKeyCode(x,y)

CAMLprim value brlapiml_expandKeyCode(value handle, value camlKeyCode)
{
  CAMLparam2(handle, camlKeyCode);
  CAMLlocal1(result);
  brlapi_expandedKeyCode_t ekc;
  brlapiCheckError(expandKeyCode, NULL, Int64_val(camlKeyCode), &ekc);
  result = caml_alloc_tuple(4);
  Store_field(result, 0, caml_copy_int32(ekc.type));
  Store_field(result, 1, caml_copy_int32(ekc.command));
  Store_field(result, 2, caml_copy_int32(ekc.argument));
  Store_field(result, 2, caml_copy_int32(ekc.flags));
  CAMLreturn(result);
}

CAMLprim value brlapiml_ignoreKeys(value handle, value rt, value camlKeys)
{
  CAMLparam3(handle, rt, camlKeys);
  unsigned int i, size = Wosize_val(camlKeys);
  brlapi_keyCode_t keys[size];
  for (i=0; i<size; i++) keys[i] = Int64_val(Field(camlKeys, i)); 
  brlapiCheckError(ignoreKeys, NULL, Int_val(rt), keys, size);
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_acceptKeys(value handle, value rt, value camlKeys)
{
  CAMLparam3(handle, rt, camlKeys);
  unsigned int i, size = Wosize_val(camlKeys);
  brlapi_keyCode_t keys[size];
  for (i=0; i<size; i++) keys[i] = Int64_val(Field(camlKeys, i)); 
  brlapiCheckError(acceptKeys, NULL, Int_val(rt), keys, size);
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_ignoreAllKeys(value handle, value unit)
{
  CAMLparam2(handle, unit);
  brlapiCheckError(ignoreAllKeys, NULL);
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_acceptAllKeys(value handle, value unit)
{
  CAMLparam2(handle, unit);
  brlapiCheckError(acceptAllKeys, NULL);
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_ignoreKeyRanges(value handle, value camlRanges)
{
  CAMLparam2(handle, camlRanges);
  CAMLlocal1(r);
  unsigned int i, size = Wosize_val(camlRanges);
  brlapi_range_t ranges[size];
  for (i=0; i<size; i++) {
    r = Field(camlRanges, i);
    ranges[i].first = Int64_val(Field(r, 0));
    ranges[i].last = Int64_val(Field(r, 1));
  }
  brlapiCheckError(ignoreKeyRanges, NULL, ranges, size);
  CAMLreturn(Val_unit);
}

CAMLprim value brlapiml_acceptKeyRanges(value handle, value camlRanges)
{
  CAMLparam2(handle, camlRanges);
  CAMLlocal1(r);
  unsigned int i, size = Wosize_val(camlRanges);
  brlapi_range_t ranges[size];
  for (i=0; i<size; i++) {
    r = Field(camlRanges, i);
    ranges[i].first = Int64_val(Field(r, 0));
    ranges[i].last = Int64_val(Field(r, 1));
  }
  brlapiCheckError(acceptKeyRanges, NULL, ranges, size);
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
  int res;
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
  int i, size;
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

CAMLprim value brlapiml_strerror(value camlError)
{
  CAMLparam1(camlError);
  brlapi_error_t error;
  error.brlerrno = Int_val(Field(camlError,0));
  error.libcerrno = Int_val(Field(camlError,1));
  error.gaierrno = Int_val(Field(camlError,2));
  error.errfun = String_val(Field(camlError,3));
  CAMLreturn(caml_copy_string(brlapi_strerror(&error)));
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

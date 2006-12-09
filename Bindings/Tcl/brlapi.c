/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2006 by Dave Mielke <dave@mielke.cc>
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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <tcl.h>
#include "Programs/brldefs.h"

#define BRLAPI_NO_DEPRECATED
#include "Programs/api.h"

#define allocateMemory(size) ((void *)ckalloc((size)))
#define deallocateMemory(address) ckfree((void *)(address))

typedef struct {
  brlapi_connectionSettings_t settings;
  brlapi_handle_t *handle;
  brlapi_fileDescriptor fileDescriptor;
} BrlapiSession;

static void
setIntResult (Tcl_Interp *interp, int value) {
  Tcl_SetIntObj(Tcl_GetObjResult(interp), value);
}

static void
setWideIntResult (Tcl_Interp *interp, Tcl_WideInt value) {
  Tcl_SetWideIntObj(Tcl_GetObjResult(interp), value);
}

static void
setStringResult (Tcl_Interp *interp, const char *string, int length) {
  Tcl_SetStringObj(Tcl_GetObjResult(interp), string, length);
}

static void
setStringsResult (Tcl_Interp *interp, ...) {
  va_list arguments;
  va_start(arguments, interp);
  Tcl_AppendStringsToObjVA(Tcl_GetObjResult(interp), arguments);
  va_end(arguments);
}

static void
setByteArrayResult (Tcl_Interp *interp, const unsigned char *bytes, int count) {
  Tcl_SetByteArrayObj(Tcl_GetObjResult(interp), bytes, count);
}

static void
setBrlapiError (Tcl_Interp *interp) {
  const char *text = brlapi_strerror(&brlapi_error);
  const char *name;
  int number;

  switch (brlapi_error.brlerrno) {
    case BRLERR_LIBCERR:
      name = "LIBC";
      number = brlapi_error.libcerrno;
      break;

    case BRLERR_GAIERR:
      name = "GAI";
      number = brlapi_error.gaierrno;
      break;

    default:
      name = "BRL";
      number = brlapi_error.brlerrno;
      break;
  }

  {
    Tcl_Obj *const elements[] = {
      Tcl_NewStringObj("BrlAPI", -1),
      Tcl_NewStringObj(name, -1),
      Tcl_NewIntObj(number),
      Tcl_NewStringObj(text, -1)
    };
    Tcl_SetObjErrorCode(interp, Tcl_NewListObj(4, elements));
  }

  setStringResult(interp, text, -1);
}

static int
getDisplaySize (
  Tcl_Interp *interp, BrlapiSession *session,
  unsigned int *width, unsigned int *height
) {
  if (brlapi__getDisplaySize(session->handle, width, height) != -1) return TCL_OK;
  setBrlapiError(interp);
  return TCL_ERROR;
}

#define OPTION_HANDLER_RETURN int
#define OPTION_HANDLER_PARAMETERS (Tcl_Interp *interp, Tcl_Obj *const objv[], void *data)
#define OPTION_HANDLER_NAME(command,function,option) optionHandler_##command##_##function##_##option
#define OPTION_HANDLER(command,function,option) \
  static OPTION_HANDLER_RETURN \
  OPTION_HANDLER_NAME(command, function, option) \
  OPTION_HANDLER_PARAMETERS

typedef OPTION_HANDLER_RETURN (*OptionHandler) OPTION_HANDLER_PARAMETERS;

typedef struct {
  const char *name;
  OptionHandler handler;
  int operands;
  const char *help;
} OptionEntry;

static void
makeOptionNames (const OptionEntry *options, const char ***names) {
  if (!*names) {
    const OptionEntry *option = options;
    const char **name;
    while (option->name) ++option;
    *names = name = allocateMemory(((option - options) + 1) * sizeof(*name));
    option = options;
    while (option->name) *name++ = option++->name;
    *name = NULL;
  }
}

static int
processOptions (
  Tcl_Interp *interp, void *data,
  Tcl_Obj *const objv[], int objc, int start,
  const OptionEntry *options, const char ***names
) {
  makeOptionNames(options, names);

  objv += start;
  objc -= start;

  while (objc > 0) {
    int index;
    {
      int result = Tcl_GetIndexFromObj(interp, objv[0], *names, "option", 0, &index);
      if (result != TCL_OK) return result;
    }

    {
      const OptionEntry *option = &options[index];
      int count = option->operands + 1;

      if (count > objc) {
        Tcl_WrongNumArgs(interp, 1, objv, option->help);
        return TCL_ERROR;
      }

      {
        int result = option->handler(interp, objv, data);
        if (result != TCL_OK) return result;
      }

      objv += count;
      objc -= count;
    }
  }

  return TCL_OK;
}

#define BEGIN_OPTIONS { static const OptionEntry optionTable[] = {
#define END_OPTIONS(start) \
  , {.name = NULL} }; \
  static const char **optionNames = NULL; \
  int result = processOptions(interp, &options, objv, objc, (start), optionTable, &optionNames); \
  if (result != TCL_OK) return result; \
}
#define OPTION(command,function,option) \
  .name = "-" #option, .handler = OPTION_HANDLER_NAME(command, function, option)
#define OPERANDS(count,text) \
  .operands = (count), .help = ((count)? (text): NULL)

typedef struct {
  Tcl_Obj *tty;
  const char *driver;
} FunctionData_session_enterTtyMode;

OPTION_HANDLER(session, enterTtyMode, keyCodes) {
  FunctionData_session_enterTtyMode *options = data;
  options->driver = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, enterTtyMode, events) {
  FunctionData_session_enterTtyMode *options = data;
  options->driver = Tcl_GetString(objv[1]);
  return TCL_OK;
}

OPTION_HANDLER(session, enterTtyMode, tty) {
  FunctionData_session_enterTtyMode *options = data;
  Tcl_Obj *obj = objv[1];
  const char *string = Tcl_GetString(obj);

  if (strcmp(string, "default") == 0) {
    options->tty = NULL;
  } else {
    options->tty = obj;
  }

  return TCL_OK;
}

typedef struct {
  brlapi_writeStruct_t arguments;
  int textLength;
  int andLength;
  int orLength;
} FunctionData_session_write;

OPTION_HANDLER(session, write, and) {
  FunctionData_session_write *options = data;
  options->arguments.attrAnd = Tcl_GetByteArrayFromObj(objv[1], &options->andLength);
  if (!options->andLength) options->arguments.attrAnd = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, write, charset) {
  FunctionData_session_write *options = data;
  options->arguments.charset = Tcl_GetString(objv[1]);
  return TCL_OK;
}

OPTION_HANDLER(session, write, cursor) {
  FunctionData_session_write *options = data;
  Tcl_Obj *obj = objv[1];
  const char *string = Tcl_GetString(obj);

  if (strcmp(string, "off") == 0) {
    options->arguments.cursor = 0;
  } else if (strcmp(string, "leave") == 0) {
    options->arguments.cursor = -1;
  } else {
    int number;
    int result = Tcl_GetIntFromObj(interp, obj, &number);
    if (result != TCL_OK) return result;
    if (number < 1) number = 1;
    options->arguments.cursor = number;
  }

  return TCL_OK;
}

OPTION_HANDLER(session, write, display) {
  FunctionData_session_write *options = data;
  Tcl_Obj *obj = objv[1];
  const char *string = Tcl_GetString(obj);

  if (strcmp(string, "default") == 0) {
    options->arguments.displayNumber = -1;
  } else {
    int number;
    int result = Tcl_GetIntFromObj(interp, obj, &number);
    if (result != TCL_OK) return result;
    if (number < 0) number = 0;
    options->arguments.displayNumber = number;
  }

  return TCL_OK;
}

OPTION_HANDLER(session, write, or) {
  FunctionData_session_write *options = data;
  options->arguments.attrOr = Tcl_GetByteArrayFromObj(objv[1], &options->orLength);
  if (!options->orLength) options->arguments.attrOr = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, write, start) {
  FunctionData_session_write *options = data;
  int offset;

  {
    int result = Tcl_GetIntFromObj(interp, objv[1], &offset);
    if (result != TCL_OK) return result;
  }

  if (offset < 0) offset = 0;
  options->arguments.regionBegin = offset;
  return TCL_OK;
}

OPTION_HANDLER(session, write, text) {
  FunctionData_session_write *options = data;
  options->arguments.text = Tcl_GetStringFromObj(objv[1], &options->textLength);
  if (!options->textLength) options->arguments.text = NULL;
  return TCL_OK;
}

static void
endSession (ClientData data) {
  BrlapiSession *session = data;
  brlapi__closeConnection(session->handle);
  deallocateMemory(session->handle);
  deallocateMemory(session);
}

static int
brlapiSessionCommand (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
  static const char *functions[] = {
    "acceptKeyRange",
    "acceptKeySet",
    "closeConnection",
    "enterRawMode",
    "enterTtyMode",
    "getAuth",
    "getDisplaySize",
    "getDriverId",
    "getDriverName",
    "getFileDescriptor",
    "getHost",
    "ignoreKeyRange",
    "ignoreKeySet",
    "leaveRawMode",
    "leaveTtyMode",
    "readKey",
    "recvRaw",
    "resumeDriver",
    "sendRaw",
    "setFocus",
    "suspendDriver",
    "write",
    "writeDots",
    NULL
  };

  typedef enum {
    FCN_acceptKeyRange,
    FCN_acceptKeySet,
    FCN_closeConnection,
    FCN_enterRawMode,
    FCN_enterTtyMode,
    FCN_getAuth,
    FCN_getDisplaySize,
    FCN_getDriverId,
    FCN_getDriverName,
    FCN_getFileDescriptor,
    FCN_getHost,
    FCN_ignoreKeyRange,
    FCN_ignoreKeySet,
    FCN_leaveRawMode,
    FCN_leaveTtyMode,
    FCN_readKey,
    FCN_recvRaw,
    FCN_resumeDriver,
    FCN_sendRaw,
    FCN_setFocus,
    FCN_suspendDriver,
    FCN_write,
    FCN_writeDots
  } Function;

  BrlapiSession *session = data;
  int function;

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "<function> [<arg> ...]");
    return TCL_ERROR;
  }

  {
    int result = Tcl_GetIndexFromObj(interp, objv[1], functions,"function", 0, &function);
    if (result != TCL_OK) return result;
  }

  switch (function) {
    case FCN_getHost: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      setStringResult(interp, session->settings.host, -1);
      return TCL_OK;
    }

    case FCN_getAuth: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      setStringResult(interp, session->settings.auth, -1);
      return TCL_OK;
    }

    case FCN_getFileDescriptor: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      setIntResult(interp, session->fileDescriptor);
      return TCL_OK;
    }

    case FCN_getDriverId: {
      size_t size = 0X10;

      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      while (1) {
        char buffer[size];
        int result = brlapi__getDriverId(session->handle, buffer, size);

        if (result == -1) {
          setBrlapiError(interp);
          return TCL_ERROR;
        }

        if (result <= size) {
          setStringResult(interp, buffer, result-1);
          return TCL_OK;
        }

        size = result;
      }
    }

    case FCN_getDriverName: {
      size_t size = 0X10;

      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      while (1) {
        char buffer[size];
        int result = brlapi__getDriverName(session->handle, buffer, size);

        if (result == -1) {
          setBrlapiError(interp);
          return TCL_ERROR;
        }

        if (result <= size) {
          setStringResult(interp, buffer, result-1);
          return TCL_OK;
        }

        size = result;
      }
    }

    case FCN_getDisplaySize: {
      unsigned int width, height;

      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      {
        int result = getDisplaySize(interp, session, &width, &height);
        if (result != TCL_OK) return result;
      }

      {
        Tcl_Obj *const elements[] = {
          Tcl_NewIntObj(width),
          Tcl_NewIntObj(height)
        };
        Tcl_SetObjResult(interp, Tcl_NewListObj(2, elements));
      }
      return TCL_OK;
    }

    case FCN_enterTtyMode: {
      FunctionData_session_enterTtyMode options = {
        .tty = NULL,
        .driver = NULL
      };

      BEGIN_OPTIONS
        {
          OPTION(session, enterTtyMode, keyCodes),
          OPERANDS(0, "")
        }
        ,
        {
          OPTION(session, enterTtyMode, events),
          OPERANDS(1, "<driver>")
        }
        ,
        {
          OPTION(session, enterTtyMode, tty),
          OPERANDS(1, "{default | <number>}")
        }
      END_OPTIONS(2)

      if (options.tty) {
        Tcl_Obj **elements;
        int count;

        {
          int result = Tcl_ListObjGetElements(interp, options.tty, &count, &elements);
          if (result != TCL_OK) return result;
        }

        if (count) {
          int ttys[count];
          int index;

          for (index=0; index<count; ++index) {
            int result = Tcl_GetIntFromObj(interp, elements[index], &ttys[index]);
            if (result != TCL_OK) return result;
          }

          if (brlapi__enterTtyModeWithPath(session->handle, ttys, count, options.driver) != -1) return TCL_OK;
        } else if (brlapi__enterTtyModeWithPath(session->handle, NULL, 0, options.driver) != -1) {
          return TCL_OK;
        }
      } else {
        int result = brlapi__enterTtyMode(session->handle, -1, options.driver);

        if (result != -1) {
          setIntResult(interp, result);
          return TCL_OK;
        }
      }

      setBrlapiError(interp);
      return TCL_ERROR;
    }

    case FCN_leaveTtyMode: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      if (brlapi__leaveTtyMode(session->handle) != -1) return TCL_OK;
      setBrlapiError(interp);
      return TCL_ERROR;
    }

    case FCN_setFocus: {
      int tty;

      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<ttyNumber>");
        return TCL_ERROR;
      }

      {
        int result = Tcl_GetIntFromObj(interp, objv[2], &tty);
        if (result != TCL_OK) return result;
      }

      if (brlapi__setFocus(session->handle, tty) != -1) return TCL_OK;
      setBrlapiError(interp);
      return TCL_ERROR;
    }

    case FCN_readKey: {
      int wait;

      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<wait>");
        return TCL_ERROR;
      }

      {
        int result = Tcl_GetBooleanFromObj(interp, objv[2], &wait);
        if (result != TCL_OK) return result;
      }

      {
        brlapi_keyCode_t key;
        int result = brlapi__readKey(session->handle, wait, &key);

        if (result == -1) {
          setBrlapiError(interp);
          return TCL_ERROR;
        }

        if (result == 1) setWideIntResult(interp, key);
        return TCL_OK;
      }
    }

    {
      int ignore;

    case FCN_acceptKeyRange:
      ignore = 0;
      goto doKeyRange;

    case FCN_ignoreKeyRange:
      ignore = 1;

    doKeyRange:
      if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "<firstKey> <lastKey>");
        return TCL_ERROR;
      }

      {
        brlapi_keyCode_t keys[2];

        {
          int index;
          for (index=0; index<2; ++index) {
            Tcl_WideInt value;
            int result = Tcl_GetWideIntFromObj(interp, objv[index+2], &value);
            if (result != TCL_OK) return result;
            keys[index] = value;
          }
        }

        {
          int result = ignore? brlapi__ignoreKeyRange(session->handle, keys[0], keys[1]):
                               brlapi__acceptKeyRange(session->handle, keys[0], keys[1]);
          if (result != -1) return TCL_OK;
          setBrlapiError(interp);
          return TCL_ERROR;
        }
      }
    }

    {
      int ignore;

    case FCN_acceptKeySet:
      ignore = 0;
      goto doKeySet;

    case FCN_ignoreKeySet:
      ignore = 1;

    doKeySet:
      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<keyList>");
        return TCL_ERROR;
      }

      {
        Tcl_Obj **elements;
        int count;

        {
          int result = Tcl_ListObjGetElements(interp, objv[2], &count, &elements);
          if (result != TCL_OK) return result;
        }

        if (count) {
          brlapi_keyCode_t keys[count];
          int index;

          for (index=0; index<count; ++index) {
            Tcl_WideInt value;
            int result = Tcl_GetWideIntFromObj(interp, elements[index], &value);
            if (result != TCL_OK) return result;
            keys[index] = value;
          }

          {
            int result = ignore? brlapi__ignoreKeySet(session->handle, keys, count):
                                 brlapi__acceptKeySet(session->handle, keys, count);
            if (result == -1) {
              setBrlapiError(interp);
              return TCL_ERROR;
            }
          }
        }

        return TCL_OK;
      }
    }

    case FCN_write: {
      FunctionData_session_write options = {
        .arguments = BRLAPI_WRITESTRUCT_INITIALIZER
      };

      if (objc < 2) {
        Tcl_WrongNumArgs(interp, 2, objv, "[<option> ...]");
        return TCL_ERROR;
      }

      BEGIN_OPTIONS
        {
          OPTION(session, write, and),
          OPERANDS(1, "<mask>")
        }
        ,
        {
          OPTION(session, write, charset),
          OPERANDS(1, "<name>")
        }
        ,
        {
          OPTION(session, write, cursor),
          OPERANDS(1, "{leave | off | <offset>}")
        }
        ,
        {
          OPTION(session, write, display),
          OPERANDS(1, "{default | <number>}")
        }
        ,
        {
          OPTION(session, write, or),
          OPERANDS(1, "<mask>")
        }
        ,
        {
          OPTION(session, write, start),
          OPERANDS(1, "<offset>")
        }
        ,
        {
          OPTION(session, write, text),
          OPERANDS(1, "<string>")
        }
      END_OPTIONS(2)

      {
        typedef struct {
          const char *name;
          int value;
        } LengthEntry;

        const LengthEntry lengths[] = {
          {.name="text", .value=options.textLength},
          {.name="and", .value=options.andLength},
          {.name="or", .value=options.orLength},
          {.name=NULL}
        };

        const LengthEntry *current = lengths;
        const LengthEntry *first = NULL;

        while (current->name) {
          if (current->value) {
            if (!first) {
              first = current;
            } else if (current->value != first->value) {
              setStringsResult(interp, first->name, "/", current->name, " length mismatch", NULL);
              return TCL_ERROR;
            }
          }

          ++current;
        }

        if (first) options.arguments.regionSize = first->value;
      }

      if (!options.arguments.regionBegin) {
        int size;

        {
          int width, height;
          int result = getDisplaySize(interp, session, &width, &height);
          if (result != TCL_OK) return result;
          size = width * height;
        }

        {
          char text[size+1];
          unsigned char and[size];
          unsigned char or[size];
          int count = options.arguments.regionSize;

          if (count < size) {
            if (options.arguments.attrAnd) {
              memset(and, 0XFF, size);
              memcpy(and, options.arguments.attrAnd, count);
              options.arguments.attrAnd = and;
            }

            if (options.arguments.attrOr) {
              memset(or, 0X00, size);
              memcpy(or, options.arguments.attrOr, count);
              options.arguments.attrOr = or;
            }
          } else {
            count = size;
          }

          if (options.arguments.text) {
            memset(text, ' ', size);
            text[size] = 0;
            memcpy(text, options.arguments.text, count);
            options.arguments.text = text;
          }

          options.arguments.regionSize = 0;
          if (brlapi__write(session->handle, &options.arguments) != -1) return TCL_OK;
        }
      } else if (brlapi__write(session->handle, &options.arguments) != -1) {
        return TCL_OK;
      }

      setBrlapiError(interp);
      return TCL_ERROR;
    }

    case FCN_writeDots: {
      int size;

      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<dots>");
        return TCL_ERROR;
      }

      {
        unsigned int width, height;
        int result = getDisplaySize(interp, session, &width, &height);
        if (result != TCL_OK) return result;
        size = width * height;
      }

      {
        unsigned char buffer[size];
        int count;
        const unsigned char *cells = Tcl_GetByteArrayFromObj(objv[2], &count);

        if (count < size) {
          memcpy(buffer, cells, count);
          memset(&buffer[count], 0, size-count);
          cells = buffer;
        }

        if (brlapi__writeDots(session->handle, cells) != -1) return TCL_OK;
        setBrlapiError(interp);
        return TCL_ERROR;
      }
    }

    case FCN_enterRawMode: {
      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<driver>");
        return TCL_ERROR;
      }

      {
        const char *driver = Tcl_GetString(objv[2]);

        if (brlapi__enterRawMode(session->handle, driver) != -1) return TCL_OK;
        setBrlapiError(interp);
        return TCL_ERROR;
      }
    }

    case FCN_leaveRawMode: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      if (brlapi__leaveRawMode(session->handle) != -1) return TCL_OK;
      setBrlapiError(interp);
      return TCL_ERROR;
    }

    case FCN_recvRaw: {
      int size;

      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<maximumLength>");
        return TCL_ERROR;
      }

      {
        int result = Tcl_GetIntFromObj(interp, objv[2], &size);
        if (result != TCL_OK) return result;
      }

      {
        unsigned char buffer[size];
        int result = brlapi__recvRaw(session->handle, buffer, size);

        if (result != -1) {
          setByteArrayResult(interp, buffer, result);
          return TCL_OK;
        }

        setBrlapiError(interp);
        return TCL_ERROR;
      }
    }

    case FCN_sendRaw: {
      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<packet>");
        return TCL_ERROR;
      }

      {
        int count;
        const unsigned char *bytes = Tcl_GetByteArrayFromObj(objv[2], &count);

        if (brlapi__sendRaw(session->handle, bytes, count) != -1) return TCL_OK;
        setBrlapiError(interp);
        return TCL_ERROR;
      }
    }

    case FCN_suspendDriver: {
      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<driver>");
        return TCL_ERROR;
      }

      {
        const char *driver = Tcl_GetString(objv[2]);

        if (brlapi__suspendDriver(session->handle, driver) != -1) return TCL_OK;
        setBrlapiError(interp);
        return TCL_ERROR;
      }
    }

    case FCN_resumeDriver: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      if (brlapi__resumeDriver(session->handle) != -1) return TCL_OK;
      setBrlapiError(interp);
      return TCL_ERROR;
    }

    case FCN_closeConnection: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      Tcl_DeleteCommand(interp, Tcl_GetString(objv[0]));
      return TCL_OK;
    }
  }

  setStringResult(interp, "unimplemented function", -1);
  return TCL_ERROR;
}

typedef struct {
  brlapi_connectionSettings_t settings;
} FunctionData_general_connect;

OPTION_HANDLER(general, openConnection, host) {
  FunctionData_general_connect *options = data;
  options->settings.host = Tcl_GetString(objv[1]);
  return TCL_OK;
}

OPTION_HANDLER(general, openConnection, auth) {
  FunctionData_general_connect *options = data;
  options->settings.auth = Tcl_GetString(objv[1]);
  return TCL_OK;
}

static int
brlapiGeneralCommand (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
  static const char *functions[] = {
    "expandHost",
    "expandKeyCode",
    "getHandleSize",
    "getKeyName",
    "makeDots",
    "openConnection",
    NULL
  };

  typedef enum {
    FCN_expandHost,
    FCN_expandKeyCode,
    FCN_getHandleSize,
    FCN_getKeyName,
    FCN_makeDots,
    FCN_openConnection
  } Function;

  int function;

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "<function> [<arg> ...]");
    return TCL_ERROR;
  }

  {
    int result = Tcl_GetIndexFromObj(interp, objv[1], functions, "function", 0, &function);
    if (result != TCL_OK) return result;
  }

  switch (function) {
    case FCN_openConnection: {
      FunctionData_general_connect options = {
        .settings = BRLAPI_SETTINGS_INITIALIZER
      };

      BEGIN_OPTIONS
        {
          OPTION(general, openConnection, host),
          OPERANDS(1, "<hostSpec>")
        }
        ,
        {
          OPTION(general, openConnection, auth),
          OPERANDS(1, "<file>")
        }
      END_OPTIONS(2)

      {
        BrlapiSession *session = allocateMemory(sizeof(*session));
        session->handle = allocateMemory(brlapi_getHandleSize());
        int result = brlapi__openConnection(session->handle, &options.settings, &session->settings);
        if (result != -1) {
          session->fileDescriptor = result;

          {
            static unsigned int suffix = 0;
            char name[0X20];
            snprintf(name, sizeof(name), "brlapi%u", suffix++);
            Tcl_CreateObjCommand(interp, name, brlapiSessionCommand, session, endSession);
            setStringResult(interp, name, -1);
          }

          return TCL_OK;
        } else {
          setBrlapiError(interp);
        }

        deallocateMemory(session->handle);
        deallocateMemory(session);
      }
      return TCL_ERROR;
    }

    case FCN_expandHost: {
      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<hostSpec>");
        return TCL_ERROR;
      }

      {
        char *name;
        char *port;
        int family = brlapi_expandHost(Tcl_GetString(objv[2]), &name, &port);
        Tcl_Obj *elements[] = {
          Tcl_NewStringObj(name, -1),
          Tcl_NewStringObj(port, -1),
          Tcl_NewIntObj(family)
        };

        Tcl_SetObjResult(interp, Tcl_NewListObj(3, elements));
        return TCL_OK;
      }
    }

#define STRING(string) Tcl_NewStringObj((string), -1)
#define FLAG(name,string) if (flagsField & BRLAPI_KEY_FLG_##name) Tcl_ListObjAppendElement(interp, flagsObject, STRING((string)))
    case FCN_expandKeyCode: {
      Tcl_WideInt keyCode;
      brlapi_expandedKeyCode_t ekc;

      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<keyCode>");
        return TCL_ERROR;
      }

      {
        int result = Tcl_GetWideIntFromObj(interp, objv[2], &keyCode);
        if (result != TCL_OK) return result;
      }

      if (brlapi_expandKeyCode(keyCode, &ekc) == -1) {
        setBrlapiError(interp);
        return TCL_ERROR;
      }

      {
        Tcl_Obj *elements[] = {
          Tcl_NewIntObj(ekc.type),
          Tcl_NewIntObj(ekc.command),
          Tcl_NewIntObj(ekc.argument),
          Tcl_NewIntObj(ekc.flags)
        };
        Tcl_SetObjResult(interp, Tcl_NewListObj(4, elements));
        return TCL_OK;
      }
    }

    case FCN_getKeyName: {
      int command, argument;

      if ((objc < 3) || (objc > 4)) {
        Tcl_WrongNumArgs(interp, 2, objv, "<command> [<argument>]");
        return TCL_ERROR;
      }

      {
        int result = Tcl_GetIntFromObj(interp, objv[2], &command);
        if (result != TCL_OK) return result;
      }

      if (objc < 4) {
        argument = 0;
      } else {
        int result = Tcl_GetIntFromObj(interp, objv[3], &argument);
        if (result != TCL_OK) return result;
      }

      {
        const char *name = brlapi_getKeyName(command, argument);

        if (!name) {
          setBrlapiError(interp);
          return TCL_ERROR;
        }

        setStringResult(interp, name, -1);
        return TCL_OK;
      }
    }

    case FCN_makeDots: {
      Tcl_Obj **elements;
      int elementCount;

      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<numbersList>");
        return TCL_ERROR;
      }

      {
        int result = Tcl_ListObjGetElements(interp, objv[2], &elementCount, &elements);
        if (result != TCL_OK) return result;
      }

      if (elementCount) {
        unsigned char cells[elementCount];
        int elementIndex;

        for (elementIndex=0; elementIndex<elementCount; ++elementIndex) {
          unsigned char *cell = &cells[elementIndex];
          Tcl_Obj *element = elements[elementIndex];
          int numberCount;
          const char *numbers = Tcl_GetStringFromObj(element, &numberCount);

          *cell = 0;
          if ((numberCount != 1) || (numbers[0] != '0')) {
            int numberIndex;
            for (numberIndex=0; numberIndex<numberCount; ++numberIndex) {
              unsigned char number = numbers[numberIndex];
              unsigned char bit = brlapi_dotNumberToBit(number);

              if (!bit) {
                setStringResult(interp, "invalid dot number", -1);
                return TCL_ERROR;
              }

              if (*cell & bit) {
                setStringResult(interp, "duplicate dot number", -1);
                return TCL_ERROR;
              }

              *cell |= bit;
            }
          }
        }

        setByteArrayResult(interp, cells, elementCount);
      }

      return TCL_OK;
    }

    case FCN_getHandleSize: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      setIntResult(interp, brlapi_getHandleSize());
      return TCL_OK;
    }
  }

  setStringResult(interp, "unimplemented function", -1);
  return TCL_ERROR;
}

int
Brlapi_tcl_Init (Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "brlapi", brlapiGeneralCommand, NULL, NULL);
  Tcl_PkgProvide(interp, "Brlapi", BRLAPI_RELEASE);
  return TCL_OK;
}

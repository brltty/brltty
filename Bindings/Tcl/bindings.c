/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2007 by Dave Mielke <dave@mielke.cc>
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
#include "Programs/iso_11548-1.h"

#define BRLAPI_NO_DEPRECATED
#define BRLAPI_NO_SINGLE_SESSION
#include "Programs/brlapi.h"

#define allocateMemory(size) ((void *)ckalloc((size)))
#define deallocateMemory(address) ckfree((void *)(address))

#define SET_ARRAY_ELEMENT(element, object) \
do { \
  const char *name = (element); \
  Tcl_Obj *value = (object); \
  Tcl_Obj *result; \
  Tcl_IncrRefCount(value); \
  result = Tcl_SetVar2Ex(interp, array, name, value, TCL_LEAVE_ERR_MSG); \
  Tcl_DecrRefCount(value); \
  if (!result) return TCL_ERROR; \
} while (0)

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
    case BRLAPI_ERROR_LIBCERR:
      name = "LIBC";
      number = brlapi_error.libcerrno;
      break;

    case BRLAPI_ERROR_GAIERR:
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

static int
parseCursorOperand (Tcl_Interp *interp, Tcl_Obj *obj, int *cursor) {
  const char *string = Tcl_GetString(obj);

  if (strcmp(string, "off") == 0) {
    *cursor = BRLAPI_CURSOR_OFF;
  } else if (strcmp(string, "leave") == 0) {
    *cursor = BRLAPI_CURSOR_LEAVE;
  } else {
    int number;
    int result = Tcl_GetIntFromObj(interp, obj, &number);
    if (result != TCL_OK) return result;
    if (number < 1) number = 1;
    *cursor = number;
  }

  return TCL_OK;
}

typedef struct {
  int tty;
  const char *driver;
} FunctionData_session_enterTtyMode;

OPTION_HANDLER(session, enterTtyMode, events) {
  FunctionData_session_enterTtyMode *options = data;
  options->driver = Tcl_GetString(objv[1]);
  return TCL_OK;
}

OPTION_HANDLER(session, enterTtyMode, keyCodes) {
  FunctionData_session_enterTtyMode *options = data;
  options->driver = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, enterTtyMode, tty) {
  FunctionData_session_enterTtyMode *options = data;
  Tcl_Obj *obj = objv[1];
  const char *string = Tcl_GetString(obj);

  if (strcmp(string, "default") == 0) {
    options->tty = BRLAPI_TTY_DEFAULT;
  } else {
    int result = Tcl_GetIntFromObj(interp, obj, &options->tty);
    if (result != TCL_OK) return result;
  }

  return TCL_OK;
}

typedef struct {
  Tcl_Obj *path;
  const char *driver;
} FunctionData_session_enterTtyModeWithPath;

OPTION_HANDLER(session, enterTtyModeWithPath, events) {
  FunctionData_session_enterTtyModeWithPath *options = data;
  options->driver = Tcl_GetString(objv[1]);
  return TCL_OK;
}

OPTION_HANDLER(session, enterTtyModeWithPath, keyCodes) {
  FunctionData_session_enterTtyModeWithPath *options = data;
  options->driver = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, enterTtyModeWithPath, path) {
  FunctionData_session_enterTtyModeWithPath *options = data;
  options->path = objv[1];
  return TCL_OK;
}

typedef struct {
  brlapi_writeArguments_t arguments;
  Tcl_Obj *textObject;
  int textLength;
  int andLength;
  int orLength;
} FunctionData_session_write;

OPTION_HANDLER(session, write, andMask) {
  FunctionData_session_write *options = data;
  options->arguments.andMask = Tcl_GetByteArrayFromObj(objv[1], &options->andLength);
  if (!options->andLength) options->arguments.andMask = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, write, begin) {
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

OPTION_HANDLER(session, write, cursor) {
  FunctionData_session_write *options = data;
  return parseCursorOperand(interp, objv[1], &options->arguments.cursor);
}

OPTION_HANDLER(session, write, displayNumber) {
  FunctionData_session_write *options = data;
  Tcl_Obj *obj = objv[1];
  const char *string = Tcl_GetString(obj);

  if (strcmp(string, "default") == 0) {
    options->arguments.displayNumber = BRLAPI_DISPLAY_DEFAULT;
  } else {
    int number;
    int result = Tcl_GetIntFromObj(interp, obj, &number);
    if (result != TCL_OK) return result;
    if (number < 0) number = 0;
    options->arguments.displayNumber = number;
  }

  return TCL_OK;
}

OPTION_HANDLER(session, write, orMask) {
  FunctionData_session_write *options = data;
  options->arguments.orMask = Tcl_GetByteArrayFromObj(objv[1], &options->orLength);
  if (!options->orLength) options->arguments.orMask = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, write, text) {
  FunctionData_session_write *options = data;
  options->textObject = objv[1];
  options->textLength = Tcl_GetCharLength(options->textObject);
  if (!options->textLength) options->textObject = NULL;
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
    "acceptKeyRanges",
    "acceptKeys",
    "closeConnection",
    "enterRawMode",
    "enterTtyMode",
    "enterTtyModeWithPath",
    "getAuth",
    "getDisplaySize",
    "getDriverName",
    "getFileDescriptor",
    "getHost",
    "ignoreKeyRanges",
    "ignoreKeys",
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
    FCN_acceptKeyRanges,
    FCN_acceptKeys,
    FCN_closeConnection,
    FCN_enterRawMode,
    FCN_enterTtyMode,
    FCN_enterTtyModeWithPath,
    FCN_getAuth,
    FCN_getDisplaySize,
    FCN_getDriverName,
    FCN_getFileDescriptor,
    FCN_getHost,
    FCN_ignoreKeyRanges,
    FCN_ignoreKeys,
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
        .tty = BRLAPI_TTY_DEFAULT,
        .driver = NULL
      };

      BEGIN_OPTIONS
        {
          OPTION(session, enterTtyMode, events),
          OPERANDS(1, "<driver>")
        }
        ,
        {
          OPTION(session, enterTtyMode, keyCodes),
          OPERANDS(0, "")
        }
        ,
        {
          OPTION(session, enterTtyMode, tty),
          OPERANDS(1, "{default | <number>}")
        }
      END_OPTIONS(2)

      {
        int result = brlapi__enterTtyMode(session->handle, options.tty, options.driver);

        if (result == -1) {
          setBrlapiError(interp);
          return TCL_ERROR;
        }

        setIntResult(interp, result);
        return TCL_OK;
      }
    }

    case FCN_enterTtyModeWithPath: {
      FunctionData_session_enterTtyModeWithPath options = {
        .path = NULL,
        .driver = NULL
      };
      Tcl_Obj **elements;
      int count;

      BEGIN_OPTIONS
        {
          OPTION(session, enterTtyModeWithPath, events),
          OPERANDS(1, "<driver>")
        }
        ,
        {
          OPTION(session, enterTtyModeWithPath, keyCodes),
          OPERANDS(0, "")
        }
        ,
        {
          OPTION(session, enterTtyModeWithPath, path),
          OPERANDS(1, "<list>")
        }
      END_OPTIONS(2)

      if (options.path) {
        int result = Tcl_ListObjGetElements(interp, options.path, &count, &elements);
        if (result != TCL_OK) return result;
      } else {
        count = 0;
      }

      if (count) {
        int path[count];
        int index;

        for (index=0; index<count; ++index) {
          int result = Tcl_GetIntFromObj(interp, elements[index], &path[index]);
          if (result != TCL_OK) return result;
        }

        if (brlapi__enterTtyModeWithPath(session->handle, path, count, options.driver) != -1) return TCL_OK;
      } else if (brlapi__enterTtyModeWithPath(session->handle, NULL, 0, options.driver) != -1) {
        return TCL_OK;
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
      brlapi_rangeType_t rangeType;
      Tcl_Obj *codeList;

    case FCN_acceptKeys:
      ignore = 0;
      goto doKeys;

    case FCN_ignoreKeys:
      ignore = 1;

    doKeys:
      if ((objc < 3) || (objc > 4)) {
        Tcl_WrongNumArgs(interp, 2, objv, "<rangeType> [<keyCodeList>]");
        return TCL_ERROR;
      }

      {
        static const char *rangeNames[] = {
          "all",
          "code",
          "command",
          "key",
          "type",
          NULL
        };

        static const brlapi_rangeType_t rangeTypes[] = {
          brlapi_rangeType_all,
          brlapi_rangeType_code,
          brlapi_rangeType_command,
          brlapi_rangeType_key,
          brlapi_rangeType_type
        };

        int rangeIndex;
        int result = Tcl_GetIndexFromObj(interp, objv[2], rangeNames, "range type", 0, &rangeIndex);
        if (result != TCL_OK) return result;
        rangeType = rangeTypes[rangeIndex];
      }

      if (objc < 4) {
        codeList = NULL;
      } else {
        codeList = objv[3];
      }

      if (rangeType != brlapi_rangeType_all) {
        Tcl_Obj **codeElements;
        int codeCount;

        if (!codeList) {
          setStringResult(interp, "no key code list", -1);
          return TCL_ERROR;
        }

        {
          int result = Tcl_ListObjGetElements(interp, codeList, &codeCount, &codeElements);
          if (result != TCL_OK) return result;
        }

        if (codeCount) {
          brlapi_keyCode_t codes[codeCount];
          int codeIndex;

          for (codeIndex=0; codeIndex<codeCount; ++codeIndex) {
            Tcl_WideInt code;
            int result = Tcl_GetWideIntFromObj(interp, codeElements[codeIndex], &code);
            if (result != TCL_OK) return result;
            codes[codeIndex] = code;
          }

          {
            int result = ignore? brlapi__ignoreKeys(session->handle, rangeType, codes, codeCount):
                                 brlapi__acceptKeys(session->handle, rangeType, codes, codeCount);
            if (result != -1) return TCL_OK;
            setBrlapiError(interp);
            return TCL_ERROR;
          }
        }
      } else if (codeList) {
        setStringResult(interp, "unexpected key code list", -1);
        return TCL_ERROR;
      }

      {
        int result = ignore? brlapi__ignoreKeys(session->handle, rangeType, NULL, 0):
                             brlapi__acceptKeys(session->handle, rangeType, NULL, 0);
        if (result != -1) return TCL_OK;
        setBrlapiError(interp);
        return TCL_ERROR;
      }
    }

    {
      int ignore;
      Tcl_Obj **rangeElements;
      int rangeCount;

    case FCN_acceptKeyRanges:
      ignore = 0;
      goto doKeyRanges;

    case FCN_ignoreKeyRanges:
      ignore = 1;

    doKeyRanges:
      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<keyRangeList>");
        return TCL_ERROR;
      }

      {
        int result = Tcl_ListObjGetElements(interp, objv[2], &rangeCount, &rangeElements);
        if (result != TCL_OK) return result;
      }

      if (rangeCount) {
        brlapi_range_t ranges[rangeCount];
        int rangeIndex;

        for (rangeIndex=0; rangeIndex<rangeCount; ++rangeIndex) {
          brlapi_range_t *range = &ranges[rangeIndex];
          Tcl_Obj **codeElements;
          int codeCount;

          {
            int result = Tcl_ListObjGetElements(interp, rangeElements[rangeIndex], &codeCount, &codeElements);
            if (result != TCL_OK) return result;
          }

          if (codeCount != 2) {
            setStringResult(interp, "key range element is not a two-element list", -1);
            return TCL_ERROR;
          }

          {
            Tcl_WideInt codes[codeCount];
            int codeIndex;

            for (codeIndex=0; codeIndex<codeCount; ++codeIndex) {
              int result = Tcl_GetWideIntFromObj(interp, codeElements[codeIndex], &codes[codeIndex]);
              if (result != TCL_OK) return result;
            }

            range->first = codes[0];
            range->last = codes[1];
          }
        }

        {
          int result = ignore? brlapi__ignoreKeyRanges(session->handle, ranges, rangeCount):
                               brlapi__acceptKeyRanges(session->handle, ranges, rangeCount);
          if (result != -1) return TCL_OK;
          setBrlapiError(interp);
          return TCL_ERROR;
        }
      }

      {
        int result = ignore? brlapi__ignoreKeyRanges(session->handle, NULL, 0):
                             brlapi__acceptKeyRanges(session->handle, NULL, 0);
        if (result != -1) return TCL_OK;
        setBrlapiError(interp);
        return TCL_ERROR;
      }
    }

    case FCN_write: {
      FunctionData_session_write options = {
        .arguments = BRLAPI_WRITEARGUMENTS_INITIALIZER
      };

      if (objc < 2) {
        Tcl_WrongNumArgs(interp, 2, objv, "[<option> ...]");
        return TCL_ERROR;
      }

      BEGIN_OPTIONS
        {
          OPTION(session, write, andMask),
          OPERANDS(1, "<dots>")
        }
        ,
        {
          OPTION(session, write, begin),
          OPERANDS(1, "<offset>")
        }
        ,
        {
          OPTION(session, write, cursor),
          OPERANDS(1, "{leave | off | <offset>}")
        }
        ,
        {
          OPTION(session, write, displayNumber),
          OPERANDS(1, "{default | <number>}")
        }
        ,
        {
          OPTION(session, write, orMask),
          OPERANDS(1, "<dots>")
        }
        ,
        {
          OPTION(session, write, text),
          OPERANDS(1, "<string>")
        }
      END_OPTIONS(2)

      if (options.textObject) options.arguments.charset = "UTF-8";

      {
        typedef struct {
          const char *name;
          int value;
        } LengthEntry;

        const LengthEntry lengths[] = {
          {.name="text", .value=options.textLength},
          {.name="andMask", .value=options.andLength},
          {.name="orMask", .value=options.orLength},
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
          unsigned int width, height;
          int result = getDisplaySize(interp, session, &width, &height);
          if (result != TCL_OK) return result;
          size = width * height;
        }

        {
          unsigned char andMask[size];
          unsigned char orMask[size];

          if (options.arguments.regionSize < size) {
            if (options.arguments.andMask) {
              memset(andMask, 0XFF, size);
              memcpy(andMask, options.arguments.andMask, options.arguments.regionSize);
              options.arguments.andMask = andMask;
            }

            if (options.arguments.orMask) {
              memset(orMask, 0X00, size);
              memcpy(orMask, options.arguments.orMask, options.arguments.regionSize);
              options.arguments.orMask = orMask;
            }

            if (options.textObject) {
              if (Tcl_IsShared(options.textObject)) {
                if (!(options.textObject = Tcl_DuplicateObj(options.textObject))) {
                  setBrlapiError(interp);
                  return TCL_ERROR;
                }
              }

              do {
                Tcl_AppendToObj(options.textObject, " ", -1);
              } while (Tcl_GetCharLength(options.textObject) < size);
            }
          } else if (options.arguments.regionSize > size) {
            if (options.textObject) {
              if (!(options.textObject = Tcl_GetRange(options.textObject, 0, options.arguments.regionSize-1))) {
                setBrlapiError(interp);
                return TCL_ERROR;
              }
            }
          }

          options.arguments.regionSize = 0;
          if (options.textObject) {
            int length;
            options.arguments.text = Tcl_GetStringFromObj(options.textObject, &length);
          }

          if (brlapi__write(session->handle, &options.arguments) != -1) return TCL_OK;
        }
      } else {
        if (options.textObject) {
          int length;
          options.arguments.text = Tcl_GetStringFromObj(options.textObject, &length);
        }

        if (brlapi__write(session->handle, &options.arguments) != -1) return TCL_OK;
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

OPTION_HANDLER(general, openConnection, auth) {
  FunctionData_general_connect *options = data;
  options->settings.auth = Tcl_GetString(objv[1]);
  return TCL_OK;
}

OPTION_HANDLER(general, openConnection, host) {
  FunctionData_general_connect *options = data;
  options->settings.host = Tcl_GetString(objv[1]);
  return TCL_OK;
}

static int
brlapiGeneralCommand (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
  static const char *functions[] = {
    "describeKeyCode",
    "expandKeyCode",
    "getHandleSize",
    "makeDots",
    "openConnection",
    NULL
  };

  typedef enum {
    FCN_describeKeyCode,
    FCN_expandKeyCode,
    FCN_getHandleSize,
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
          OPTION(general, openConnection, auth),
          OPERANDS(1, "<file>")
        }
        ,
        {
          OPTION(general, openConnection, host),
          OPERANDS(1, "<hostSpec>")
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

    case FCN_expandKeyCode: {
      Tcl_WideInt keyCode;
      brlapi_expandedKeyCode_t ekc;
      const char *array;

      if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "<keyCode> <arrayName>");
        return TCL_ERROR;
      }

      {
        int result = Tcl_GetWideIntFromObj(interp, objv[2], &keyCode);
        if (result != TCL_OK) return result;
      }

      if (!(array = Tcl_GetString(objv[3]))) return TCL_ERROR;

      if (brlapi_expandKeyCode(keyCode, &ekc) == -1) {
        setBrlapiError(interp);
        return TCL_ERROR;
      }

      SET_ARRAY_ELEMENT("type", Tcl_NewIntObj(ekc.type));
      SET_ARRAY_ELEMENT("command", Tcl_NewIntObj(ekc.command));
      SET_ARRAY_ELEMENT("argument", Tcl_NewIntObj(ekc.argument));
      SET_ARRAY_ELEMENT("flags", Tcl_NewIntObj(ekc.flags));
      return TCL_OK;
    }

    case FCN_describeKeyCode: {
      Tcl_WideInt keyCode;
      brlapi_describedKeyCode_t dkc;
      const char *array;

      if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "<keyCode> <arrayName>");
        return TCL_ERROR;
      }

      {
        int result = Tcl_GetWideIntFromObj(interp, objv[2], &keyCode);
        if (result != TCL_OK) return result;
      }

      if (!(array = Tcl_GetString(objv[3]))) return TCL_ERROR;

      if (brlapi_describeKeyCode(keyCode, &dkc) == -1) {
        setBrlapiError(interp);
        return TCL_ERROR;
      }

      SET_ARRAY_ELEMENT("type", Tcl_NewStringObj(dkc.type, -1));
      SET_ARRAY_ELEMENT("command", Tcl_NewStringObj(dkc.command, -1));
      SET_ARRAY_ELEMENT("argument", Tcl_NewIntObj(dkc.argument));

      {
        Tcl_Obj *flags = Tcl_NewListObj(0, NULL);
        SET_ARRAY_ELEMENT("flags", flags);

        {
          int index;
          for (index=0; index<dkc.flags; ++index) {
            Tcl_ListObjAppendElement(interp, flags, Tcl_NewStringObj(dkc.flag[index], -1));
          }
        }
      }

      return TCL_OK;
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
        BrlDots cells[elementCount];
        int elementIndex;

        for (elementIndex=0; elementIndex<elementCount; ++elementIndex) {
          BrlDots *cell = &cells[elementIndex];
          Tcl_Obj *element = elements[elementIndex];
          int numberCount;
          const char *numbers = Tcl_GetStringFromObj(element, &numberCount);

          *cell = 0;
          if ((numberCount != 1) || (numbers[0] != '0')) {
            int numberIndex;
            for (numberIndex=0; numberIndex<numberCount; ++numberIndex) {
              char number = numbers[numberIndex];
              BrlDots dot = brlNumberToDot(number);

              if (!dot) {
                setStringResult(interp, "invalid dot number", -1);
                return TCL_ERROR;
              }

              if (*cell & dot) {
                setStringResult(interp, "duplicate dot number", -1);
                return TCL_ERROR;
              }

              *cell |= dot;
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

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
#include "Programs/api.h"

#define allocateMemory(size) ((void *)ckalloc((size)))
#define deallocateMemory(address) ckfree((void *)(address))

typedef struct {
  brlapi_settings_t settings;
  int fileDescriptor;
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
  .name = #option, .handler = OPTION_HANDLER_NAME(command, function, option)
#define OPERANDS(count,text) \
  .operands = (count), .help = ((count)? (text): NULL)

typedef struct {
  Tcl_Obj *tty;
  const char *driver;
} FunctionData_session_claimTty;

OPTION_HANDLER(session, claimTty, commands) {
  FunctionData_session_claimTty *options = data;
  options->driver = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, claimTty, events) {
  FunctionData_session_claimTty *options = data;
  options->driver = Tcl_GetString(objv[1]);
  return TCL_OK;
}

OPTION_HANDLER(session, claimTty, tty) {
  FunctionData_session_claimTty *options = data;
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
  brlapi_writeStruct arguments;
} FunctionData_session_writeText;

OPTION_HANDLER(session, writeText, and) {
  FunctionData_session_writeText *options = data;
  int count;
  char *mask = Tcl_GetByteArrayFromObj(objv[1], &count);

  if (!count) {
    options->arguments.attrAnd = NULL;
  } else if (count == options->arguments.regionSize) {
    options->arguments.attrAnd = mask;
  } else {
    setStringResult(interp, "wrong and mask length", -1);
    return TCL_ERROR;
  }

  return TCL_OK;
}

OPTION_HANDLER(session, writeText, charset) {
  FunctionData_session_writeText *options = data;
  options->arguments.charset = Tcl_GetString(objv[1]);
  return TCL_OK;
}

OPTION_HANDLER(session, writeText, cursor) {
  FunctionData_session_writeText *options = data;
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
    if (number < 0) number = 0;
    options->arguments.cursor = number + 1;
  }

  return TCL_OK;
}

OPTION_HANDLER(session, writeText, display) {
  FunctionData_session_writeText *options = data;
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

OPTION_HANDLER(session, writeText, or) {
  FunctionData_session_writeText *options = data;
  int count;
  char *mask = Tcl_GetByteArrayFromObj(objv[1], &count);

  if (!count) {
    options->arguments.attrOr = NULL;
  } else if (count == options->arguments.regionSize) {
    options->arguments.attrOr = mask;
  } else {
    setStringResult(interp, "wrong or mask length", -1);
    return TCL_ERROR;
  }

  return TCL_OK;
}

OPTION_HANDLER(session, writeText, start) {
  FunctionData_session_writeText *options = data;
  int offset;

  {
    int result = Tcl_GetIntFromObj(interp, objv[1], &offset);
    if (result != TCL_OK) return result;
  }

  if (offset < 0) offset = 0;
  options->arguments.regionBegin = offset + 1;
  return TCL_OK;
}

static void
endSession (ClientData data) {
  BrlapiSession *session = data;
  brlapi_closeConnection();
  deallocateMemory(session);
}

static int
brlapiSessionCommand (data, interp, objc, objv)
  ClientData data;
  Tcl_Interp *interp;
  int objc;
  Tcl_Obj *const objv[];
{
  static const char *functions[] = {
    "acceptKeys",
    "claimTty",
    "disconnect",
    "displaySize",
    "driverIdentifier",
    "driverName",
    "enterRawMode",
    "fileDescriptor",
    "host",
    "ignoreKeys",
    "keyFile",
    "leaveRawMode",
    "readKey",
    "readRaw",
    "releaseTty",
    "resume",
    "setFocus",
    "suspend",
    "writeCells",
    "writeRaw",
    "writeText",
    NULL
  };

  typedef enum {
    FCN_acceptKeys,
    FCN_claimTty,
    FCN_disconnect,
    FCN_displaySize,
    FCN_driverIdentifier,
    FCN_driverName,
    FCN_enterRawMode,
    FCN_fileDescriptor,
    FCN_host,
    FCN_ignoreKeys,
    FCN_keyFile,
    FCN_leaveRawMode,
    FCN_readKey,
    FCN_readRaw,
    FCN_releaseTty,
    FCN_resume,
    FCN_setFocus,
    FCN_suspend,
    FCN_writeCells,
    FCN_writeRaw,
    FCN_writeText
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
    case FCN_host: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      setStringResult(interp, session->settings.hostName, -1);
      return TCL_OK;
    }

    case FCN_keyFile: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      setStringResult(interp, session->settings.authKey, -1);
      return TCL_OK;
    }

    case FCN_fileDescriptor: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      setIntResult(interp, session->fileDescriptor);
      return TCL_OK;
    }

    case FCN_driverIdentifier: {
      size_t size = 0X10;

      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      while (1) {
        char buffer[size];
        int result = brlapi_getDriverId(buffer, size);

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

    case FCN_driverName: {
      size_t size = 0X10;

      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      while (1) {
        char buffer[size];
        int result = brlapi_getDriverName(buffer, size);

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

    case FCN_displaySize: {
      int width, height;

      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      if (brlapi_getDisplaySize(&width, &height) == -1) {
        setBrlapiError(interp);
        return TCL_ERROR;
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

    case FCN_claimTty: {
      FunctionData_session_claimTty options = {
        .tty = NULL,
        .driver = NULL
      };

      BEGIN_OPTIONS
        {
          OPTION(session, claimTty, commands),
          OPERANDS(0, "")
        }
        ,
        {
          OPTION(session, claimTty, events),
          OPERANDS(1, "<driver>")
        }
        ,
        {
          OPTION(session, claimTty, tty),
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

          if (brlapi_getTtyPath(ttys, count, options.driver) != -1) return TCL_OK;
        } else if (brlapi_getTtyPath(NULL, 0, options.driver) != -1) {
          return TCL_OK;
        }
      } else {
        int result = brlapi_getTty(-1, options.driver);

        if (result != -1) {
          setIntResult(interp, result);
          return TCL_OK;
        }
      }

      setBrlapiError(interp);
      return TCL_ERROR;
    }

    case FCN_releaseTty: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      if (brlapi_leaveTty() != -1) return TCL_OK;
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

      if (brlapi_setFocus(tty) != -1) return TCL_OK;
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
        brl_keycode_t key;
        int result = brlapi_readKey(wait, &key);

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

    case FCN_acceptKeys:
      ignore = 0;
      goto doIgnoreKeys;

    case FCN_ignoreKeys:
      ignore = 1;

    doIgnoreKeys:
      if (objc == 3) {
        Tcl_Obj **elements;
        int count;

        {
          int result = Tcl_ListObjGetElements(interp, objv[2], &count, &elements);
          if (result != TCL_OK) return result;
        }

        if (count) {
          brl_keycode_t keys[count];
          int index;

          for (index=0; index<count; ++index) {
            Tcl_WideInt value;
            int result = Tcl_GetWideIntFromObj(interp, elements[index], &value);
            if (result != TCL_OK) return result;
            keys[index] = value;
          }

          {
            int result = ignore? brlapi_ignoreKeySet(keys, count):
                                 brlapi_unignoreKeySet(keys, count);
            if (result == -1) {
              setBrlapiError(interp);
              return TCL_ERROR;
            }
          }
        }

        return TCL_OK;
      }

      if (objc == 4) {
        brl_keycode_t keys[2];

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
          int result = ignore? brlapi_ignoreKeyRange(keys[0], keys[1]):
                               brlapi_unignoreKeyRange(keys[0], keys[1]);
          if (result != -1) return TCL_OK;
          setBrlapiError(interp);
          return TCL_ERROR;
        }
      }

      Tcl_WrongNumArgs(interp, 2, objv, "{<keyList> | <firstKey> <lastKey>}");
      return TCL_ERROR;
    }

    case FCN_writeText: {
      FunctionData_session_writeText options = {
        .arguments = BRLAPI_WRITESTRUCT_INITIALIZER,
        .arguments.regionBegin = 1
      };

      if (objc < 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<text> [<option> ...]");
        return TCL_ERROR;
      }

      {
        int length;
        char *string = Tcl_GetStringFromObj(objv[2], &length);

        options.arguments.text = length? string: NULL;
        options.arguments.regionSize = length;
      }

      BEGIN_OPTIONS
        {
          OPTION(session, writeText, and),
          OPERANDS(1, "<mask>")
        }
        ,
        {
          OPTION(session, writeText, charset),
          OPERANDS(1, "<name>")
        }
        ,
        {
          OPTION(session, writeText, cursor),
          OPERANDS(1, "{leave | off | <offset>}")
        }
        ,
        {
          OPTION(session, writeText, display),
          OPERANDS(1, "{default | <number>}")
        }
        ,
        {
          OPTION(session, writeText, or),
          OPERANDS(1, "<mask>")
        }
        ,
        {
          OPTION(session, writeText, start),
          OPERANDS(1, "<offset>")
        }
      END_OPTIONS(3)

      if (!options.arguments.regionSize) return TCL_OK;
      if (brlapi_write(&options.arguments) != -1) return TCL_OK;
      setBrlapiError(interp);
      return TCL_ERROR;
    }

    case FCN_writeCells: {
      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<cells>");
        return TCL_ERROR;
      }

      {
        int count;
        const char *cells = Tcl_GetByteArrayFromObj(objv[2], &count);

        if (brlapi_writeDots(cells) != -1) return TCL_OK;
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

        if (brlapi_getRaw(driver) != -1) return TCL_OK;
        setBrlapiError(interp);
        return TCL_ERROR;
      }
    }

    case FCN_leaveRawMode: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      if (brlapi_leaveRaw() != -1) return TCL_OK;
      setBrlapiError(interp);
      return TCL_ERROR;
    }

    case FCN_readRaw: {
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
        int result = brlapi_recvRaw(buffer, size);

        if (result != -1) {
          setByteArrayResult(interp, buffer, result);
          return TCL_OK;
        }

        setBrlapiError(interp);
        return TCL_ERROR;
      }
    }

    case FCN_writeRaw: {
      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<packet>");
        return TCL_ERROR;
      }

      {
        int count;
        const char *bytes = Tcl_GetByteArrayFromObj(objv[2], &count);

        if (brlapi_sendRaw(bytes, count) != -1) return TCL_OK;
        setBrlapiError(interp);
        return TCL_ERROR;
      }
    }

    case FCN_suspend: {
      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<driver>");
        return TCL_ERROR;
      }

      {
        const char *driver = Tcl_GetString(objv[2]);

        if (brlapi_suspend(driver) != -1) return TCL_OK;
        setBrlapiError(interp);
        return TCL_ERROR;
      }
    }

    case FCN_resume: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      if (brlapi_resume() != -1) return TCL_OK;
      setBrlapiError(interp);
      return TCL_ERROR;
    }

    case FCN_disconnect: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      Tcl_DeleteCommand(interp, Tcl_GetString(objv[0]));
      return TCL_OK;
    }
  }
}

typedef struct {
  brlapi_settings_t settings;
} FunctionData_general_connect;

OPTION_HANDLER(general, connect, host) {
  FunctionData_general_connect *options = data;
  options->settings.hostName = Tcl_GetString(objv[1]);
  return TCL_OK;
}

OPTION_HANDLER(general, connect, keyFile) {
  FunctionData_general_connect *options = data;
  options->settings.authKey = Tcl_GetString(objv[1]);
  return TCL_OK;
}

static int
brlapiGeneralCommand (data, interp, objc, objv)
  ClientData data;
  Tcl_Interp *interp;
  int objc;
  Tcl_Obj *const objv[];
{
  static const char *functions[] = {
    "connect",
    "expandCommand",
    "expandHost",
    "makeCells",
    NULL
  };

  typedef enum {
    FCN_connect,
    FCN_expandCommand,
    FCN_expandHost,
    FCN_makeCells
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
    case FCN_connect: {
      FunctionData_general_connect options = {
        .settings = BRLAPI_SETTINGS_INITIALIZER
      };

      BEGIN_OPTIONS
        {
          OPTION(general, connect, host),
          OPERANDS(1, "<hostSpec>")
        }
        ,
        {
          OPTION(general, connect, keyFile),
          OPERANDS(1, "<file>")
        }
      END_OPTIONS(2)

      {
        BrlapiSession *session = allocateMemory(sizeof(*session));
        int result = brlapi_initializeConnection(&options.settings, &session->settings);
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
        int family = brlapi_splitHost(Tcl_GetString(objv[2]), &name, &port);
        Tcl_Obj *elements[] = {
          Tcl_NewStringObj(name, -1),
          Tcl_NewStringObj(port, -1),
          Tcl_NewIntObj(family)
        };

        Tcl_SetObjResult(interp, Tcl_NewListObj(3, elements));
        return TCL_OK;
      }
    }

    case FCN_expandCommand: {
      Tcl_WideInt key;

      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<key>");
        return TCL_ERROR;
      }

      {
        int result = Tcl_GetWideIntFromObj(interp, objv[2], &key);
        if (result != TCL_OK) return result;
      }

      {
        Tcl_Obj *elements[] = {
          Tcl_NewWideIntObj((key & BRLAPI_KEY_TYPE_MASK) >> BRLAPI_KEY_TYPE_SHIFT),
          Tcl_NewWideIntObj((key & BRLAPI_KEY_CODE_MASK) >> BRLAPI_KEY_CODE_SHIFT),
          Tcl_NewWideIntObj((key & BRLAPI_KEY_FLAGS_MASK) >> BRLAPI_KEY_FLAGS_SHIFT)
        };

        Tcl_SetObjResult(interp, Tcl_NewListObj(3, elements));
        return TCL_OK;
      }
    }

    case FCN_makeCells: {
      Tcl_Obj **elements;
      int elementCount;

      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "<dotsList>");
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
          int dotCount;
          const char *dots = Tcl_GetStringFromObj(element, &dotCount);

          *cell = 0;
          if ((dotCount != 1) || (dots[0] != '0')) {
            int dotIndex;
            for (dotIndex=0; dotIndex<dotCount; ++dotIndex) {
              unsigned char dotNumber = dots[dotIndex];
              static const char dotNumbers[] = {'1', '2', '3', '4', '5', '6', '7', '8'};
              const char *dot = memchr(dotNumbers, dotNumber, sizeof(dotNumbers));

              if (!dot) {
                setStringResult(interp, "invalid dot number", -1);
                return TCL_ERROR;
              }

              {
                static const unsigned char dotBits[] = {
                  BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4,
                  BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8
                };
                unsigned char dotBit = dotBits[dot - dotNumbers];

                if (*cell & dotBit) {
                  setStringResult(interp, "duplicate dot number", -1);
                  return TCL_ERROR;
                }

                *cell |= dotBit;
              }
            }
          }
        }

        setByteArrayResult(interp, cells, elementCount);
      }

      return TCL_OK;
    }
  }
}

int
Brlapi_Init (Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "brlapi", brlapiGeneralCommand, NULL, NULL);
  return TCL_OK;
}

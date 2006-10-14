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

#include <tcl.h>
#include "Programs/api.h"

#define allocateMemory(size) ((void *)ckalloc((size)))
#define deallocateMemory(address) ckfree((void *)(address))

typedef struct {
  brlapi_settings_t settings;
  int fileDescriptor;
} BrlapiSession;

#define OPTION_HANDLER_RETURN int
#define OPTION_HANDLER_PARAMETERS (Tcl_Interp *interp, Tcl_Obj *const objv[], void *data)
#define OPTION_HANDLER_NAME(function,option) optionHandler_##function##option
#define OPTION_HANDLER(function,option) static OPTION_HANDLER_RETURN OPTION_HANDLER_NAME(function,option) OPTION_HANDLER_PARAMETERS

typedef OPTION_HANDLER_RETURN (*OptionHandler) OPTION_HANDLER_PARAMETERS;

typedef struct {
  const char *name;
  OptionHandler handler;
  int operands;
  const char *help;
} OptionEntry;

static void
setIntResult (Tcl_Interp *interp, int value) {
  Tcl_SetIntObj(Tcl_GetObjResult(interp), value);
}

static void
setStringResult (Tcl_Interp *interp, const char *string, int length) {
  Tcl_SetStringObj(Tcl_GetObjResult(interp), string, length);
}

static void
setBrlapiError (Tcl_Interp *interp) {
  setStringResult(interp, brlapi_strerror(&brlapi_error), -1);
}

static int
processOptions (
  Tcl_Interp *interp, void *data,
  Tcl_Obj *const objv[], int objc, int skip,
  const OptionEntry *options, const char ***names
) {
  if (!*names) {
    const OptionEntry *option = options;
    const char **name;
    while (option->name) ++option;
    *names = name = allocateMemory(((option - options) + 1) * sizeof(*name));
    option = options;
    while (option->name) *name++ = option++->name;
    *name = NULL;
  }

  objv += skip;
  objc -= skip;

  while (objc > 0) {
    int index;
    if (Tcl_GetIndexFromObj(interp, objv[0], *names, "option", 0, &index) != TCL_OK) return TCL_ERROR;

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

typedef struct {
  Tcl_Obj *tty;
  const char *how;
} FunctionData_getTty;

OPTION_HANDLER(getTty,commands) {
  FunctionData_getTty *getTty = data;
  getTty->how = NULL;
  return TCL_OK;
}

OPTION_HANDLER(getTty,raw) {
  FunctionData_getTty *getTty = data;
  getTty->how = Tcl_GetString(objv[1]);
  return TCL_OK;
}

OPTION_HANDLER(getTty,tty) {
  FunctionData_getTty *getTty = data;
  Tcl_Obj *obj = objv[1];
  const char *string = Tcl_GetString(obj);

  if (strcmp(string, "control") == 0) {
    getTty->tty = NULL;
  } else {
    getTty->tty = obj;
  }

  return TCL_OK;
}

typedef struct {
  brlapi_writeStruct arguments;
} FunctionData_writeText;

OPTION_HANDLER(writeText,and) {
  FunctionData_writeText *writeText = data;
  int count;
  char *cells = Tcl_GetByteArrayFromObj(objv[1], &count);

  if (!count) {
    writeText->arguments.attrAnd = NULL;
  } else if (count == writeText->arguments.regionSize) {
    writeText->arguments.attrAnd = cells;
  } else {
    setStringResult(interp, "wrong and mask length", -1);
    return TCL_ERROR;
  }

  return TCL_OK;
}

OPTION_HANDLER(writeText,charset) {
  FunctionData_writeText *writeText = data;
  writeText->arguments.charset = Tcl_GetString(objv[1]);
  return TCL_OK;
}

OPTION_HANDLER(writeText,cursor) {
  FunctionData_writeText *writeText = data;
  Tcl_Obj *obj = objv[1];
  const char *string = Tcl_GetString(obj);

  if (strcmp(string, "off") == 0) {
    writeText->arguments.cursor = 0;
  } else if (strcmp(string, "leave") == 0) {
    writeText->arguments.cursor = -1;
  } else {
    int number;
    int result = Tcl_GetIntFromObj(interp, obj, &number);
    if (result != TCL_OK) return result;
    if (number < 0) number = 0;
    writeText->arguments.cursor = number + 1;
  }

  return TCL_OK;
}

OPTION_HANDLER(writeText,display) {
  FunctionData_writeText *writeText = data;
  Tcl_Obj *obj = objv[1];
  const char *string = Tcl_GetString(obj);

  if (strcmp(string, "default") == 0) {
    writeText->arguments.displayNumber = -1;
  } else {
    int number;
    int result = Tcl_GetIntFromObj(interp, obj, &number);
    if (result != TCL_OK) return result;
    if (number < 0) number = 0;
    writeText->arguments.displayNumber = number;
  }

  return TCL_OK;
}

OPTION_HANDLER(writeText,or) {
  FunctionData_writeText *writeText = data;
  int count;
  char *cells = Tcl_GetByteArrayFromObj(objv[1], &count);

  if (!count) {
    writeText->arguments.attrOr = NULL;
  } else if (count == writeText->arguments.regionSize) {
    writeText->arguments.attrOr = cells;
  } else {
    setStringResult(interp, "wrong or mask length", -1);
    return TCL_ERROR;
  }

  return TCL_OK;
}

OPTION_HANDLER(writeText,start) {
  FunctionData_writeText *writeText = data;
  int offset;

  {
    int result = Tcl_GetIntFromObj(interp, objv[1], &offset);
    if (result != TCL_OK) return result;
  }

  if (offset < 0) offset = 0;
  writeText->arguments.regionBegin = offset + 1;
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
    "disconnect",
    "displaySize",
    "driverCode",
    "driverName",
    "fileDescriptor",
    "getTty",
    "host",
    "key",
    "leaveTty",
    "setFocus",
    "writeDots",
    "writeText",
    NULL
  };

  typedef enum {
    FCN_disconnect,
    FCN_displaySize,
    FCN_driverCode,
    FCN_driverName,
    FCN_fileDescriptor,
    FCN_getTty,
    FCN_host,
    FCN_key,
    FCN_leaveTty,
    FCN_setFocus,
    FCN_writeDots,
    FCN_writeText
  } Function;

  BrlapiSession *session = data;
  int function;

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "function ?arg ...?");
    return TCL_ERROR;
  }

  if (Tcl_GetIndexFromObj(interp, objv[1], functions,
                          "function", 0, &function) != TCL_OK)
    return TCL_ERROR;
  switch (function) {
    case FCN_disconnect: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      Tcl_DeleteCommand(interp, Tcl_GetString(objv[0]));
      return TCL_OK;
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

    case FCN_driverCode: {
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

    case FCN_fileDescriptor: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      setIntResult(interp, session->fileDescriptor);
      return TCL_OK;
    }

    case FCN_getTty: {
      FunctionData_getTty getTty = {
        .tty = NULL,
        .how = NULL
      };

      {
        static const OptionEntry options[] = {
          {
            .name = "commands",
            .operands = 0,
            .help = NULL,
            .handler = OPTION_HANDLER_NAME(getTty,commands)
          }
          ,
          {
            .name = "raw",
            .operands = 1,
            .help = "driver",
            .handler = OPTION_HANDLER_NAME(getTty,raw)
          }
          ,
          {
            .name = "tty",
            .operands = 1,
            .help = "{number | control}",
            .handler = OPTION_HANDLER_NAME(getTty,tty)
          }
          ,
          {
            .name = NULL
          }
        };
        static const char **names = NULL;
        int result = processOptions(interp, &getTty, objv, objc, 2, options, &names);
        if (result != TCL_OK) return result;
      }

      if (getTty.tty) {
        Tcl_Obj **elements;
        int count;

        {
          int result = Tcl_ListObjGetElements(interp, getTty.tty, &count, &elements);
          if (result != TCL_OK) return result;
        }

        if (count) {
          int ttys[count];
          int index;

          for (index=0; index<count; ++index) {
            int result = Tcl_GetIntFromObj(interp, elements[index], &ttys[index]);
            if (result != TCL_OK) return result;
          }

          if (brlapi_getTtyPath(ttys, count, getTty.how) != -1) return TCL_OK;
        } else if (brlapi_getTtyPath(NULL, 0, getTty.how) != -1) {
          return TCL_OK;
        }
      } else {
        int result = brlapi_getTty(-1, getTty.how);

        if (result != -1) {
          setIntResult(interp, result);
          return TCL_OK;
        }
      }

      setBrlapiError(interp);
      return TCL_ERROR;
    }

    case FCN_host: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      setStringResult(interp, session->settings.hostName, -1);
      return TCL_OK;
    }

    case FCN_key: {
      if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
      }

      setStringResult(interp, session->settings.authKey, -1);
      return TCL_OK;
    }

    case FCN_leaveTty: {
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
        Tcl_WrongNumArgs(interp, 2, objv, "tty");
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

    case FCN_writeDots: {
      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "cells");
        return TCL_ERROR;
      }

      {
        int count;
        const char *bytes = Tcl_GetByteArrayFromObj(objv[2], &count);

        if (brlapi_writeDots(bytes) != -1) return TCL_OK;
        setBrlapiError(interp);
        return TCL_ERROR;
      }
    }

    case FCN_writeText: {
      FunctionData_writeText writeText = {
        .arguments = BRLAPI_WRITESTRUCT_INITIALIZER,
        .arguments.regionBegin = 1
      };

      if (objc < 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "text ?option ...?");
        return TCL_ERROR;
      }

      {
        int length;
        char *string = Tcl_GetStringFromObj(objv[2], &length);

        writeText.arguments.text = length? string: NULL;
        writeText.arguments.regionSize = length;
      }

      {
        static const OptionEntry options[] = {
          {
            .name = "and",
            .operands = 1,
            .help = "cells",
            .handler = OPTION_HANDLER_NAME(writeText,and)
          }
          ,
          {
            .name = "charset",
            .operands = 1,
            .help = "name",
            .handler = OPTION_HANDLER_NAME(writeText,charset)
          }
          ,
          {
            .name = "cursor",
            .operands = 1,
            .help = "{number | off | leave}",
            .handler = OPTION_HANDLER_NAME(writeText,cursor)
          }
          ,
          {
            .name = "display",
            .operands = 1,
            .help = "{number | default}",
            .handler = OPTION_HANDLER_NAME(writeText,display)
          }
          ,
          {
            .name = "or",
            .operands = 1,
            .help = "cells",
            .handler = OPTION_HANDLER_NAME(writeText,or)
          }
          ,
          {
            .name = "start",
            .operands = 1,
            .help = "offset",
            .handler = OPTION_HANDLER_NAME(writeText,start)
          }
          ,
          {
            .name = NULL
          }
        };
        static const char **names = NULL;
        int result = processOptions(interp, &writeText, objv, objc, 3, options, &names);
        if (result != TCL_OK) return result;
      }

      if (brlapi_write(&writeText.arguments) != -1) return TCL_OK;
      setBrlapiError(interp);
      return TCL_ERROR;
    }
  }
}

typedef struct {
  brlapi_settings_t settings;
} FunctionData_connect;

OPTION_HANDLER(connect,host) {
  FunctionData_connect *connect = data;
  connect->settings.hostName = Tcl_GetString(objv[1]);
  return TCL_OK;
}

OPTION_HANDLER(connect,key) {
  FunctionData_connect *connect = data;
  connect->settings.authKey = Tcl_GetString(objv[1]);
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
    "parseHost",
    NULL
  };

  typedef enum {
    FCN_connect,
    FCN_parseHost
  } Function;

  int function;

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "function ?arg ...?");
    return TCL_ERROR;
  }

  if (Tcl_GetIndexFromObj(interp, objv[1], functions,
                          "function", 0, &function) != TCL_OK)
    return TCL_ERROR;
  switch (function) {
    case FCN_connect: {
      FunctionData_connect connect = {
        .settings = {
          .authKey = NULL,
          .hostName = NULL
        }
      };

      {
        static const OptionEntry options[] = {
          {
            .name = "host",
            .operands = 1,
            .help = "[host][:port]",
            .handler = OPTION_HANDLER_NAME(connect,host)
          }
          ,
          {
            .name = "key",
            .operands = 1,
            .help = "file",
            .handler = OPTION_HANDLER_NAME(connect,key)
          }
          ,
          {
            .name = NULL
          }
        };
        static const char **names = NULL;
        int result = processOptions(interp, &connect, objv, objc, 2, options, &names);
        if (result != TCL_OK) return result;
      }

      {
        BrlapiSession *session = allocateMemory(sizeof(*session));
        int result = brlapi_initializeConnection(&connect.settings, &session->settings);
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

    case FCN_parseHost: {
      if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "host");
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
  }
}

int
Brlapi_Init (Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "brlapi", brlapiGeneralCommand, NULL, NULL);
  return TCL_OK;
}

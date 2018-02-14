/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2018 by Dave Mielke <dave@mielke.cc>
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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BRLAPI_NO_DEPRECATED
#define BRLAPI_NO_SINGLE_SESSION
#include "brlapi.h"

#include <tcl.h>
#include "brl_dots.h"

#define allocateMemory(size) ((void *)ckalloc((size)))
#define deallocateMemory(address) ckfree((void *)(address))

#define TEST_TCL_OK(expression) \
do { \
  int result = (expression); \
  if (result != TCL_OK) return result; \
} while (0)

static int
setArrayElement (Tcl_Interp *interp, const char *array, const char *element, Tcl_Obj *value) {
  if (!value) return TCL_ERROR;
  Tcl_IncrRefCount(value);
  Tcl_Obj *result = Tcl_SetVar2Ex(interp, array, element, value, TCL_LEAVE_ERR_MSG);
  Tcl_DecrRefCount(value);
  return result? TCL_OK: TCL_ERROR;
}

#define SET_ARRAY_ELEMENT(element, object) \
  TEST_TCL_OK(setArrayElement(interp, array, element, object))

typedef struct {
  brlapi_connectionSettings_t settings;
  brlapi_handle_t *handle;
  brlapi_fileDescriptor fileDescriptor;

  unsigned int displayWidth;
  unsigned int displayHeight;
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
  Tcl_ResetResult(interp);

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

  Tcl_Obj *result = Tcl_GetObjResult(interp);
  Tcl_SetStringObj(result, "BrlAPI error: ", -1);
  size_t length = strlen(text);

  while (length > 0) {
    size_t end = length - 1;
    if (text[end] != '\n') break;
    length = end;
  }

  Tcl_AppendToObj(result, text, length);
}

static int
getDisplaySize (Tcl_Interp *interp, BrlapiSession *session) {
  int result = brlapi__getDisplaySize(
    session->handle,
    &session->displayWidth,
    &session->displayHeight
  );

  if (result != -1) return TCL_OK;
  setBrlapiError(interp);
  return TCL_ERROR;
}

static int
getCellCount (
  Tcl_Interp *interp, BrlapiSession *session, unsigned int *count
) {
  if (!(session->displayWidth && session->displayHeight)) {
    TEST_TCL_OK(getDisplaySize(interp, session));
  }

  *count = session->displayWidth * session->displayHeight;
  return TCL_OK;
}

#define FUNCTION_HANDLER_RETURN int
#define FUNCTION_HANDLER_PARAMETERS (Tcl_Interp *interp, Tcl_Obj *const objv[], int objc, void *data)
#define FUNCTION_HANDLER_NAME(command,function) functionHandler_##command##_##function
#define FUNCTION_HANDLER(command,function) \
  static FUNCTION_HANDLER_RETURN \
  FUNCTION_HANDLER_NAME(command, function) \
  FUNCTION_HANDLER_PARAMETERS
typedef FUNCTION_HANDLER_RETURN (*FunctionHandler) FUNCTION_HANDLER_PARAMETERS;

typedef struct {
  const char *name;
  FunctionHandler handler;
} FunctionEntry;

#define FUNCTION(command,function) \
  { .name=#function, .handler = FUNCTION_HANDLER_NAME(command, function) }

#define BEGIN_FUNCTIONS static const FunctionEntry functions[] = {
#define END_FUNCTIONS { .name=NULL }};

static int
testArgumentCount (
  Tcl_Interp *interp, Tcl_Obj *const objv[], int objc,
  int start, int required, int optional,
  const char *syntax
) {
  int minimum = start + required;
  int maximum = (optional < 0)? objc: (minimum + optional);

  if ((objc >= minimum) && (objc <= maximum)) return TCL_OK;
  Tcl_WrongNumArgs(interp, start, objv, syntax);
  return TCL_ERROR;
}

#define TEST_ARGUMENT_COUNT(start,required,optional,syntax) \
  TEST_TCL_OK(testArgumentCount(interp, objv, objc, (start), (required), (optional), (syntax)))

#define TEST_COMMAND_ARGUMENTS() \
  TEST_ARGUMENT_COUNT(1, 1, -1, "<function> [<arg> ...]")

#define TEST_FUNCTION_ARGUMENT() \
  int functionIndex; \
  TEST_TCL_OK(Tcl_GetIndexFromObjStruct(interp, objv[1], functions, sizeof(functions[0]), "function", 0, &functionIndex)); \
  const FunctionEntry *function = &functions[functionIndex];

#define TEST_FUNCTION_ARGUMENTS(required,optional,syntax) \
  TEST_ARGUMENT_COUNT(2, (required), (optional), (syntax))

#define TEST_FUNCTION_NO_ARGUMENTS() TEST_FUNCTION_ARGUMENTS(0, 0, NULL)

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

static int
processOptions (
  Tcl_Interp *interp, void *data,
  Tcl_Obj *const objv[], int objc, int start,
  const OptionEntry *options
) {
  objv += start;
  objc -= start;

  while (objc > 0) {
    int index;
    TEST_TCL_OK(Tcl_GetIndexFromObjStruct(interp, objv[0], options, sizeof(*options), "option", 0, &index));
    const OptionEntry *option = &options[index];

    int count = option->operands;
    TEST_ARGUMENT_COUNT(1, count, -1, option->help);
    TEST_TCL_OK(option->handler(interp, objv, data));

    count += 1;
    objv += count;
    objc -= count;
  }

  return TCL_OK;
}

#define BEGIN_OPTIONS { static const OptionEntry optionTable[] = {
#define END_OPTIONS(start) \
  , {.name = NULL} }; \
  TEST_TCL_OK(processOptions(interp, &options, objv, objc, (start), optionTable)); \
}
#define OPTION(command,function,option) \
  .name = "-" #option, .handler = OPTION_HANDLER_NAME(command, function, option)
#define OPERANDS(count,text) \
  .operands = (count), .help = ((count)? (text): NULL)

static int
parseCursorOperand (Tcl_Interp *interp, Tcl_Obj *obj, int *cursor) {
  const char *string = Tcl_GetString(obj);
  if (!string) return TCL_ERROR;

  if (strcmp(string, "off") == 0) {
    *cursor = BRLAPI_CURSOR_OFF;
  } else if (strcmp(string, "leave") == 0) {
    *cursor = BRLAPI_CURSOR_LEAVE;
  } else {
    int number;
    TEST_TCL_OK(Tcl_GetIntFromObj(interp, obj, &number));

    if (number < 1) number = 1;
    *cursor = number;
  }

  return TCL_OK;
}

static int
getSessionStringProperty (
  Tcl_Interp *interp, BrlapiSession *session,
  int BRLAPI_STDCALL (*getProperty) (brlapi_handle_t *handle, char *buffer, size_t size)
) {
  size_t size = 0X10;

  while (1) {
    char buffer[size];
    int result = getProperty(session->handle, buffer, size);

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

FUNCTION_HANDLER(session, closeConnection) {
//BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();

  const char *name = Tcl_GetString(objv[0]);
  if (!name) return TCL_ERROR;

  Tcl_DeleteCommand(interp, name);
  return TCL_OK;
}

FUNCTION_HANDLER(session, enterRawMode) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<driver>");

  const char *driver = Tcl_GetString(objv[2]);
  if (!driver) return TCL_ERROR;

  if (brlapi__enterRawMode(session->handle, driver) != -1) return TCL_OK;
  setBrlapiError(interp);
  return TCL_ERROR;
}

typedef struct {
  int tty;
  const char *driver;
} EnterTtyModeOptions;

OPTION_HANDLER(session, enterTtyMode, events) {
  EnterTtyModeOptions *options = data;
  return (options->driver = Tcl_GetString(objv[1]))? TCL_OK: TCL_ERROR;
}

OPTION_HANDLER(session, enterTtyMode, keyCodes) {
  EnterTtyModeOptions *options = data;
  options->driver = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, enterTtyMode, tty) {
  EnterTtyModeOptions *options = data;
  Tcl_Obj *obj = objv[1];

  const char *string = Tcl_GetString(obj);
  if (!string) return TCL_ERROR;

  if (strcmp(string, "default") == 0) {
    options->tty = BRLAPI_TTY_DEFAULT;
  } else {
    TEST_TCL_OK(Tcl_GetIntFromObj(interp, obj, &options->tty));
  }

  return TCL_OK;
}

FUNCTION_HANDLER(session, enterTtyMode) {
  BrlapiSession *session = data;

  EnterTtyModeOptions options = {
    .tty = BRLAPI_TTY_DEFAULT,
    .driver = NULL
  };

  BEGIN_OPTIONS
    { OPTION(session, enterTtyMode, events),
      OPERANDS(1, "<driver>")
    },

    { OPTION(session, enterTtyMode, keyCodes),
      OPERANDS(0, "")
    },

    { OPTION(session, enterTtyMode, tty),
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

typedef struct {
  Tcl_Obj *path;
  const char *driver;
} EnterTtyModeWithPathOptions;

OPTION_HANDLER(session, enterTtyModeWithPath, events) {
  EnterTtyModeWithPathOptions *options = data;
  return (options->driver = Tcl_GetString(objv[1]))? TCL_OK: TCL_ERROR;
}

OPTION_HANDLER(session, enterTtyModeWithPath, keyCodes) {
  EnterTtyModeWithPathOptions *options = data;
  options->driver = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, enterTtyModeWithPath, path) {
  EnterTtyModeWithPathOptions *options = data;
  options->path = objv[1];
  return TCL_OK;
}

FUNCTION_HANDLER(session, enterTtyModeWithPath) {
  BrlapiSession *session = data;

  EnterTtyModeWithPathOptions options = {
    .path = NULL,
    .driver = NULL
  };

  BEGIN_OPTIONS
    { OPTION(session, enterTtyModeWithPath, events),
      OPERANDS(1, "<driver>")
    },

    { OPTION(session, enterTtyModeWithPath, keyCodes),
      OPERANDS(0, "")
    },

    { OPTION(session, enterTtyModeWithPath, path),
      OPERANDS(1, "<list>")
    }
  END_OPTIONS(2)

  Tcl_Obj **elements;
  int count;

  if (options.path) {
    TEST_TCL_OK(Tcl_ListObjGetElements(interp, options.path, &count, &elements));
  } else {
    elements = NULL;
    count = 0;
  }

  if (count) {
    int path[count];

    for (int index=0; index<count; index+=1) {
      TEST_TCL_OK(Tcl_GetIntFromObj(interp, elements[index], &path[index]));
    }

    if (brlapi__enterTtyModeWithPath(session->handle, path, count, options.driver) != -1) return TCL_OK;
  } else if (brlapi__enterTtyModeWithPath(session->handle, NULL, 0, options.driver) != -1) {
    return TCL_OK;
  }

  setBrlapiError(interp);
  return TCL_ERROR;
}

FUNCTION_HANDLER(session, getAuth) {
  BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();

  setStringResult(interp, session->settings.auth, -1);
  return TCL_OK;
}

FUNCTION_HANDLER(session, getDisplaySize) {
  BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();
  TEST_TCL_OK(getDisplaySize(interp, session));

  Tcl_Obj *width = Tcl_NewIntObj(session->displayWidth);
  if (!width) return TCL_ERROR;

  Tcl_Obj *height = Tcl_NewIntObj(session->displayHeight);
  if (!height) return TCL_ERROR;

  Tcl_Obj *const elements[] = {width, height};
  Tcl_Obj *list = Tcl_NewListObj(2, elements);
  if (!list) return TCL_ERROR;

  Tcl_SetObjResult(interp, list);
  return TCL_OK;
}

FUNCTION_HANDLER(session, getDriverName) {
  BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();
  return getSessionStringProperty(interp, session, brlapi__getDriverName);
}

FUNCTION_HANDLER(session, getFileDescriptor) {
  BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();

  setIntResult(interp, session->fileDescriptor);
  return TCL_OK;
}

FUNCTION_HANDLER(session, getHost) {
  BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();

  setStringResult(interp, session->settings.host, -1);
  return TCL_OK;
}

FUNCTION_HANDLER(session, getModelIdentifier) {
  BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();
  return getSessionStringProperty(interp, session, brlapi__getModelIdentifier);
}

FUNCTION_HANDLER(session, leaveRawMode) {
  BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();

  if (brlapi__leaveRawMode(session->handle) != -1) return TCL_OK;
  setBrlapiError(interp);
  return TCL_ERROR;
}

FUNCTION_HANDLER(session, leaveTtyMode) {
  BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();

  if (brlapi__leaveTtyMode(session->handle) != -1) return TCL_OK;
  setBrlapiError(interp);
  return TCL_ERROR;
}

FUNCTION_HANDLER(session, readKey) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<wait>");

  int wait;
  TEST_TCL_OK(Tcl_GetBooleanFromObj(interp, objv[2], &wait));

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

FUNCTION_HANDLER(session, recvRaw) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<maximumLength>");

  int size;
  TEST_TCL_OK(Tcl_GetIntFromObj(interp, objv[2], &size));

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

FUNCTION_HANDLER(session, resumeDriver) {
  BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();

  if (brlapi__resumeDriver(session->handle) != -1) return TCL_OK;
  setBrlapiError(interp);
  return TCL_ERROR;
}

FUNCTION_HANDLER(session, sendRaw) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<packet>");

  {
    int count;
    const unsigned char *bytes = Tcl_GetByteArrayFromObj(objv[2], &count);

    if (brlapi__sendRaw(session->handle, bytes, count) != -1) return TCL_OK;
    setBrlapiError(interp);
    return TCL_ERROR;
  }
}

FUNCTION_HANDLER(session, setFocus) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<ttyNumber>");

  int tty;
  TEST_TCL_OK(Tcl_GetIntFromObj(interp, objv[2], &tty));

  if (brlapi__setFocus(session->handle, tty) != -1) return TCL_OK;
  setBrlapiError(interp);
  return TCL_ERROR;
}

FUNCTION_HANDLER(session, suspendDriver) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<driver>");

  const char *driver = Tcl_GetString(objv[2]);
  if (!driver) return TCL_ERROR;

  if (brlapi__suspendDriver(session->handle, driver) != -1) return TCL_OK;
  setBrlapiError(interp);
  return TCL_ERROR;
}

typedef struct {
  brlapi_writeArguments_t arguments;
  Tcl_Obj *textObject;
  int textLength;
  int andLength;
  int orLength;
} WriteOptions;

OPTION_HANDLER(session, write, andMask) {
  WriteOptions *options = data;
  options->arguments.andMask = Tcl_GetByteArrayFromObj(objv[1], &options->andLength);
  if (!options->andLength) options->arguments.andMask = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, write, begin) {
  WriteOptions *options = data;
  int offset;
  TEST_TCL_OK(Tcl_GetIntFromObj(interp, objv[1], &offset));

  if (offset < 0) offset = 0;
  options->arguments.regionBegin = offset;
  return TCL_OK;
}

OPTION_HANDLER(session, write, cursor) {
  WriteOptions *options = data;
  return parseCursorOperand(interp, objv[1], &options->arguments.cursor);
}

OPTION_HANDLER(session, write, displayNumber) {
  WriteOptions *options = data;
  Tcl_Obj *obj = objv[1];

  const char *string = Tcl_GetString(obj);
  if (!string) return TCL_ERROR;

  if (strcmp(string, "default") == 0) {
    options->arguments.displayNumber = BRLAPI_DISPLAY_DEFAULT;
  } else {
    int number;
    TEST_TCL_OK(Tcl_GetIntFromObj(interp, obj, &number));

    if (number < 0) number = 0;
    options->arguments.displayNumber = number;
  }

  return TCL_OK;
}

OPTION_HANDLER(session, write, orMask) {
  WriteOptions *options = data;
  options->arguments.orMask = Tcl_GetByteArrayFromObj(objv[1], &options->orLength);
  if (!options->orLength) options->arguments.orMask = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, write, text) {
  WriteOptions *options = data;
  options->textObject = objv[1];
  options->textLength = Tcl_GetCharLength(options->textObject);
  if (!options->textLength) options->textObject = NULL;
  return TCL_OK;
}

FUNCTION_HANDLER(session, write) {
  BrlapiSession *session = data;

  WriteOptions options = {
    .arguments = BRLAPI_WRITEARGUMENTS_INITIALIZER
  };

  BEGIN_OPTIONS
    { OPTION(session, write, andMask),
      OPERANDS(1, "<dots>")
    },

    { OPTION(session, write, begin),
      OPERANDS(1, "<offset>")
    },

    { OPTION(session, write, cursor),
      OPERANDS(1, "{leave | off | <offset>}")
    },

    { OPTION(session, write, displayNumber),
      OPERANDS(1, "{default | <number>}")
    },

    { OPTION(session, write, orMask),
      OPERANDS(1, "<dots>")
    },

    { OPTION(session, write, text),
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

      current += 1;
    }

    if (first) options.arguments.regionSize = first->value;
  }

  if (!options.arguments.regionBegin) {
    unsigned int size;
    TEST_TCL_OK(getCellCount(interp, session, &size));

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

FUNCTION_HANDLER(session, writeDots) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<dots>");

  unsigned int size;
  TEST_TCL_OK(getCellCount(interp, session, &size));

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

static int
changeKeys (
  Tcl_Interp *interp, Tcl_Obj *const objv[], int objc, void *data,
  int BRLAPI_STDCALL (*change) (brlapi_handle_t *handle, brlapi_rangeType_t type, const brlapi_keyCode_t keys[], unsigned int count)
) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 1, "<rangeType> [<keyCodeList>]");

  brlapi_rangeType_t rangeType;
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
    TEST_TCL_OK(Tcl_GetIndexFromObj(interp, objv[2], rangeNames, "range type", 0, &rangeIndex));
    rangeType = rangeTypes[rangeIndex];
  }

  Tcl_Obj *codeList = (objc < 4)? NULL: objv[3];

  if (rangeType != brlapi_rangeType_all) {
    if (!codeList) {
      setStringResult(interp, "no key code list", -1);
      return TCL_ERROR;
    }

    Tcl_Obj **codeElements;
    int codeCount;
    TEST_TCL_OK(Tcl_ListObjGetElements(interp, codeList, &codeCount, &codeElements));

    if (codeCount) {
      brlapi_keyCode_t codes[codeCount];

      for (int codeIndex=0; codeIndex<codeCount; codeIndex+=1) {
        Tcl_WideInt code;
        TEST_TCL_OK(Tcl_GetWideIntFromObj(interp, codeElements[codeIndex], &code));
        codes[codeIndex] = code;
      }

      {
        int result = change(session->handle, rangeType, codes, codeCount);
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
    int result = change(session->handle, rangeType, NULL, 0);
    if (result != -1) return TCL_OK;
                         brlapi__acceptKeys(session->handle, rangeType, NULL, 0);
    setBrlapiError(interp);
    return TCL_ERROR;
  }
}

FUNCTION_HANDLER(session, acceptKeys) {
  return changeKeys(interp, objv, objc, data, brlapi__acceptKeys);
}

FUNCTION_HANDLER(session, ignoreKeys) {
  return changeKeys(interp, objv, objc, data, brlapi__ignoreKeys);
}

static int
changeKeyRanges (
  Tcl_Interp *interp, Tcl_Obj *const objv[], int objc, void *data,
  int BRLAPI_STDCALL (*change) (brlapi_handle_t *handle, const brlapi_range_t ranges[], unsigned int count)
) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<keyRangeList>");

  Tcl_Obj **rangeElements;
  int rangeCount;
  TEST_TCL_OK(Tcl_ListObjGetElements(interp, objv[2], &rangeCount, &rangeElements));

  if (rangeCount) {
    brlapi_range_t ranges[rangeCount];

    for (int rangeIndex=0; rangeIndex<rangeCount; rangeIndex+=1) {
      brlapi_range_t *range = &ranges[rangeIndex];
      Tcl_Obj **codeElements;
      int codeCount;
      TEST_TCL_OK(Tcl_ListObjGetElements(interp, rangeElements[rangeIndex], &codeCount, &codeElements));

      if (codeCount != 2) {
        setStringResult(interp, "key range element is not a two-element list", -1);
        return TCL_ERROR;
      }

      {
        Tcl_WideInt codes[codeCount];

        for (int codeIndex=0; codeIndex<codeCount; codeIndex+=1) {
          TEST_TCL_OK(Tcl_GetWideIntFromObj(interp, codeElements[codeIndex], &codes[codeIndex]));
        }

        range->first = codes[0];
        range->last = codes[1];
      }
    }

    {
      int result = change(session->handle, ranges, rangeCount);
      if (result != -1) return TCL_OK;

      setBrlapiError(interp);
      return TCL_ERROR;
    }
  }

  {
    int result = change(session->handle, NULL, 0);
    if (result != -1) return TCL_OK;

    setBrlapiError(interp);
    return TCL_ERROR;
  }
}

FUNCTION_HANDLER(session, acceptKeyRanges) {
  return changeKeyRanges(interp, objv, objc, data, brlapi__acceptKeyRanges);
}

FUNCTION_HANDLER(session, ignoreKeyRanges) {
  return changeKeyRanges(interp, objv, objc, data, brlapi__ignoreKeyRanges);
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
  BEGIN_FUNCTIONS
    FUNCTION(session, acceptKeyRanges),
    FUNCTION(session, acceptKeys),
    FUNCTION(session, closeConnection),
    FUNCTION(session, enterRawMode),
    FUNCTION(session, enterTtyMode),
    FUNCTION(session, enterTtyModeWithPath),
    FUNCTION(session, getAuth),
    FUNCTION(session, getDisplaySize),
    FUNCTION(session, getDriverName),
    FUNCTION(session, getFileDescriptor),
    FUNCTION(session, getHost),
    FUNCTION(session, getModelIdentifier),
    FUNCTION(session, ignoreKeyRanges),
    FUNCTION(session, ignoreKeys),
    FUNCTION(session, leaveRawMode),
    FUNCTION(session, leaveTtyMode),
    FUNCTION(session, readKey),
    FUNCTION(session, recvRaw),
    FUNCTION(session, resumeDriver),
    FUNCTION(session, sendRaw),
    FUNCTION(session, setFocus),
    FUNCTION(session, suspendDriver),
    FUNCTION(session, write),
    FUNCTION(session, writeDots),
  END_FUNCTIONS

  TEST_COMMAND_ARGUMENTS();
  TEST_FUNCTION_ARGUMENT();
  return function->handler(interp, objv, objc, data);
}

FUNCTION_HANDLER(general, describeKeyCode) {
  TEST_FUNCTION_ARGUMENTS(2, 0, "<keyCode> <arrayName>");

  Tcl_WideInt keyCode;
  TEST_TCL_OK(Tcl_GetWideIntFromObj(interp, objv[2], &keyCode));

  const char *array = Tcl_GetString(objv[3]);
  if (!array) return TCL_ERROR;

  brlapi_describedKeyCode_t dkc;
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

    for (int index=0; index<dkc.flags; index+=1) {
      Tcl_Obj *flag = Tcl_NewStringObj(dkc.flag[index], -1);
      if (!flag) return TCL_ERROR;
      TEST_TCL_OK(Tcl_ListObjAppendElement(interp, flags, flag));
    }
  }

  return TCL_OK;
}

FUNCTION_HANDLER(general, expandKeyCode) {
  TEST_FUNCTION_ARGUMENTS(2, 0, "<keyCode> <arrayName>");

  Tcl_WideInt keyCode;
  TEST_TCL_OK(Tcl_GetWideIntFromObj(interp, objv[2], &keyCode));

  const char *array = Tcl_GetString(objv[3]);
  if (!array) return TCL_ERROR;

  brlapi_expandedKeyCode_t ekc;
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

FUNCTION_HANDLER(general, getHandleSize) {
  TEST_FUNCTION_NO_ARGUMENTS();

  setIntResult(interp, brlapi_getHandleSize());
  return TCL_OK;
}

FUNCTION_HANDLER(general, getVersionString) {
  TEST_FUNCTION_NO_ARGUMENTS();

  setStringResult(interp, BRLAPI_RELEASE, -1);
  return TCL_OK;
}

FUNCTION_HANDLER(general, makeDots) {
  TEST_FUNCTION_ARGUMENTS(1, 0, "<dotNumbersList>");

  Tcl_Obj **elements;
  int elementCount;
  TEST_TCL_OK(Tcl_ListObjGetElements(interp, objv[2], &elementCount, &elements));

  if (elementCount) {
    BrlDots cells[elementCount];

    for (int elementIndex=0; elementIndex<elementCount; elementIndex+=1) {
      Tcl_Obj *element = elements[elementIndex];
      BrlDots *cell = &cells[elementIndex];
      *cell = 0;

      int numberCount;
      const char *numbers = Tcl_GetStringFromObj(element, &numberCount);
      if (!numbers) return TCL_ERROR;

      if ((numberCount != 1) || (numbers[0] != '0')) {
        for (int numberIndex=0; numberIndex<numberCount; numberIndex+=1) {
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
  } else {
    setByteArrayResult(interp, NULL, elementCount);
  }

  return TCL_OK;
}

typedef struct {
  brlapi_connectionSettings_t settings;
} OpenConnectionOptions;

OPTION_HANDLER(general, openConnection, auth) {
  OpenConnectionOptions *options = data;
  return (options->settings.auth = Tcl_GetString(objv[1]))? TCL_OK: TCL_ERROR;
}

OPTION_HANDLER(general, openConnection, host) {
  OpenConnectionOptions *options = data;
  return (options->settings.host = Tcl_GetString(objv[1]))? TCL_OK: TCL_ERROR;
}

FUNCTION_HANDLER(general, openConnection) {
  OpenConnectionOptions options = {
    .settings = BRLAPI_SETTINGS_INITIALIZER
  };

  BEGIN_OPTIONS
    { OPTION(general, openConnection, auth),
      OPERANDS(1, "{none | <scheme>,...}")
    },

    { OPTION(general, openConnection, host),
      OPERANDS(1, "[<host>][:<port>]")
    }
  END_OPTIONS(2)

  BrlapiSession *session = allocateMemory(sizeof(*session));
  if (session) {
    if ((session->handle = allocateMemory(brlapi_getHandleSize()))) {
      int result = brlapi__openConnection(session->handle, &options.settings, &session->settings);

      if (result != -1) {
        session->fileDescriptor = result;

        char name[0X20];
        Tcl_Command command;

        {
          static unsigned int suffix = 0;
          snprintf(name, sizeof(name), "brlapi%u", suffix++);
          command = Tcl_CreateObjCommand(interp, name, brlapiSessionCommand, session, endSession);
        }

        if (command) {
          setStringResult(interp, name, -1);
          return TCL_OK;
        }
      } else {
        setBrlapiError(interp);
      }

      deallocateMemory(session->handle);
    }

    deallocateMemory(session);
  }

  return TCL_ERROR;
}

static int
brlapiGeneralCommand (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
  BEGIN_FUNCTIONS
    FUNCTION(general, describeKeyCode),
    FUNCTION(general, expandKeyCode),
    FUNCTION(general, getHandleSize),
    FUNCTION(general, getVersionString),
    FUNCTION(general, makeDots),
    FUNCTION(general, openConnection),
  END_FUNCTIONS

  TEST_COMMAND_ARGUMENTS();
  TEST_FUNCTION_ARGUMENT();
  return function->handler(interp, objv, objc, data);
}

int
Brlapi_tcl_Init (Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "brlapi", brlapiGeneralCommand, NULL, NULL);
  Tcl_PkgProvide(interp, "Brlapi", BRLAPI_RELEASE);
  return TCL_OK;
}

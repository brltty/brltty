/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2023 by Dave Mielke <dave@mielke.cc>
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
  int tclResult = (expression); \
  if (tclResult != TCL_OK) return tclResult; \
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

  Tcl_Interp *tclInterpreter;
  Tcl_Command tclCommand;
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
  size_t size = brlapi_strerror_r(&brlapi_error, NULL, 0);
  char text[size+1];
  const char *name;
  int number;

  brlapi_strerror_r(&brlapi_error, text, sizeof(text));

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
  Tcl_AppendToObj(result, text, -1);
}

#define TEST_BRLAPI_OK(expression) \
do { \
  if ((expression) == -1) { \
    setBrlapiError(interp); \
    return TCL_ERROR; \
  } \
} while (0)

static int
getDisplaySize (Tcl_Interp *interp, BrlapiSession *session) {
  TEST_BRLAPI_OK(brlapi__getDisplaySize(session->handle, &session->displayWidth, &session->displayHeight));
  return TCL_OK;
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

#define TEST_FUNCTION_ARGUMENTS(required,optional,syntax) \
  TEST_ARGUMENT_COUNT(2, (required), (optional), (syntax))

#define TEST_FUNCTION_NO_ARGUMENTS() TEST_FUNCTION_ARGUMENTS(0, 0, NULL)

static int
invokeFunction (Tcl_Interp *interp, Tcl_Obj *const objv[], int objc, const FunctionEntry *functions, void *data) {
  TEST_ARGUMENT_COUNT(1, 1, -1, "<function> [<arg> ...]");
  const FunctionEntry *function;

  {
    int index;
    TEST_TCL_OK(Tcl_GetIndexFromObjStruct(interp, objv[1], functions, sizeof(*functions), "function", 0, &index));
    function = functions + index;
  }

  return function->handler(interp, objv, objc, data);
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
  const char *syntax;
} OptionEntry;

static int
processOptions (
  Tcl_Interp *interp, void *data,
  Tcl_Obj *const objv[], int objc,
  int start, int *consumed,
  const OptionEntry *options
) {
  *consumed = objc;
  objv += start;
  objc -= start;

  while (objc > 0) {
    Tcl_Obj *name = objv[0];

    {
      const char *string = Tcl_GetString(name);
      if (!string) return TCL_ERROR;
      if (*string != '-') break;
    }

    int index;
    TEST_TCL_OK(Tcl_GetIndexFromObjStruct(interp, name, options, sizeof(*options), "option", 0, &index));
    const OptionEntry *option = &options[index];

    int count = option->operands;
    TEST_ARGUMENT_COUNT(1, count, -1, option->syntax);
    TEST_TCL_OK(option->handler(interp, objv, data));

    count += 1;
    objv += count;
    objc -= count;
  }

  *consumed -= objc;
  return TCL_OK;
}

#define BEGIN_OPTIONS { static const OptionEntry optionTable[] = {
#define END_OPTIONS(start,required,optional,syntax) \
  , {.name = NULL} \
  }; \
  int consumed; \
  TEST_TCL_OK(processOptions(interp, &options, objv, objc, (start), &consumed, optionTable)); \
  objv += consumed; \
  objc -= consumed; \
  TEST_ARGUMENT_COUNT(0, (required), (optional), (syntax)); \
}
#define OPTION(command,function,option) \
  .name = "-" #option, .handler = OPTION_HANDLER_NAME(command, function, option)
#define OPERANDS(count,text) \
  .operands = (count), .syntax = ((count)? (text): NULL)

static int
getSessionStringProperty (
  Tcl_Interp *interp, BrlapiSession *session,
  int BRLAPI_STDCALL (*getProperty) (brlapi_handle_t *handle, char *buffer, size_t size)
) {
  size_t size = 0X10;

  while (1) {
    char buffer[size];
    int result = getProperty(session->handle, buffer, size);
    TEST_BRLAPI_OK(result);

    if (result <= size) {
      setStringResult(interp, buffer, result-1);
      return TCL_OK;
    }

    size = result;
  }
}

FUNCTION_HANDLER(session, closeConnection) {
  BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();
  Tcl_DeleteCommandFromToken(interp, session->tclCommand);
  return TCL_OK;
}

FUNCTION_HANDLER(session, enterRawMode) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<driver>");

  const char *driver = Tcl_GetString(objv[2]);
  if (!driver) return TCL_ERROR;

  TEST_BRLAPI_OK(brlapi__enterRawMode(session->handle, driver));
  return TCL_OK;
}

typedef struct {
  int tty;
  const char *driver;
} EnterTtyModeOptions;

OPTION_HANDLER(session, enterTtyMode, commands) {
  EnterTtyModeOptions *options = data;
  options->driver = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, enterTtyMode, keys) {
  EnterTtyModeOptions *options = data;
  return (options->driver = Tcl_GetString(objv[1]))? TCL_OK: TCL_ERROR;
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
    { OPTION(session, enterTtyMode, commands),
      OPERANDS(0, "")
    },

    { OPTION(session, enterTtyMode, keys),
      OPERANDS(1, "<driver>")
    },

    { OPTION(session, enterTtyMode, tty),
      OPERANDS(1, "{default | <number>}")
    }
  END_OPTIONS(2, 0, 0, "")

  {
    int result = brlapi__enterTtyMode(session->handle, options.tty, options.driver);
    TEST_BRLAPI_OK(result);

    setIntResult(interp, result);
    return TCL_OK;
  }
}

typedef struct {
  Tcl_Obj *path;
  const char *driver;
} EnterTtyModeWithPathOptions;

OPTION_HANDLER(session, enterTtyModeWithPath, commands) {
  EnterTtyModeWithPathOptions *options = data;
  options->driver = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, enterTtyModeWithPath, keys) {
  EnterTtyModeWithPathOptions *options = data;
  return (options->driver = Tcl_GetString(objv[1]))? TCL_OK: TCL_ERROR;
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
    { OPTION(session, enterTtyModeWithPath, commands),
      OPERANDS(0, "")
    },

    { OPTION(session, enterTtyModeWithPath, keys),
      OPERANDS(1, "<driver>")
    },

    { OPTION(session, enterTtyModeWithPath, path),
      OPERANDS(1, "<list>")
    }
  END_OPTIONS(2, 0, 0, "")

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

    TEST_BRLAPI_OK(brlapi__enterTtyModeWithPath(session->handle, path, count, options.driver));
  } else {
    TEST_BRLAPI_OK(brlapi__enterTtyModeWithPath(session->handle, NULL, 0, options.driver));
  }

  return TCL_OK;
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

  TEST_BRLAPI_OK(brlapi__leaveRawMode(session->handle));
  return TCL_OK;
}

FUNCTION_HANDLER(session, leaveTtyMode) {
  BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();

  TEST_BRLAPI_OK(brlapi__leaveTtyMode(session->handle));
  return TCL_OK;
}

typedef struct {
  Tcl_Obj *value;
  brlapi_param_flags_t flags;
} ParameterOptions;

OPTION_HANDLER(session, parameter, echo) {
  ParameterOptions *options = data;
  brlapi_param_flags_t flag = BRLAPI_PARAMF_SELF;

  int echo;
  TEST_TCL_OK(Tcl_GetBooleanFromObj(interp, objv[1], &echo));

  if (echo) {
    options->flags |= flag;
  } else {
    options->flags &= ~flag;
  }

  return TCL_OK;
}

OPTION_HANDLER(session, parameter, global) {
  ParameterOptions *options = data;
  brlapi_param_flags_t flag = BRLAPI_PARAMF_GLOBAL;

  int global;
  TEST_TCL_OK(Tcl_GetBooleanFromObj(interp, objv[1], &global));

  if (global) {
    options->flags |= flag;
  } else {
    options->flags &= ~flag;
  }

  return TCL_OK;
}

OPTION_HANDLER(session, parameter, set) {
  ParameterOptions *options = data;
  options->value = objv[1];
  return TCL_OK;
}

static int
parseParameterName (Tcl_Interp *interp, Tcl_Obj *name, brlapi_param_t *parameter) {
  typedef struct {
    const char *name;
    brlapi_param_t value;
  } ParameterEntry;

  static const ParameterEntry parameters[] = {
    #include "parameters.auto.h"
    { .name=NULL }
  };

  int index;
  TEST_TCL_OK(Tcl_GetIndexFromObjStruct(interp, name, parameters, sizeof(*parameters), "parameter name", 0, &index));

  *parameter = parameters[index].value;
  return TCL_OK;
}

FUNCTION_HANDLER(session, parameter) {
  BrlapiSession *session = data;

  ParameterOptions options = {
    .value = NULL,
    .flags = 0
  };

  BEGIN_OPTIONS
    { OPTION(session, parameter, echo),
      OPERANDS(1, "<boolean>")
    },

    { OPTION(session, parameter, global),
      OPERANDS(1, "<boolean>")
    },

    { OPTION(session, parameter, set),
      OPERANDS(1, "<value>")
    }
  END_OPTIONS(2, 1, 1, "<name> [<subparam>]")

  Tcl_Obj *name = objv[0];
  brlapi_param_t parameter;
  TEST_TCL_OK(parseParameterName(interp, name, &parameter));

  const brlapi_param_properties_t *properties = brlapi_getParameterProperties(parameter);
  if (!properties) {
    setStringsResult(interp, "bad parameter name", Tcl_GetString(name), NULL);
    return TCL_ERROR;
  }

  int haveSubparam = objc > 1;
  brlapi_param_subparam_t subparam;

  if (properties->hasSubparam) {
    if (!haveSubparam) {
      setStringsResult(interp, "subparam not specified for \"", Tcl_GetString(name), "\"", NULL);
      return TCL_ERROR;
    }

    Tcl_WideInt value;
    TEST_TCL_OK(Tcl_GetWideIntFromObj(interp, objv[1], &value));
    subparam = value;
  } else {
    if (haveSubparam) {
      setStringsResult(interp, "subparam specified for \"", Tcl_GetString(name), "\"", NULL);
      return TCL_ERROR;
    }

    subparam = 0;
  }

  if (options.value) {
    Tcl_Obj *value = options.value;

    switch (properties->type) {
      case BRLAPI_PARAM_TYPE_STRING: {
        const char *string = Tcl_GetString(value);
	TEST_BRLAPI_OK(brlapi__setParameter(session->handle, parameter, subparam, options.flags, string, strlen(string)));
        break;
      }

      default: {
        Tcl_Obj **elements;
        int count;
        TEST_TCL_OK(Tcl_ListObjGetElements(interp, value, &count, &elements));

        if (count) {
          switch (properties->type) {
            case BRLAPI_PARAM_TYPE_BOOLEAN: {
              brlapi_param_bool_t buffer[count];

              for (int index=0; index<count; index+=1) {
                int boolean;
                TEST_TCL_OK(Tcl_GetBooleanFromObj(interp, elements[index], &boolean));
                buffer[index] = !!boolean;
              }

	      TEST_BRLAPI_OK(brlapi__setParameter(session->handle, parameter, subparam, options.flags, buffer, sizeof(buffer)));
              break;
            }

            case BRLAPI_PARAM_TYPE_UINT8: {
              uint8_t buffer[count];

              for (int index=0; index<count; index+=1) {
                int integer;
                TEST_TCL_OK(Tcl_GetIntFromObj(interp, elements[index], &integer));
                buffer[index] = integer;
              }

	      TEST_BRLAPI_OK(brlapi__setParameter(session->handle, parameter, subparam, options.flags, buffer, sizeof(buffer)));
              break;
            }

            case BRLAPI_PARAM_TYPE_UINT16: {
              uint16_t buffer[count];

              for (int index=0; index<count; index+=1) {
                int integer;
                TEST_TCL_OK(Tcl_GetIntFromObj(interp, elements[index], &integer));
                buffer[index] = integer;
              }

	      TEST_BRLAPI_OK(brlapi__setParameter(session->handle, parameter, subparam, options.flags, buffer, sizeof(buffer)));
              break;
            }

            case BRLAPI_PARAM_TYPE_UINT32: {
              uint32_t buffer[count];

              for (int index=0; index<count; index+=1) {
                int integer;
                TEST_TCL_OK(Tcl_GetIntFromObj(interp, elements[index], &integer));
                buffer[index] = integer;
              }

	      TEST_BRLAPI_OK(brlapi__setParameter(session->handle, parameter, subparam, options.flags, buffer, sizeof(buffer)));
              break;
            }

            case BRLAPI_PARAM_TYPE_UINT64: {
              uint64_t buffer[count];

              for (int index=0; index<count; index+=1) {
                long integer;
                TEST_TCL_OK(Tcl_GetLongFromObj(interp, elements[index], &integer));
                buffer[index] = integer;
              }

	      TEST_BRLAPI_OK(brlapi__setParameter(session->handle, parameter, subparam, options.flags, buffer, sizeof(buffer)));
              break;
            }

            default:
              break;
          }
        } else {
	  TEST_BRLAPI_OK(brlapi__setParameter(session->handle, parameter, subparam, options.flags, NULL, 0));
        }

        break;
      }
    }
  } else {
    size_t length;
    void *value = brlapi__getParameterAlloc(session->handle, parameter, subparam, options.flags, &length);

    if (!value) {
      setBrlapiError(interp);
      return TCL_ERROR;
    }

    Tcl_Obj *result = Tcl_GetObjResult(interp);

    switch (properties->type) {
      case BRLAPI_PARAM_TYPE_STRING: {
        Tcl_SetStringObj(result, value, -1);
        break;
      }

      default: {
        Tcl_SetListObj(result, 0, NULL);
        const void *end = value + length;

        switch (properties->type) {
          case BRLAPI_PARAM_TYPE_BOOLEAN: {
            const brlapi_param_bool_t *boolean = value;

            while (boolean < (const brlapi_param_bool_t *)end) {
              Tcl_Obj *element = Tcl_NewBooleanObj(*boolean);
              if (!element) return TCL_ERROR;
              TEST_TCL_OK(Tcl_ListObjAppendElement(interp, result, element));
              boolean += 1;
            }

            break;
          }

          case BRLAPI_PARAM_TYPE_UINT8: {
            const uint8_t *integer = value;

            while (integer < (const uint8_t *)end) {
              Tcl_Obj *element = Tcl_NewIntObj(*integer);
              if (!element) return TCL_ERROR;
              TEST_TCL_OK(Tcl_ListObjAppendElement(interp, result, element));
              integer += 1;
            }

            break;
          }

          case BRLAPI_PARAM_TYPE_UINT16: {
            const uint16_t *integer = value;

            while (integer < (const uint16_t *)end) {
              Tcl_Obj *element = Tcl_NewIntObj(*integer);
              if (!element) return TCL_ERROR;
              TEST_TCL_OK(Tcl_ListObjAppendElement(interp, result, element));
              integer += 1;
            }

            break;
          }

          case BRLAPI_PARAM_TYPE_UINT32: {
            const uint32_t *integer = value;

            while (integer < (const uint32_t *)end) {
              Tcl_Obj *element = Tcl_NewIntObj(*integer);
              if (!element) return TCL_ERROR;
              TEST_TCL_OK(Tcl_ListObjAppendElement(interp, result, element));
              integer += 1;
            }

            break;
          }

          case BRLAPI_PARAM_TYPE_UINT64: {
            const uint64_t *integer = value;

            while (integer < (const uint64_t *)end) {
              Tcl_Obj *element = Tcl_NewWideIntObj(*integer);
              if (!element) return TCL_ERROR;
              TEST_TCL_OK(Tcl_ListObjAppendElement(interp, result, element));
              integer += 1;
            }

            break;
          }

          default:
            break;
        }

        break;
      }
    }

    free(value);
  }

  return TCL_OK;
}

FUNCTION_HANDLER(session, readKey) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<wait>");

  int length;
  const char *operand = Tcl_GetStringFromObj(objv[2], &length);
  if (!operand) return TCL_ERROR;

  int wait;
  TEST_TCL_OK(Tcl_GetBoolean(interp, operand, &wait));

  brlapi_keyCode_t key;
  int result = brlapi__readKey(session->handle, wait, &key);
  TEST_BRLAPI_OK(result);

  if (result == 1) setWideIntResult(interp, key);
  return TCL_OK;
}

FUNCTION_HANDLER(session, readKeyWithTimeout) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "{infinite | <seconds>}");

  int length;
  const char *operand = Tcl_GetStringFromObj(objv[2], &length);
  if (!operand) return TCL_ERROR;
  int timeout;

  if (strcmp(operand, "infinite") == 0) {
    timeout = -1;
  } else {
    int seconds;
    TEST_TCL_OK(Tcl_GetInt(interp, operand, &seconds));

    if (seconds < 0) {
      setStringsResult(interp, "negative timeout ", operand, NULL);
      return TCL_ERROR;
    }

    timeout = seconds * 1000;
  }

  brlapi_keyCode_t code;
  int result = brlapi__readKeyWithTimeout(session->handle, timeout, &code);
  TEST_BRLAPI_OK(result);

  if (result == 1) setWideIntResult(interp, code);
  return TCL_OK;
}

FUNCTION_HANDLER(session, recvRaw) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<maximumLength>");

  int size;
  TEST_TCL_OK(Tcl_GetIntFromObj(interp, objv[2], &size));

  unsigned char buffer[size];
  int result = brlapi__recvRaw(session->handle, buffer, size);
  TEST_BRLAPI_OK(result);

  setByteArrayResult(interp, buffer, result);
  return TCL_OK;
}

FUNCTION_HANDLER(session, resumeDriver) {
  BrlapiSession *session = data;
  TEST_FUNCTION_NO_ARGUMENTS();

  TEST_BRLAPI_OK(brlapi__resumeDriver(session->handle));
  return TCL_OK;
}

FUNCTION_HANDLER(session, sendRaw) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<packet>");

  int count;
  const unsigned char *bytes = Tcl_GetByteArrayFromObj(objv[2], &count);

  TEST_BRLAPI_OK(brlapi__sendRaw(session->handle, bytes, count));
  return TCL_OK;
}

FUNCTION_HANDLER(session, setFocus) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<ttyNumber>");

  int tty;
  TEST_TCL_OK(Tcl_GetIntFromObj(interp, objv[2], &tty));

  TEST_BRLAPI_OK(brlapi__setFocus(session->handle, tty));
  return TCL_OK;
}

FUNCTION_HANDLER(session, suspendDriver) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<driver>");

  const char *driver = Tcl_GetString(objv[2]);
  if (!driver) return TCL_ERROR;

  TEST_BRLAPI_OK(brlapi__suspendDriver(session->handle, driver));
  return TCL_OK;
}

typedef struct {
  brlapi_writeArguments_t arguments;

  Tcl_Obj *textObject;
  int textLength;

  int andLength;
  int orLength;

  unsigned numericCursor:1;
  unsigned numericDisplay:1;
  unsigned regionSpecified:1;
} WriteOptions;

OPTION_HANDLER(session, write, and) {
  WriteOptions *options = data;
  options->arguments.andMask = Tcl_GetByteArrayFromObj(objv[1], &options->andLength);
  if (!options->andLength) options->arguments.andMask = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, write, cursor) {
  WriteOptions *options = data;
  int isNumeric = 0;

  Tcl_Obj *object = objv[1];
  const char *operand = Tcl_GetString(object);
  if (!operand) return TCL_ERROR;

  if (strcmp(operand, "off") == 0) {
    options->arguments.cursor = BRLAPI_CURSOR_OFF;
  } else if (strcmp(operand, "leave") == 0) {
    options->arguments.cursor = BRLAPI_CURSOR_LEAVE;
  } else {
    int position;
    TEST_TCL_OK(Tcl_GetIntFromObj(interp, object, &position));

    options->arguments.cursor = position + 1;
    isNumeric = 1;
  }

  options->numericCursor = isNumeric;
  return TCL_OK;
}

OPTION_HANDLER(session, write, display) {
  WriteOptions *options = data;
  int isNumeric = 0;

  Tcl_Obj *object = objv[1];
  const char *operand = Tcl_GetString(object);
  if (!operand) return TCL_ERROR;

  if (strcmp(operand, "default") == 0) {
    options->arguments.displayNumber = BRLAPI_DISPLAY_DEFAULT;
  } else {
    int number;
    TEST_TCL_OK(Tcl_GetIntFromObj(interp, object, &number));

    options->arguments.displayNumber = number;
    isNumeric = 1;
  }

  options->numericDisplay = isNumeric;
  return TCL_OK;
}

OPTION_HANDLER(session, write, or) {
  WriteOptions *options = data;
  options->arguments.orMask = Tcl_GetByteArrayFromObj(objv[1], &options->orLength);
  if (!options->orLength) options->arguments.orMask = NULL;
  return TCL_OK;
}

OPTION_HANDLER(session, write, region) {
  WriteOptions *options = data;

  int start;
  TEST_TCL_OK(Tcl_GetIntFromObj(interp, objv[1], &start));
  options->arguments.regionBegin = start + 1;

  int size;
  TEST_TCL_OK(Tcl_GetIntFromObj(interp, objv[2], &size));
  options->arguments.regionSize = size;

  options->regionSpecified = 1;
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
    { OPTION(session, write, and),
      OPERANDS(1, "<dots>")
    },

    { OPTION(session, write, cursor),
      OPERANDS(1, "{leave | off | <offset>}")
    },

    { OPTION(session, write, display),
      OPERANDS(1, "{default | <number>}")
    },

    { OPTION(session, write, or),
      OPERANDS(1, "<dots>")
    },

    { OPTION(session, write, region),
      OPERANDS(2, "<start> <size>")
    },

    { OPTION(session, write, text),
      OPERANDS(1, "<string>")
    }
  END_OPTIONS(2, 0, 0, "")

  if (options.numericDisplay) {
    if (options.arguments.displayNumber < 0) {
      setStringResult(interp, "display number out of range", -1);
      return TCL_ERROR;
    }
  }

  int characterCount = 0;
  {
    typedef struct {
      const char *name;
      const void *address;
      int length;
    } LengthEntry;

    const LengthEntry lengths[] = {
      { .name = "text",
        .address = options.textObject,
        .length = options.textLength
      },

      { .name = "andMask",
        .address = options.arguments.andMask,
        .length = options.andLength
      },

      { .name = "orMask",
        .address = options.arguments.orMask,
        .length = options.orLength
      },

      { .name = NULL }
    };

    const LengthEntry *current = lengths;
    const LengthEntry *first = NULL;

    while (current->name) {
      if (current->address) {
        if (!first) {
          first = current;
          characterCount = current->length;
        } else if (current->length != first->length) {
          setStringsResult(interp, first->name, "/", current->name, " length mismatch", NULL);
          return TCL_ERROR;
        }
      }

      current += 1;
    }
  }

  unsigned int cellCount;
  TEST_TCL_OK(getCellCount(interp, session, &cellCount));

  if (options.numericCursor) {
    int position = options.arguments.cursor - 1;

    if ((position < 0) || (position >= cellCount)) {
      setStringResult(interp, "cursor position out of range", -1);
      return TCL_ERROR;
    }
  }

  if (options.regionSpecified) {
    int begin = options.arguments.regionBegin;
    int size = options.arguments.regionSize;

    if ((begin < 1) || (begin >= cellCount)) {
      setStringResult(interp, "beginning of region out of range", -1);
      return TCL_ERROR;
    }

    if ((size < 1) || (((begin - 1) + size) > cellCount)) {
      setStringResult(interp, "size of region out of range", -1);
      return TCL_ERROR;
    }

    cellCount = size;
  } else {
    options.arguments.regionBegin = 1;
    options.arguments.regionSize = cellCount;
  }

  unsigned char andMask[cellCount];
  unsigned char orMask[cellCount];

  if (characterCount < cellCount) {
    if (options.arguments.andMask) {
      memset(andMask, 0XFF, cellCount);
      memcpy(andMask, options.arguments.andMask, characterCount);
      options.arguments.andMask = andMask;
    }

    if (options.arguments.orMask) {
      memset(orMask, 0X00, cellCount);
      memcpy(orMask, options.arguments.orMask, characterCount);
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
      } while (Tcl_GetCharLength(options.textObject) < cellCount);
    }
  } else if (characterCount > cellCount) {
    if (options.textObject) {
      if (!(options.textObject = Tcl_GetRange(options.textObject, 0, cellCount-1))) {
        return TCL_ERROR;
      }
    }
  }

  if (options.textObject) {
    options.arguments.charset = "UTF-8";
    options.textLength = Tcl_GetCharLength(options.textObject);

    options.arguments.text = Tcl_GetString(options.textObject);
    if (!options.arguments.text) return TCL_ERROR;
  }

  // TCL uses C0,80 as the UTF-8 representation for NUL.
  // This causes problems for BrlAPI.
  {
    const char *text = options.arguments.text;
    size_t length = text? strlen(text): 0;
    char buffer[length? length: 1];

    if (text) {
      const char *from = text;
      char *to = buffer;

      while (1) {
        const char *nul = strstr(from, "\xC0\x80");

        if (!nul) {
          if (to != buffer) {
            strcpy(to, from);
            options.arguments.text = buffer;
          }

          break;
        }

        size_t count = nul - from;
        memcpy(to, from, count);
        to += count;
        *to++ = 0;
        from += count + 2;
        length -= 1;
      }
    }

    options.arguments.textSize = length;
    TEST_BRLAPI_OK(brlapi__write(session->handle, &options.arguments));
    return TCL_OK;
  }
}

FUNCTION_HANDLER(session, writeDots) {
  BrlapiSession *session = data;
  TEST_FUNCTION_ARGUMENTS(1, 0, "<dots>");

  unsigned int size;
  TEST_TCL_OK(getCellCount(interp, session, &size));

  unsigned char buffer[size];
  int count;
  const unsigned char *cells = Tcl_GetByteArrayFromObj(objv[2], &count);

  if (count < size) {
    memcpy(buffer, cells, count);
    memset(&buffer[count], 0, size-count);
    cells = buffer;
  }

  TEST_BRLAPI_OK(brlapi__writeDots(session->handle, cells));
  return TCL_OK;
}

static int
changeKeys (
  Tcl_Interp *interp, Tcl_Obj *const objv[], int objc, BrlapiSession *session,
  int BRLAPI_STDCALL (*change) (brlapi_handle_t *handle, brlapi_rangeType_t type, const brlapi_keyCode_t keys[], unsigned int count)
) {
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

      TEST_BRLAPI_OK(change(session->handle, rangeType, codes, codeCount));
      return TCL_OK;
    }
  } else if (codeList) {
    setStringResult(interp, "unexpected key code list", -1);
    return TCL_ERROR;
  }

  TEST_BRLAPI_OK(change(session->handle, rangeType, NULL, 0));
  return TCL_OK;
}

FUNCTION_HANDLER(session, acceptKeys) {
  BrlapiSession *session = data;
  return changeKeys(interp, objv, objc, session, brlapi__acceptKeys);
}

FUNCTION_HANDLER(session, ignoreKeys) {
  BrlapiSession *session = data;
  return changeKeys(interp, objv, objc, session, brlapi__ignoreKeys);
}

static int
changeKeyRanges (
  Tcl_Interp *interp, Tcl_Obj *const objv[], int objc, BrlapiSession *session,
  int BRLAPI_STDCALL (*change) (brlapi_handle_t *handle, const brlapi_range_t ranges[], unsigned int count)
) {
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

    TEST_BRLAPI_OK(change(session->handle, ranges, rangeCount));
    return TCL_OK;
  }

  TEST_BRLAPI_OK(change(session->handle, NULL, 0));
  return TCL_OK;
}

FUNCTION_HANDLER(session, acceptKeyRanges) {
  BrlapiSession *session = data;
  return changeKeyRanges(interp, objv, objc, session, brlapi__acceptKeyRanges);
}

FUNCTION_HANDLER(session, ignoreKeyRanges) {
  BrlapiSession *session = data;
  return changeKeyRanges(interp, objv, objc, session, brlapi__ignoreKeyRanges);
}

static void
endSession (ClientData data) {
  BrlapiSession *session = data;
  brlapi__closeConnection(session->handle);
  deallocateMemory(session->handle);
  deallocateMemory(session);
}

static void
handleBrlapiException (
  brlapi_handle_t *handle,
  int error, brlapi_packetType_t type,
  const void *packet, size_t size
) {
  BrlapiSession *session = brlapi__getClientData(handle);
  Tcl_Interp *interp = session->tclInterpreter;
  char message[0X100];

  brlapi__strexception(
    handle, message, sizeof(message), error, type, packet, size
  );

  const char *name = Tcl_GetCommandName(interp, session->tclCommand);
  fprintf(stderr, "BrlAPI session failure: %s: %s\n", name, message);
  exit(1);
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
    FUNCTION(session, parameter),
    FUNCTION(session, readKey),
    FUNCTION(session, readKeyWithTimeout),
    FUNCTION(session, recvRaw),
    FUNCTION(session, resumeDriver),
    FUNCTION(session, sendRaw),
    FUNCTION(session, setFocus),
    FUNCTION(session, suspendDriver),
    FUNCTION(session, write),
    FUNCTION(session, writeDots),
  END_FUNCTIONS

  return invokeFunction(interp, objv, objc, functions, data);
}

FUNCTION_HANDLER(general, describeKeyCode) {
  TEST_FUNCTION_ARGUMENTS(2, 0, "<keyCode> <arrayName>");

  Tcl_WideInt keyCode;
  TEST_TCL_OK(Tcl_GetWideIntFromObj(interp, objv[2], &keyCode));

  const char *array = Tcl_GetString(objv[3]);
  if (!array) return TCL_ERROR;

  brlapi_describedKeyCode_t dkc;
  TEST_BRLAPI_OK(brlapi_describeKeyCode(keyCode, &dkc));

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
  TEST_BRLAPI_OK(brlapi_expandKeyCode(keyCode, &ekc));

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

FUNCTION_HANDLER(general, getVersionNumbers) {
  TEST_FUNCTION_NO_ARGUMENTS();

  int majorNumber, minorNumber, revisionNumber;
  brlapi_getLibraryVersion(&majorNumber, &minorNumber, &revisionNumber);

  Tcl_Obj *majorObject = Tcl_NewIntObj(majorNumber);
  if (!majorObject) return TCL_ERROR;

  Tcl_Obj *minorObject = Tcl_NewIntObj(minorNumber);
  if (!minorObject) return TCL_ERROR;

  Tcl_Obj *revisionObject = Tcl_NewIntObj(revisionNumber);
  if (!revisionObject) return TCL_ERROR;

  Tcl_Obj *const elements[] = {majorObject, minorObject, revisionObject};
  Tcl_Obj *list = Tcl_NewListObj(3, elements);
  if (!list) return TCL_ERROR;

  Tcl_SetObjResult(interp, list);
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
  END_OPTIONS(2, 0, 0, "")

  BrlapiSession *session = allocateMemory(sizeof(*session));
  if (session) {
    memset(session, 0, sizeof(*session));
    session->tclInterpreter = interp;

    if ((session->handle = allocateMemory(brlapi_getHandleSize()))) {
      int result = brlapi__openConnection(session->handle, &options.settings, &session->settings);

      if (result != -1) {
        session->fileDescriptor = result;

        brlapi__setClientData(session->handle, session);
        brlapi__setExceptionHandler(session->handle, handleBrlapiException);

        char name[0X20];
        {
          static unsigned int suffix = 0;
          snprintf(name, sizeof(name), "brlapi%u", suffix++);
          session->tclCommand = Tcl_CreateObjCommand(interp, name, brlapiSessionCommand, session, endSession);
        }

        if (session->tclCommand) {
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
    FUNCTION(general, getVersionNumbers),
    FUNCTION(general, getVersionString),
    FUNCTION(general, makeDots),
    FUNCTION(general, openConnection),
  END_FUNCTIONS

  return invokeFunction(interp, objv, objc, functions, data);
}

int
Brlapi_tcl_Init (Tcl_Interp *interp) {
  int result = TCL_ERROR;
  Tcl_Command command = Tcl_CreateObjCommand(interp, "brlapi", brlapiGeneralCommand, NULL, NULL);

  if (command) {
    result = Tcl_PkgProvide(interp, "Brlapi", BRLAPI_RELEASE);
    if (result != TCL_OK) Tcl_DeleteCommandFromToken(interp, command);
  }

  return result;
}

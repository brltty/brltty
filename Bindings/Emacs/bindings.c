/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2022 by Dave Mielke <dave@mielke.cc>
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

#define BRLAPI_NO_SINGLE_SESSION
#include "prologue.h"
#include <brlapi.h>
#include <emacs-module.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int plugin_is_GPL_compatible;

static const char *error_name = "brlapi-error";
static const char *error_message = "BrlAPI Error";

static void
error(emacs_env *env) {
  const char *const str = brlapi_strerror(&brlapi_error);
  emacs_value error_symbol = env->intern(env, error_name);
  emacs_value data[] = {
    env->make_integer(env, brlapi_errno),
    env->make_integer(env, brlapi_libcerrno),
    env->make_integer(env, brlapi_gaierrno),
    env->make_string(env, str, strlen(str))
  };

  env->non_local_exit_signal(env,
    error_symbol,
    env->funcall(env, env->intern(env, "list"), ARRAY_COUNT(data), data)
  );
}

static char *
extract_string(emacs_env *env, ptrdiff_t nargs, ptrdiff_t arg, emacs_value *args) {
  if (arg >= nargs) return NULL;

  if (!env->is_not_nil(env, args[arg])) return NULL;
  
  ptrdiff_t length;
  env->copy_string_contents(env, args[arg], NULL, &length);
  char *buf = malloc(length);
  env->copy_string_contents(env, args[arg], buf, &length);
  return buf;
}

static emacs_value
openConnection(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  brlapi_handle_t *const handle = malloc(brlapi_getHandleSize());
  char *host = extract_string(env, nargs, 0, args);
  char *auth = extract_string(env, nargs, 1, args);
  const brlapi_connectionSettings_t desiredSettings = {
    .host = host, .auth = auth
  };
  brlapi_connectionSettings_t actualSettings;
  int result;

  if (env->non_local_exit_check(env) != emacs_funcall_exit_return) {
    if (host) free(host);
    if (auth) free(auth);
    return NULL;
  }

  result = brlapi__openConnection(handle, &desiredSettings, &actualSettings);

  if (host) free(host);
  if (auth) free(auth);
  
  if (result == -1) {
    error(env);
    return NULL;
  }

  return env->make_user_ptr(env, NULL, handle);
}

static emacs_value
getDriverName(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  brlapi_handle_t *const handle = env->get_user_ptr(env, args[0]);
  char name[BRLAPI_MAXNAMELENGTH + 1];
  size_t length;

  if (handle == NULL) return NULL;

  length = brlapi__getDriverName(handle, name, sizeof(name));

  if (length == -1) {
    error(env);
    return NULL;
  }

  if (length > 0) length--;

  return env->make_string(env, name, length);
}

static emacs_value
getModelIdentifier(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  brlapi_handle_t *const handle = env->get_user_ptr(env, args[0]);
  char name[BRLAPI_MAXNAMELENGTH + 1];
  size_t length;

  if (handle == NULL) return NULL;

  length = brlapi__getModelIdentifier(handle, name, sizeof(name));

  if (length == -1) {
    error(env);
    return NULL;
  }

  if (length > 0) length--;

  return env->make_string(env, name, length);
}

static inline emacs_value
cons(emacs_env *env, emacs_value car, emacs_value cdr) {
  emacs_value args[] = { car, cdr };

  return env->funcall(env, env->intern(env, "cons"), ARRAY_COUNT(args), args);
}

static emacs_value
getDisplaySize(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  brlapi_handle_t *const handle = env->get_user_ptr(env, args[0]);
  unsigned int x, y;

  if (handle == NULL) return NULL;

  if (brlapi__getDisplaySize(handle, &x, &y) == -1) {
    error(env);
    return NULL;
  }

  return cons(env, env->make_integer(env, x), env->make_integer(env, y));
}

static inline int
extract_tty(emacs_env *env, emacs_value arg) {
  if (!env->is_not_nil(env, arg)) return BRLAPI_TTY_DEFAULT;
  return env->extract_integer(env, arg);
}

static char *
extract_driver(emacs_env *env, emacs_value arg) {
  ptrdiff_t length;
  env->copy_string_contents(env, arg, NULL, &length);
  char *driver = malloc(length);
  env->copy_string_contents(env, arg, driver, &length);
  return driver;
}

static emacs_value
enterTtyMode(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  brlapi_handle_t *const handle = env->get_user_ptr(env, args[0]);
  int tty = nargs > 1? extract_tty(env, args[1]): BRLAPI_TTY_DEFAULT;
  char *driver = nargs > 2? extract_driver(env, args[2]): NULL;

  if (env->non_local_exit_check(env) != emacs_funcall_exit_return) {
    if (driver) free(driver);
    return NULL;
  }

  tty = brlapi__enterTtyMode(handle, tty, driver);

  if (driver) free(driver);

  if (tty == -1) {
    error(env);
    return NULL;
  }
  
  return env->make_integer(env, tty);
}

static emacs_value
leaveTtyMode(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  brlapi_handle_t *const handle = env->get_user_ptr(env, args[0]);

  if (handle == NULL) return NULL;

  if (brlapi__leaveTtyMode(handle) == -1) {
    error(env);
    return NULL;
  }

  return env->intern(env, "nil");
}

static inline int
extract_cursor(emacs_env *env, emacs_value arg) {
  if (!env->is_not_nil(env, arg)) return BRLAPI_CURSOR_OFF;
  if (env->eq(env, arg, env->intern(env, "leave"))) return BRLAPI_CURSOR_LEAVE;
  return env->extract_integer(env, arg);
}

static emacs_value
writeText(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  brlapi_handle_t *const handle = env->get_user_ptr(env, args[0]);

  if (handle) {
    ptrdiff_t size;

    if (env->copy_string_contents(env, args[1], NULL, &size)) {
      char text[size];

      if (env->copy_string_contents(env, args[1], text, &size)) {
	int cursor = nargs > 2? extract_cursor(env, args[2]): BRLAPI_CURSOR_OFF;

	if (env->non_local_exit_check(env) == emacs_funcall_exit_return) {
	  if (brlapi__writeText(handle, cursor, text) != -1) {
	    return env->intern(env, "nil");
	  }

	  error(env);
	}
      }
    }
  }

  return NULL;
}

static emacs_value
readKey(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  brlapi_handle_t *const handle = env->get_user_ptr(env, args[0]);
  const int wait = nargs > 1? env->is_not_nil(env, args[1]): 0;
  brlapi_keyCode_t keyCode;
  int result;

  if (handle == NULL) return NULL;

  do {
    if (env->process_input(env) != emacs_process_input_continue)
      return NULL;

    result = brlapi__readKey(handle, wait, &keyCode);
  } while (result == -1 &&
           brlapi_errno == BRLAPI_ERROR_LIBCERR &&
           brlapi_libcerrno == EINTR);
  
  if (result == -1) {
    error(env);
    return NULL;
  }

  if (result == 0) return env->intern(env, "nil");

  return env->make_integer(env, (intmax_t)keyCode);
}

static emacs_value
readKeyWithTimeout(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  brlapi_handle_t *const handle = env->get_user_ptr(env, args[0]);
  const int timeout_ms = env->extract_integer(env, args[1]);
  brlapi_keyCode_t keyCode;
  int result;

  do {
    if (env->process_input(env) != emacs_process_input_continue)
      return NULL;

    result = brlapi__readKeyWithTimeout(handle, timeout_ms, &keyCode);
  } while (result == -1 &&
           timeout_ms < 0 &&
           brlapi_errno == BRLAPI_ERROR_LIBCERR &&
           brlapi_libcerrno == EINTR);
  
  if (result == -1) {
    error(env);
    return NULL;
  }

  if (result == 0) return env->intern(env, "nil");

  return env->make_integer(env, (intmax_t)keyCode);
}

static emacs_value
getStringParameter(brlapi_param_t param, emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  brlapi_handle_t *const handle = env->get_user_ptr(env, args[0]);
  char *string;
  size_t count;
  const brlapi_param_flags_t flags = BRLAPI_PARAMF_GLOBAL;

  if (handle == NULL) return NULL;

  string = brlapi__getParameterAlloc(handle, param, 0, flags, &count);

  if (string == NULL) {
    error(env);
    return NULL;
  }

  emacs_value result = env->make_string(env, string, count);
  free(string);

  return result;
}

static emacs_value
setStringParameter(
  brlapi_param_t param,
  emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data
) {
  brlapi_handle_t *const handle = env->get_user_ptr(env, args[0]);
  ptrdiff_t length;
  const brlapi_param_flags_t flags = BRLAPI_PARAMF_GLOBAL;

  if (handle == NULL) return NULL;

  if (env->copy_string_contents(env, args[1], NULL, &length)) {
    char string[length];

    if (!env->copy_string_contents(env, args[1], string, &length))
      return NULL;
  
    if (brlapi__setParameter(handle, param, 0, flags, string, length) != -1) {
      return env->intern(env, "nil");
    }

    error(env);
  }

  return NULL;
}

static emacs_value
getDriverCode(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  return getStringParameter(BRLAPI_PARAM_DRIVER_CODE, env, nargs, args, data);
}

static emacs_value
getDriverVersion(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  return getStringParameter(BRLAPI_PARAM_DRIVER_VERSION, env, nargs, args, data);
}

static emacs_value
getClipboardContent(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  return getStringParameter(BRLAPI_PARAM_CLIPBOARD_CONTENT, env, nargs, args, data);
}

static emacs_value
setClipboardContent(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  return setStringParameter(BRLAPI_PARAM_CLIPBOARD_CONTENT, env, nargs, args, data);
}

static emacs_value
getComputerBrailleTable(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  return getStringParameter(BRLAPI_PARAM_COMPUTER_BRAILLE_TABLE, env, nargs, args, data);
}

static emacs_value
setComputerBrailleTable(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  return setStringParameter(BRLAPI_PARAM_COMPUTER_BRAILLE_TABLE, env, nargs, args, data);
}

static emacs_value
getLiteraryBrailleTable(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  return getStringParameter(BRLAPI_PARAM_LITERARY_BRAILLE_TABLE, env, nargs, args, data);
}

static emacs_value
setLiteraryBrailleTable(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  return setStringParameter(BRLAPI_PARAM_LITERARY_BRAILLE_TABLE, env, nargs, args, data);
}

static emacs_value
getMessageLocae(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  return getStringParameter(BRLAPI_PARAM_MESSAGE_LOCALE, env, nargs, args, data);
}

static emacs_value
setMessageLocale(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  return setStringParameter(BRLAPI_PARAM_MESSAGE_LOCALE, env, nargs, args, data);
}

static emacs_value
closeConnection(emacs_env *env, ptrdiff_t nargs, emacs_value *args, void *data) {
  brlapi_handle_t *const handle = env->get_user_ptr(env, args[0]);

  if (handle == NULL) return NULL;

  brlapi__closeConnection(handle);

  return env->intern(env, "nil");
}

int
emacs_module_init(struct emacs_runtime *runtime) {
  emacs_env *const env = runtime->get_environment(runtime);

  if (runtime->size < sizeof(*runtime)) return 1;
  if (env->size < sizeof(*env)) return 2;

  { /* (define-error 'brlapi-error "BrlAPI Error") */
    emacs_value args[] = {
      env->intern(env, error_name),
      env->make_string(env, error_message, strlen(error_message))
    };

    env->funcall(env, env->intern(env, "define-error"), ARRAY_COUNT(args), args);
  }

#define register_function(func, minargs, maxargs, name, doc)                  \
  {                                                                           \
    emacs_value args[] = {                                                    \
      env->intern(env, "brlapi-" name),                                       \
      env->make_function(env, minargs, maxargs, func, doc, NULL)              \
    };                                                                        \
    env->funcall(env, env->intern(env, "defalias"), ARRAY_COUNT(args), args); \
  }

  register_function(openConnection, 0, 2, "open-connection",
    "Open a new connection to BRLTTY."
    "\n\nHOST is a string of the form \"host:0\" where the number after the colon"
    "\nspecifies the instance of BRLTTY on the given host."
    "\n\nIf AUTH is non-nil, specifies the path to the secret key."
    "\n\n(fn &optional HOST AUTH)"
  )
  register_function(getDriverName, 1, 1, "get-driver-name",
    "Get the complete name of the driver used by BRLTTY."
    "\n\n(fn CONNECTION)"
  )
  register_function(getModelIdentifier, 1, 1, "get-model-identifier",
    "Get model identifier."
    "\n\n(fn CONNECTION)"
  )
  register_function(getDisplaySize, 1, 1, "get-display-size",
    "Get the size of the braille display reachable via CONNECTION."
    "\n\nReturns a `cons' where `car' is the x dimension and `cdr' the y dimension."
    "\n\n(fn CONNECTION)"
  )
  register_function(enterTtyMode, 1, 3, "enter-tty-mode",
    "Enter TTY mode."
    "\n\n(fn CONNECTION &optional TTY DRIVER)"
  )
  register_function(leaveTtyMode, 1, 1, "leave-tty-mode",
    "Leave TTY mode."
    "\n\n(fn CONNECTION)"
  )
  register_function(writeText, 2, 3, "write-text",
    "Write STRING to the braille display."
    "\n\nCURSOR is either an integer or the symbol `leave'."
    "\n\n(fn CONNECTION STRING CURSOR)"
  )
  register_function(readKey, 1, 2, "read-key",
    "Read a keypress from CONNECTION."
    "\n\nIf WAIT is non-nil, wait until a keypress actually arrives."
    "\n\n(fn CONNECTION WAIT)"
  )
  register_function(readKeyWithTimeout, 2, 2, "read-key-with-timeout",
    "Read a keypress from CONNECTION waiting MILISECONDS."
    "\n\n(fn CONNECTION MILISECONDS)"
  )
  register_function(getDriverCode, 1, 1, "get-driver-code",
    "Get the driver code (a 2 letter string) of CONNECTION."
    "\n\n(fn CONNECTION)"
  )
  register_function(getDriverVersion, 1, 1, "get-driver-version",
    "Get the driver version(a string) of CONNECTION."
    "\n\n(fn CONNECTION)"
  )
  register_function(getClipboardContent, 1, 1, "get-clipboard-content",
    "Get the content of the clipboard of CONNECTION."
    "\n\n(fn CONNECTION)"
  )
  register_function(setClipboardContent, 2, 2, "set-clipboard-content",
    "Set the content of the clipboard of CONNECTION to STRING."
    "\n\n(fn CONNECTION STRING)"
  )
  register_function(getComputerBrailleTable, 1, 1, "get-computer-braille-table",
    "Get the computer braille table of CONNECTION."
    "\n\n(fn CONNECTION)"
  )
  register_function(setComputerBrailleTable, 2, 2, "set-computer-braille-table",
    "Set the computer braille table of CONNECTION to STRING."
    "\n\n(fn CONNECTION STRING)"
  )
  register_function(getLiteraryBrailleTable, 1, 1, "get-literary-table",
    "Get the literary braille table of CONNECTION."
    "\n\n(fn CONNECTION)"
  )
  register_function(setLiteraryBrailleTable, 2, 2, "set-literary-braille-table",
    "Set the literary braille table of CONNECTION to STRING."
    "\n\n(fn CONNECTION STRING)"
  )
  register_function(getMessageLocae, 1, 1, "get-message-locale",
    "Get the message locale of CONNECTION."
    "\n\n(fn CONNECTION)"
  )
  register_function(setMessageLocale, 2, 2, "set-message-locale",
    "Set the message locale of CONNECTION to STRING."
    "\n\n(fn CONNECTION STRING)"
  )
  register_function(closeConnection, 1, 1, "close-connection",
    "Close the connection."
    "\n\n(fn CONNECTION)"
  )
#undef register_function

  return 0;
}

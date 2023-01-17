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

#define BRLAPI_NO_SINGLE_SESSION 1
#include "prologue.h"
#include <brlapi.h>
#include <brl_cmds.h>
#include <errno.h>
#include <lua.h>
#include <lauxlib.h>

static const char *handle_t = "brlapi";
#define checkhandle(L, arg) (brlapi_handle_t *)luaL_checkudata(L, (arg), handle_t)

static void error(lua_State *L) {
  lua_pushstring(L, brlapi_strerror(&brlapi_error));
  lua_error(L);
}

static int openConnection(lua_State *L) {
  const brlapi_connectionSettings_t desiredSettings = {
    .host = luaL_optstring(L, 1, NULL), .auth = luaL_optstring(L, 2, NULL)
  };
  brlapi_connectionSettings_t actualSettings;
  brlapi_handle_t *const handle = (brlapi_handle_t *)lua_newuserdatauv(
    L, brlapi_getHandleSize(), 0
  );
  luaL_setmetatable(L, handle_t);

  if (brlapi__openConnection(handle, &desiredSettings, &actualSettings) == -1)
    error(L);

  lua_pushstring(L, actualSettings.host);
  lua_pushstring(L, actualSettings.auth);    

  return 3;
}

static int getFileDescriptor(lua_State *L) {
  const int fileDescriptor = brlapi__getFileDescriptor(checkhandle(L, 1));

  if (fileDescriptor == BRLAPI_INVALID_FILE_DESCRIPTOR) error(L);

  lua_pushinteger(L, fileDescriptor);

  return 1;
}

static int closeConnection(lua_State *L) {
  brlapi__closeConnection(checkhandle(L, 1));

  return 0;
}

static int getDriverName(lua_State *L) {
  char name[BRLAPI_MAXNAMELENGTH + 1];

  if (brlapi__getDriverName(checkhandle(L, 1), name, sizeof(name)) == -1)
    error(L);

  lua_pushstring(L, name);

  return 1;
}

static int getModelIdentifier(lua_State *L) {
  brlapi_handle_t *const handle = checkhandle(L, 1);
  char ident[BRLAPI_MAXNAMELENGTH + 1];

  if (brlapi__getModelIdentifier(handle, ident, sizeof(ident)) == -1) error(L);

  lua_pushstring(L, ident);

  return 1;
}

static int getDisplaySize(lua_State *L) {
  unsigned int x, y;

  if (brlapi__getDisplaySize(checkhandle(L, 1), &x, &y) == -1) error(L);

  lua_pushinteger(L, x), lua_pushinteger(L, y);

  return 2;
}

static int enterTtyMode(lua_State *L) {
  const int result = brlapi__enterTtyMode(checkhandle(L, 1),
    luaL_optinteger(L, 2, BRLAPI_TTY_DEFAULT), luaL_optstring(L, 3, NULL)
  );

  if (result == -1) error(L);

  lua_pushinteger(L, result);

  return 1;
}

static int leaveTtyMode(lua_State *L) {
  if (brlapi__leaveTtyMode(checkhandle(L, 1)) == -1) error(L);

  return 0;
}

static int setFocus(lua_State *L) {
  if (brlapi__setFocus(checkhandle(L, 1), luaL_checkinteger(L, 2)) == -1)
    error(L);

  return 0;
}

static int writeText(lua_State *L) {
  brlapi_handle_t *const handle = checkhandle(L, 1);
  const int cursor = luaL_checkinteger(L, 2);
  const char *const text = luaL_checkstring(L, 3);

  if (brlapi__writeText(handle, cursor, text) == -1) error(L);

  return 0;
}

static int writeDots(lua_State *L) {
  brlapi_handle_t *const handle = checkhandle(L, 1);
  const unsigned char *const dots = (const unsigned char *)luaL_checkstring(L, 2);

  if (brlapi__writeDots(handle, dots) == -1) error(L);

  return 0;
}

static int readKey(lua_State *L) {
  brlapi_handle_t *const handle = checkhandle(L, 1);
  const int wait = lua_toboolean(L, 2);
  brlapi_keyCode_t keyCode;
  int result;

  do {
    result = brlapi__readKey(handle, wait, &keyCode);
  } while (result == -1 &&
           brlapi_errno == BRLAPI_ERROR_LIBCERR && brlapi_libcerrno == EINTR);

  switch (result) {
  case -1:
    error(L);
    break; /* never reached */
  case 0:
    break;
  case 1:
    lua_pushinteger(L, (lua_Integer)keyCode);
   
    return 1;
  }

  return 0;
}

static int readKeyWithTimeout(lua_State *L) {
  brlapi_handle_t *const handle = checkhandle(L, 1);
  const int timeout_ms = luaL_checkinteger(L, 2);
  brlapi_keyCode_t keyCode;
  int result;

  do {
    result = brlapi__readKeyWithTimeout(handle, timeout_ms, &keyCode);
  } while (result == -1 &&
           brlapi_errno == BRLAPI_ERROR_LIBCERR && brlapi_libcerrno == EINTR);

  switch (result) {
  case -1:
    error(L);
    break; /* never reached */
  case 0:
    break;
  case 1:
    lua_pushinteger(L, (lua_Integer)keyCode);
 
    return 1;
  }

  return 0;
}

static int expandKeyCode(lua_State *L) {
  brlapi_keyCode_t keyCode = (brlapi_keyCode_t)luaL_checkinteger(L, 1);
  brlapi_expandedKeyCode_t expansion;

  brlapi_expandKeyCode(keyCode, &expansion);
  lua_pushinteger(L, expansion.type),
  lua_pushinteger(L, expansion.command),
  lua_pushinteger(L, expansion.argument),
  lua_pushinteger(L, expansion.flags);

  return 4;
}

static int describeKeyCode(lua_State *L) {
  brlapi_keyCode_t keyCode = (brlapi_keyCode_t)luaL_checkinteger(L, 1);
  brlapi_describedKeyCode_t description;

  brlapi_describeKeyCode(keyCode, &description);

  lua_pushstring(L, description.type),
  lua_pushstring(L, description.command),
  lua_pushinteger(L, description.argument);
  lua_createtable(L, description.flags, 0);
  for (size_t i = 0; i < description.flags; i += 1) {
    lua_pushstring(L, description.flag[i]);
    lua_rawseti(L, -2, i + 1);
  }

  return 4;
}

static int enterRawMode(lua_State *L) {
  brlapi_handle_t *const handle = checkhandle(L, 1);
  const char *const driverName = luaL_checkstring(L, 2);

  if (brlapi__enterRawMode(handle, driverName) == -1) error(L);

  return 0;
}

static int leaveRawMode(lua_State *L) {
  brlapi_handle_t *const handle = checkhandle(L, 1);

  if (brlapi__leaveRawMode(handle) == -1) error(L);

  return 0;
}

static int changeKeys(
  lua_State *L,
  int BRLAPI_STDCALL (*change) (brlapi_handle_t *, brlapi_rangeType_t, const brlapi_keyCode_t[], unsigned int)
) {
  brlapi_handle_t *const handle = checkhandle(L, 1);
  static const char *rangeNames[] = {
    "all", "code", "command", "key", "type", NULL
  };
  static const brlapi_rangeType_t rangeTypes[] = {
    brlapi_rangeType_all,
    brlapi_rangeType_code,
    brlapi_rangeType_command,
    brlapi_rangeType_key,
    brlapi_rangeType_type
  };
  const brlapi_rangeType_t range = rangeTypes[
    luaL_checkoption(L, 2, NULL, rangeNames)
  ];

  if (range == brlapi_rangeType_all) {
    if (change(handle, range, NULL, 0) == -1) error(L);

    return 0;
  }

  luaL_checktype(L, 3, LUA_TTABLE);
  lua_len(L, 3);

  {
    const size_t count = lua_tointeger(L, -1);
    brlapi_keyCode_t keys[count];
    lua_pop(L, 1);

    for (size_t i = 0; i < count; i += 1) {
      lua_geti(L, 3, i + 1);
      keys[i] = (brlapi_keyCode_t)luaL_checkinteger(L, -1);
      lua_pop(L, 1);
    }

    if (change(handle, range, keys, count) == -1) error(L);
  }
 
  return 0;
}

static int acceptKeys(lua_State *L) {
  return changeKeys(L, brlapi__acceptKeys);
}

static int ignoreKeys(lua_State *L) {
  return changeKeys(L, brlapi__ignoreKeys);
}

static int acceptAllKeys(lua_State *L) {
  if (brlapi__acceptAllKeys(checkhandle(L, 1)) == -1) error(L);

  return 0;
}

static int ignoreAllKeys(lua_State *L) {
  if (brlapi__ignoreAllKeys(checkhandle(L, 1)) == -1) error(L);

  return 0;
}

static int suspendDriver(lua_State *L) {
  brlapi_handle_t *const handle = checkhandle(L, 1);
  const char *const driverName = luaL_checkstring(L, 2);

  if (brlapi__suspendDriver(handle, driverName) == -1) error(L);

  return 0;
}

static int resumeDriver(lua_State *L) {
  if (brlapi__resumeDriver(checkhandle(L, 1)) == -1) error(L);

  return 0;
}

static int pause_(lua_State *L) {
  brlapi_handle_t *const handle = checkhandle(L, 1);
  const int timeout_ms = luaL_checkinteger(L, 2);
  int result;

  do {
    result = brlapi__pause(handle, timeout_ms);
  } while (timeout_ms == -1 && result == -1 &&
           brlapi_errno == BRLAPI_ERROR_LIBCERR && brlapi_libcerrno == EINTR);

  if (result == -1) error(L);

  return 0;
}

static int getBooleanParameter(lua_State *L, brlapi_param_t param) {
  brlapi_param_bool_t value;
  const int result = brlapi__getParameter(checkhandle(L, 1),
    param, 0, BRLAPI_PARAMF_GLOBAL, &value, sizeof(value)
  );

  if (result == -1) error(L);

  lua_pushboolean(L, value);

  return 1;
}

static int setBooleanParameter(lua_State *L, brlapi_param_t param) {
  brlapi_handle_t *const handle = checkhandle(L, 1);
  const brlapi_param_flags_t flags = BRLAPI_PARAMF_GLOBAL;
  const brlapi_param_bool_t value = lua_toboolean(L, 2);

  if (brlapi__setParameter(handle, param, 0, flags, &value, sizeof(value)) == -1)
    error(L);

  return 0;
}

static int getStringParameter(
  lua_State *L,
  brlapi_param_t param, brlapi_param_subparam_t subparam
) {
  size_t count;
  char *string = brlapi__getParameterAlloc(checkhandle(L, 1),
    param, subparam, BRLAPI_PARAMF_GLOBAL, &count
  );

  if (string == NULL) error(L);

  lua_pushlstring(L, string, count);
  free(string);

  return 1;
}

static int setStringParameter(
  lua_State *L,
  brlapi_param_t param, brlapi_param_subparam_t subparam
) {
  brlapi_handle_t *const handle = checkhandle(L, 1);
  size_t length;
  const brlapi_param_flags_t flags = BRLAPI_PARAMF_GLOBAL;
  const char *string = luaL_checklstring(L, 2, &length);

  if (brlapi__setParameter(handle, param, subparam, flags, string, length) == -1)
    error(L);

  return 0;
}

static int getDriverCode(lua_State *L) {
  return getStringParameter(L, BRLAPI_PARAM_DRIVER_CODE, 0);
}

static int getDriverVersion(lua_State *L) {
  return getStringParameter(L, BRLAPI_PARAM_DRIVER_VERSION, 0);
}

static int getDeviceOnline(lua_State *L) {
  return getBooleanParameter(L, BRLAPI_PARAM_DEVICE_ONLINE);
}

static int getAudibleAlerts(lua_State *L) {
  return getBooleanParameter(L, BRLAPI_PARAM_AUDIBLE_ALERTS);
}

static int setAudibleAlerts(lua_State *L) {
  return setBooleanParameter(L, BRLAPI_PARAM_AUDIBLE_ALERTS);
}

static int getClipboardContent(lua_State *L) {
  return getStringParameter(L, BRLAPI_PARAM_CLIPBOARD_CONTENT, 0);
}

static int setClipboardContent(lua_State *L) {
  return setStringParameter(L, BRLAPI_PARAM_CLIPBOARD_CONTENT, 0);
}

static int getComputerBrailleTable(lua_State *L) {
  return getStringParameter(L, BRLAPI_PARAM_COMPUTER_BRAILLE_TABLE, 0);
}

static int setComputerBrailleTable(lua_State *L) {
  return setStringParameter(L, BRLAPI_PARAM_COMPUTER_BRAILLE_TABLE, 0);
}

static int getLiteraryBrailleTable(lua_State *L) {
  return getStringParameter(L, BRLAPI_PARAM_LITERARY_BRAILLE_TABLE, 0);
}

static int setLiteraryBrailleTable(lua_State *L) {
  return setStringParameter(L, BRLAPI_PARAM_LITERARY_BRAILLE_TABLE, 0);
}

static int getMessageLocale(lua_State *L) {
  return getStringParameter(L, BRLAPI_PARAM_MESSAGE_LOCALE, 0);
}

static int setMessageLocale(lua_State *L) {
  return setStringParameter(L, BRLAPI_PARAM_MESSAGE_LOCALE, 0);
}

static int getBoundCommandKeycodes(lua_State *L) {
  size_t count;
  brlapi_keyCode_t *codes = brlapi__getParameterAlloc(checkhandle(L, 1),
    BRLAPI_PARAM_BOUND_COMMAND_KEYCODES, 0, BRLAPI_PARAMF_GLOBAL, &count
  );

  if (codes == NULL) error(L);

  count /= sizeof(brlapi_keyCode_t);

  lua_newtable(L);
  for (size_t i = 0; i < count; i += 1) {
    lua_pushinteger(L, (lua_Integer)codes[i]);
    lua_rawseti(L, -2, i + 1);
  }

  free(codes);

  return 1;
}

static int getCommandKeycodeName(lua_State *L) {
  const brlapi_keyCode_t keyCode = (brlapi_keyCode_t)luaL_checkinteger(L, 2);
  return getStringParameter(L, BRLAPI_PARAM_COMMAND_KEYCODE_NAME, keyCode);
}

static int getCommandKeycodeSummary(lua_State *L) {
  const brlapi_keyCode_t keyCode = (brlapi_keyCode_t)luaL_checkinteger(L, 2);
  return getStringParameter(L, BRLAPI_PARAM_COMMAND_KEYCODE_SUMMARY, keyCode);
}

static int getDefinedDriverKeycodes(lua_State *L) {
  size_t count;
  brlapi_keyCode_t *codes = brlapi__getParameterAlloc(checkhandle(L, 1),
    BRLAPI_PARAM_DEFINED_DRIVER_KEYCODES, 0, BRLAPI_PARAMF_GLOBAL, &count
  );

  if (codes == NULL) error(L);

  count /= sizeof(brlapi_keyCode_t);

  lua_newtable(L);
  for (size_t i = 0; i < count; i += 1) {
    lua_pushinteger(L, (lua_Integer)codes[i]);
    lua_rawseti(L, -2, i + 1);
  }

  free(codes);

  return 1;
}

static int getDriverKeycodeName(lua_State *L) {
  const brlapi_keyCode_t keyCode = (brlapi_keyCode_t)luaL_checkinteger(L, 2);
  return getStringParameter(L, BRLAPI_PARAM_DRIVER_KEYCODE_NAME, keyCode);
}

static int getDriverKeycodeSummary(lua_State *L) {
  const brlapi_keyCode_t keyCode = (brlapi_keyCode_t)luaL_checkinteger(L, 2);
  return getStringParameter(L, BRLAPI_PARAM_DRIVER_KEYCODE_SUMMARY, keyCode);
}

static inline int versionGetField(lua_State *L, const char *name) {
  if (lua_getfield(L, 1, name) == LUA_TNUMBER) {
    if (lua_getfield(L, 2, name) == LUA_TNUMBER) {
      return 1;
    }
  }

  return 0;
}

static int versionCompare(lua_State *L, int op) {
  int result = 0;

  if (versionGetField(L, "major")) {
    if (lua_compare(L, -2, -1, LUA_OPLT)) {
      result = 1;
    } else if (lua_compare(L, -2, -1, LUA_OPEQ)) {
      if (versionGetField(L, "minor")) {
	if (lua_compare(L, -2, -1, LUA_OPLT)) {
	  result = 1;
	} else if (lua_compare(L, -2, -1, LUA_OPEQ)) {
	  if (versionGetField(L, "revision")) {
	    if (lua_compare(L, -2, -1, op)) {
	      result = 1;
	    }
	  }
	}
      }
    }
  }

  lua_pushboolean(L, result);
  return 1;
}

static int versionLT(lua_State *L) {
  return versionCompare(L, LUA_OPLT);
}

static int versionLE(lua_State *L) {
  return versionCompare(L, LUA_OPLE);
}

static int versionString(lua_State *L) {
  static const char *separator = ".";

  lua_getfield(L, 1, "major");
  lua_pushstring(L, separator);
  lua_getfield(L, 1, "minor");
  lua_pushstring(L, separator);
  lua_getfield(L, 1, "revision");

  lua_concat(L, 5);

  return 1;
}

static const luaL_Reg version_meta[] = {
  { "__lt", versionLT },
  { "__le", versionLE },
  { "__tostring", versionString },
  { NULL, NULL }
};

static const luaL_Reg meta[] = {
  { "__close", closeConnection },
  { NULL, NULL }
};

static const luaL_Reg funcs[] = {
  { "openConnection", openConnection },
  { "getFileDescriptor", getFileDescriptor },
  { "closeConnection", closeConnection },
  { "getDriverName", getDriverName },
  { "getModelIdentifier", getModelIdentifier },
  { "getDisplaySize", getDisplaySize },
  { "enterTtyMode", enterTtyMode },
  { "leaveTtyMode", leaveTtyMode },
  { "setFocus", setFocus },
  { "writeText", writeText },
  { "writeDots", writeDots },
  { "readKey", readKey },
  { "readKeyWithTimeout", readKeyWithTimeout },
  { "expandKeyCode", expandKeyCode },
  { "describeKeyCode", describeKeyCode },
  { "enterRawMode", enterRawMode },
  { "leaveRawMode", leaveRawMode },
  { "acceptKeys", acceptKeys },
  { "ignoreKeys", ignoreKeys },
  { "acceptAllKeys", acceptAllKeys },
  { "ignoreAllKeys", ignoreAllKeys },
  { "suspendDriver", suspendDriver },
  { "resumeDriver", resumeDriver },
  { "pause", pause_ },
  { "getDriverCode", getDriverCode },
  { "getDriverVersion", getDriverVersion },
  { "getDeviceOnline", getDeviceOnline },
  { "getAudibleAlerts", getAudibleAlerts },
  { "setAudibleAlerts", setAudibleAlerts },
  { "getClipboardContent", getClipboardContent },
  { "setClipboardContent", setClipboardContent },
  { "getComputerBrailleTable", getComputerBrailleTable },
  { "setComputerBrailleTable", setComputerBrailleTable },
  { "getLiteraryBrailleTable", getLiteraryBrailleTable },
  { "setLiteraryBrailleTable", setLiteraryBrailleTable },
  { "getMessageLocale", getMessageLocale },
  { "setMessageLocale", setMessageLocale },
  { "getBoundCommandKeycodes", getBoundCommandKeycodes },
  { "getCommandKeycodeName", getCommandKeycodeName },
  { "getCommandKeycodeSummary", getCommandKeycodeSummary },
  { "getDefinedDriverKeycodes", getDefinedDriverKeycodes },
  { "getDriverKeycodeName", getDriverKeycodeName },
  { "getDriverKeycodeSummary", getDriverKeycodeSummary },
  { NULL, NULL }
};

static void createcmdtable(lua_State *L) {
  lua_newtable(L);
#define CMD(name, value) lua_pushinteger(L, (lua_Integer)value), lua_setfield(L, -2, STRINGIFY(name));
#include "cmd.auto.h"
#undef CMD
}

int luaopen_brlapi(lua_State *L) {
  luaL_newmetatable(L, handle_t);
  luaL_setfuncs(L, meta, 0);
  luaL_newlib(L, funcs);

  {
    int major, minor, revision;

    brlapi_getLibraryVersion(&major, &minor, &revision);

    lua_createtable(L, 0, 3);
    lua_pushinteger(L, major), lua_setfield(L, -2, "major");
    lua_pushinteger(L, minor), lua_setfield(L, -2, "minor");
    lua_pushinteger(L, revision), lua_setfield(L, -2, "revision");
    luaL_newmetatable(L, "version");
    luaL_setfuncs(L, version_meta, 0);
    lua_setmetatable(L, -2);
    lua_setfield(L, -2, "version");
  }

  createcmdtable(L);
  lua_setfield(L, -2, "CMD");

  /* Set function table as metatable __index */
  lua_pushvalue(L, -1), lua_setfield(L, -3, "__index");

  return 1;
}

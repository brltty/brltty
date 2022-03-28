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

#define BRLAPI_NO_SINGLE_SESSION 1
#include "prologue.h"
#include <brlapi.h>
#include <errno.h>
#include <lua.h>
#include <lauxlib.h>

static const char *handle_t = "brlapi";
#define checkhandle(L, arg) (brlapi_handle_t *)luaL_checkudata(L, (arg), handle_t)

static void error(lua_State *L) {
  lua_pushstring(L, brlapi_strerror(&brlapi_error));
  lua_error(L);
}

static int getLibraryVersion(lua_State *L) {
  int major, minor, revision;
  brlapi_getLibraryVersion(&major, &minor, &revision);

  lua_pushinteger(L, major),
  lua_pushinteger(L, minor),
  lua_pushinteger(L, revision);
    
  return 3;
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
    luaL_checkinteger(L, 2), luaL_optstring(L, 3, NULL)
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

static const luaL_Reg meta[] = {
  { "__close", closeConnection },
  { NULL, NULL }
};

static const luaL_Reg funcs[] = {
  { "getLibraryVersion", getLibraryVersion },
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
  { NULL, NULL }
};

static void createcmdtable(lua_State *L) {
  lua_newtable(L);
#define CMD(name) lua_pushinteger(L, CONCATENATE(BRLAPI_KEY_CMD_, name)), lua_setfield(L, -2, STRINGIFY(name))
  CMD(NOOP);
  CMD(LNUP);
  CMD(LNDN);
  CMD(WINUP);
  CMD(WINDN);
  CMD(PRDIFLN);
  CMD(NXDIFLN);
  CMD(ATTRUP);
  CMD(ATTRDN);
  CMD(TOP);
  CMD(BOT);
  CMD(TOP_LEFT);
  CMD(BOT_LEFT);
  CMD(PRPGRPH);
  CMD(NXPGRPH);
  CMD(PRPROMPT);
  CMD(NXPROMPT);
  CMD(PRSEARCH);
  CMD(NXSEARCH);
  CMD(CHRLT);
  CMD(CHRRT);
  CMD(HWINLT);
  CMD(HWINRT);
  CMD(FWINLT);
  CMD(FWINRT);
  CMD(FWINLTSKIP);
  CMD(FWINRTSKIP);
  CMD(LNBEG);
  CMD(LNEND);
  CMD(HOME);
  CMD(BACK);
  CMD(RETURN);
  CMD(FREEZE);
  CMD(DISPMD);
  CMD(SIXDOTS);
  CMD(SLIDEWIN);
  CMD(SKPIDLNS);
  CMD(SKPBLNKWINS);
  CMD(CSRVIS);
  CMD(CSRHIDE);
  CMD(CSRTRK);
  CMD(CSRSIZE);
  CMD(CSRBLINK);
  CMD(ATTRVIS);
  CMD(ATTRBLINK);
  CMD(CAPBLINK);
  CMD(TUNES);
  CMD(AUTOREPEAT);
  CMD(AUTOSPEAK);
  CMD(HELP);
  CMD(INFO);
  CMD(LEARN);
  CMD(PREFMENU);
  CMD(PREFSAVE);
  CMD(PREFLOAD);
  CMD(MENU_FIRST_ITEM);
  CMD(MENU_LAST_ITEM);
  CMD(MENU_PREV_ITEM);
  CMD(MENU_NEXT_ITEM);
  CMD(MENU_PREV_SETTING);
  CMD(MENU_NEXT_SETTING);
  CMD(MUTE);
  CMD(SPKHOME);
  CMD(SAY_LINE);
  CMD(SAY_ABOVE);
  CMD(SAY_BELOW);
  CMD(SAY_SLOWER);
  CMD(SAY_FASTER);
  CMD(SAY_SOFTER);
  CMD(SAY_LOUDER);
  CMD(SWITCHVT_PREV);
  CMD(SWITCHVT_NEXT);
  CMD(CSRJMP_VERT);
  CMD(PASTE);
  CMD(RESTARTBRL);
  CMD(RESTARTSPEECH);
  CMD(OFFLINE);
  CMD(SHIFT);
  CMD(UPPER);
  CMD(CONTROL);
  CMD(META);
  CMD(TIME);
  CMD(MENU_PREV_LEVEL);
  CMD(ASPK_SEL_LINE);
  CMD(ASPK_SEL_CHAR);
  CMD(ASPK_INS_CHARS);
  CMD(ASPK_DEL_CHARS);
  CMD(ASPK_REP_CHARS);
  CMD(ASPK_CMP_WORDS);
  CMD(SPEAK_CURR_CHAR);
  CMD(SPEAK_PREV_CHAR);
  CMD(SPEAK_NEXT_CHAR);
  CMD(SPEAK_CURR_WORD);
  CMD(SPEAK_PREV_WORD);
  CMD(SPEAK_NEXT_WORD);
  CMD(SPEAK_CURR_LINE);
  CMD(SPEAK_PREV_LINE);
  CMD(SPEAK_NEXT_LINE);
  CMD(SPEAK_FRST_CHAR);
  CMD(SPEAK_LAST_CHAR);
  CMD(SPEAK_FRST_LINE);
  CMD(SPEAK_LAST_LINE);
  CMD(DESC_CURR_CHAR);
  CMD(SPELL_CURR_WORD);
  CMD(ROUTE_CURR_LOCN);
  CMD(SPEAK_CURR_LOCN);
  CMD(SHOW_CURR_LOCN);
  CMD(CLIP_SAVE);
  CMD(CLIP_RESTORE);
  CMD(BRLUCDOTS);
  CMD(BRLKBD);
  CMD(UNSTICK);
  CMD(ALTGR);
  CMD(GUI);
  CMD(BRL_STOP);
  CMD(BRL_START);
  CMD(SPK_STOP);
  CMD(SPK_START);
  CMD(SCR_STOP);
  CMD(SCR_START);
  CMD(SELECTVT_PREV);
  CMD(SELECTVT_NEXT);
  CMD(PRNBWIN);
  CMD(NXNBWIN);
  CMD(TOUCH_NAV);
  CMD(SPEAK_INDENT);
  CMD(ASPK_INDENT);
  CMD(REFRESH);
  CMD(INDICATORS);
  CMD(TXTSEL_CLEAR);
  CMD(TXTSEL_ALL);
  CMD(HOST_COPY);
  CMD(HOST_CUT);
  CMD(HOST_PASTE);
  CMD(GUI_TITLE);
  CMD(GUI_BRL_ACTIONS);
  CMD(GUI_HOME);
  CMD(GUI_BACK);
  CMD(GUI_DEV_SETTINGS);
  CMD(GUI_DEV_OPTIONS);
  CMD(GUI_APP_LIST);
  CMD(GUI_APP_MENU);
  CMD(GUI_APP_ALERTS);
  CMD(GUI_AREA_ACTV);
  CMD(GUI_AREA_PREV);
  CMD(GUI_AREA_NEXT);
  CMD(GUI_ITEM_FRST);
  CMD(GUI_ITEM_PREV);
  CMD(GUI_ITEM_NEXT);
  CMD(GUI_ITEM_LAST);
  CMD(ROUTE);
  CMD(CLIP_NEW);
  CMD(CUTBEGIN);
  CMD(CLIP_ADD);
  CMD(CUTAPPEND);
  CMD(COPY_RECT);
  CMD(CUTRECT);
  CMD(COPY_LINE);
  CMD(CUTLINE);
  CMD(SWITCHVT);
  CMD(PRINDENT);
  CMD(NXINDENT);
  CMD(DESCCHAR);
  CMD(SETLEFT);
  CMD(SETMARK);
  CMD(GOTOMARK);
  CMD(GOTOLINE);
  CMD(PRDIFCHAR);
  CMD(NXDIFCHAR);
  CMD(CLIP_COPY);
  CMD(COPYCHARS);
  CMD(CLIP_APPEND);
  CMD(APNDCHARS);
  CMD(PASTE_HISTORY);
  CMD(SET_TEXT_TABLE);
  CMD(SET_ATTRIBUTES_TABLE);
  CMD(SET_CONTRACTION_TABLE);
  CMD(SET_KEYBOARD_TABLE);
  CMD(SET_LANGUAGE_PROFILE);
  CMD(ROUTE_LINE);
  CMD(REFRESH_LINE);
  CMD(TXTSEL_START);
  CMD(TXTSEL_SET);
  CMD(SELECTVT);
  CMD(ALERT);
  CMD(PASSDOTS);
  CMD(PASSAT);
  CMD(PASSXT);
  CMD(PASSPS2);
  CMD(CONTEXT);
  CMD(TOUCH_AT);
#undef CMD
}

int luaopen_brlapi(lua_State *L) {
  luaL_newmetatable(L, handle_t);
  luaL_setfuncs(L, meta, 0);
  luaL_newlib(L, funcs);

  createcmdtable(L);
  lua_setfield(L, -2, "CMD");

  /* Set function table as metatable __index */
  lua_pushvalue(L, -1), lua_setfield(L, -3, "__index");

  return 1;
}

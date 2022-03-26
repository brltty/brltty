#define BRLAPI_NO_SINGLE_SESSION 1
#include <brlapi.h>
#include <lua.h>
#include <lauxlib.h>

static const char *handle_t = "brlapi";
#define checkhandle(L, arg) (brlapi_handle_t *)luaL_checkudata(L, (arg), handle_t)

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
    brlapi_handle_t *const handle = (brlapi_handle_t *)lua_newuserdatauv(L,
        brlapi_getHandleSize(), 0
    );
    luaL_setmetatable(L, handle_t);

    if (brlapi__openConnection(handle, &desiredSettings, &actualSettings) == -1) {
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
    }

    lua_pushstring(L, actualSettings.host);
    lua_pushstring(L, actualSettings.auth);    

    return 3;
}

static int closeConnection(lua_State *L) {
    brlapi__closeConnection(checkhandle(L, 1));

    return 0;
}

static int getDisplaySize(lua_State *L) {
    unsigned int x, y;

    if (brlapi__getDisplaySize(checkhandle(L, 1), &x, &y) == -1) {
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
    }

    lua_pushinteger(L, x), lua_pushinteger(L, y);

    return 2;
}

static int getDriverName(lua_State *L) {
    char name[BRLAPI_MAXNAMELENGTH + 1];

    if (brlapi__getDriverName(checkhandle(L, 1), name, sizeof(name)) == -1) {
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
    }

    lua_pushstring(L, name);

    return 1;
}

static int getModelIdentifier(lua_State *L) {
    brlapi_handle_t *const handle = checkhandle(L, 1);
    char name[BRLAPI_MAXNAMELENGTH + 1];

    if (brlapi__getModelIdentifier(handle, name, sizeof(name)) == -1) {
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
    }

    lua_pushstring(L, name);

    return 1;
}

static int enterTtyMode(lua_State *L) {
    int result = brlapi__enterTtyMode(checkhandle(L, 1),
        luaL_checkinteger(L, 2), luaL_optstring(L, 3, NULL)
    );

    if (result == -1) {
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
    }

    lua_pushinteger(L, result);

    return 1;
}

static int leaveTtyMode(lua_State *L) {
    if (brlapi__leaveTtyMode(checkhandle(L, 1)) == -1) {
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
    }

    return 0;
}

static int setFocus(lua_State *L) {
    if (brlapi__setFocus(checkhandle(L, 1), luaL_checkinteger(L, 2)) == -1) {
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
    }

    return 0;
}

static int writeText(lua_State *L) {
    brlapi_handle_t *const handle = checkhandle(L, 1);
    const int cursor = luaL_checkinteger(L, 2);
    const char *const text = luaL_checkstring(L, 3);

    if (brlapi__writeText(handle, cursor, text) == -1) {
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
    }

    return 0;
}

static int writeDots(lua_State *L) {
    brlapi_handle_t *const handle = checkhandle(L, 1);
    const unsigned char *const dots = (const unsigned char *)luaL_checkstring(L, 2);

    if (brlapi__writeDots(handle, dots) == -1) {
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
    }

    return 0;
}

static int readKey(lua_State *L) {
    brlapi_handle_t *const handle = checkhandle(L, 1);
    brlapi_keyCode_t keyCode;

    switch (brlapi__readKey(handle, lua_toboolean(L, 2), &keyCode)) {
    case -1:
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
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
    brlapi_keyCode_t keyCode;

    switch (brlapi__readKeyWithTimeout(handle, luaL_checkinteger(L, 2), &keyCode)) {
    case -1:
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
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

    if (brlapi__enterRawMode(handle, driverName) == -1) {
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
    }

    return 0;
}

static int leaveRawMode(lua_State *L) {
    brlapi_handle_t *handle = (brlapi_handle_t *)luaL_checkudata(L, 1, handle_t);

    if (brlapi__leaveRawMode(handle) == -1) {
        lua_pushstring(L, brlapi_strerror(&brlapi_error));
        lua_error(L);
    }

    return 0;
}

static const luaL_Reg meta[] = {
    { "__close", closeConnection },
    { NULL, NULL }
};

static const luaL_Reg funcs[] = {
    { "getLibraryVersion", getLibraryVersion },
    { "openConnection", openConnection },
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
    { NULL, NULL }
};

int luaopen_brlapi(lua_State *L) {
    luaL_newmetatable(L, handle_t);
    luaL_setfuncs(L, meta, 0);
    luaL_newlib(L, funcs);

    /* Set function table as metatable __index */
    lua_pushvalue(L, -1), lua_setfield(L, -3, "__index");

    return 1;
}

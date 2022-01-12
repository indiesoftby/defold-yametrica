

#include <dmsdk/sdk.h>

#include "cjson/cjson.h"

#include <cstdlib>

#if defined(DM_PLATFORM_HTML5)

extern "C" void YaMetrica_Hit(const int counterId, const char* url, const char* options, const int options_len);
extern "C" void YaMetrica_NotBounce(const int counterId, const char* options, const int options_len);
extern "C" void YaMetrica_Params(const int counterId, const char* visit_params, const int visit_params_len, const char* goal_params, const int goal_params_len);
extern "C" void YaMetrica_ReachGoal(const int counterId, const char* target, const char* params, const int params_len);
extern "C" void YaMetrica_UserParams(const int counterId, const char* params, const int params_len);

static int g_CounterId;

static int Hit(lua_State* L)
{
    const char* url = luaL_checkstring(L, 1);

    const char* options = 0;
    int options_len     = 0;
    if (lua_istable(L, 2))
    {
        options = YaMetrica_CJSON_Encode(L, 2, &options_len, false);
    }

    YaMetrica_Hit(g_CounterId, url, options, options_len);

    return 0;
}

static int NotBounce(lua_State* L)
{
    const char* options = 0;
    int options_len     = 0;
    if (lua_istable(L, 1))
    {
        options = YaMetrica_CJSON_Encode(L, 1, &options_len, false);
    }

    YaMetrica_NotBounce(g_CounterId, options, options_len);

    return 0;
}

static int Params(lua_State* L)
{
    if (!lua_istable(L, 1))
    {
        luaL_typerror(L, 1, lua_typename(L, LUA_TTABLE));
    }
    else
    {
        int visit_params_len     = 0;
        const char* visit_params = YaMetrica_CJSON_Encode(L, 1, &visit_params_len, true);

        int goal_params_len     = 0;
        const char* goal_params = 0;
        if (lua_istable(L, 2))
        {
            goal_params = YaMetrica_CJSON_Encode(L, 2, &goal_params_len, false);
        }

        YaMetrica_Params(g_CounterId, visit_params, visit_params_len, goal_params, goal_params_len);

        free((void*)visit_params);
    }

    return 0;
}

static int ReachGoal(lua_State* L)
{
    const char* target = luaL_checkstring(L, 1);

    const char* params = 0;
    int params_len     = 0;
    if (lua_istable(L, 2))
    {
        params = YaMetrica_CJSON_Encode(L, 2, &params_len, false);
    }

    YaMetrica_ReachGoal(g_CounterId, target, params, params_len);

    return 0;
}

static int UserParams(lua_State* L)
{
    if (!lua_istable(L, 1))
    {
        luaL_typerror(L, 1, lua_typename(L, LUA_TTABLE));
        return 0;
    }

    int params_len     = 0;
    const char* params = YaMetrica_CJSON_Encode(L, 1, &params_len, false);

    YaMetrica_UserParams(g_CounterId, params, params_len);

    return 0;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] = {
    { "hit", Hit },
    { "not_bounce", NotBounce },
    { "params", Params },
    { "reach_goal", ReachGoal },
    { "user_params", UserParams },
    /* Sentinel: */
    { NULL, NULL }
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, "yametrica", Module_methods);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

#endif

dmExtension::Result AppInitializeExt(dmExtension::AppParams* params)
{
#if defined(DM_PLATFORM_HTML5)
    dmConfigFile::HConfig config_file = dmEngine::GetConfigFile(params);

    g_CounterId = dmConfigFile::GetInt(config_file, "yametrica.counter_id", 0);
#endif

    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializeExt(dmExtension::Params* params)
{
#if defined(DM_PLATFORM_HTML5)
    LuaInit(params->m_L);
#endif

    YaMetrica_CJSON_Init();

    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeExt(dmExtension::Params* params)
{
    YaMetrica_CJSON_Final();

    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(yametrica, "yametrica", AppInitializeExt, 0, InitializeExt, 0, 0, FinalizeExt)

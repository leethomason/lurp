#include "scripthelper.h"
#include "lua.hpp"
#include "scriptbridge.h"
#include "util.h"
#include "coredata.h"

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <plog/Log.h>

namespace lurp {

ScriptHelper::ScriptHelper(
	ScriptBridge& bridge,
	CoreData& coreData,
	const ScriptEnv& env) :
	_bridge(bridge),
	_coreData(coreData),
	_scriptEnv(env)
{
	setupScriptEnv();
}

ScriptHelper::~ScriptHelper()
{
	assert(_scriptContextCount == 0);
	tearDownScriptEnv();
}

void ScriptHelper::setupScriptEnv()
{
	lua_State* L = _bridge.getLuaState();
	ScriptBridge::LuaStackCheck check(L);

	// Check if the context is already set up
	int exists = lua_getglobal(L, "script");
	CHECK(exists == LUA_TNIL);
	lua_pop(L, 1);

	// Now push the call params
	int t = lua_getglobal(L, "SetupScriptEnv");
	CHECK(t == LUA_TFUNCTION);

	lua_pushstring(L, _SCRIPTENV);
	if (_scriptEnv.npc.empty())
		lua_pushnil(L);
	else
		lua_pushstring(L, _scriptEnv.npc.c_str());

	if (_scriptEnv.zone.empty())
		lua_pushnil(L);
	else
		lua_pushstring(L, _scriptEnv.zone.c_str());

	if (_scriptEnv.room.empty())
		lua_pushnil(L);
	else
		lua_pushstring(L, _scriptEnv.room.c_str());
	pcall(-1, 4, 0);

	// Basic check everything is okay:
	t = lua_getglobal(L, "script");
	CHECK(t == LUA_TTABLE);
	lua_pop(L, 1);
}

void ScriptHelper::tearDownScriptEnv()
{
	lua_State* L = _bridge.getLuaState();
	ScriptBridge::LuaStackCheck check(L);

	int t = lua_getglobal(L, "ClearScriptEnv");
	CHECK(t == LUA_TFUNCTION);
	pcall(-1, 0, 0);

	_coreData.clearScriptEnv();
}

#if 0
void ScriptHelper::pushScriptContext()
{
	// FIXME?
	// This could obviously be implemented as a stack. Each
	// script gets a new context, and variables are scoped
	// and don't collide. BUT, that also means introducing
	// a "holder" script isolates it. Trying with one scritpt
	// context. Up for debate.
	_scriptContextCount++;
}

void ScriptHelper::popScriptContext()
{
	_scriptContextCount--;
}
#endif

bool ScriptHelper::call(int ref, int nResult) const
{
	if (ref < 0) {
		return true;
	}

	lua_State* L = _bridge.getLuaState();
	ScriptBridge::LuaStackCheck check(L);
	ScriptBridge::FuncInfo fi = _bridge.getFuncInfo(ref);

	// Push the function first, then the args. Who knew.
	lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
	assert(lua_type(L, -1) == LUA_TFUNCTION);

	return pcall(ref, fi.nParams, nResult);
}

bool ScriptHelper::callGlobal(const std::string& funcName, const std::vector<std::string>& args, int nResult) const
{
	lua_State* L = _bridge.getLuaState();
	ScriptBridge::LuaStackCheck check(L);

	int t = lua_getglobal(L, funcName.c_str());
	CHECK(t == LUA_TFUNCTION);

	for (const std::string& arg : args) {
		if (arg.empty())
			lua_pushnil(L);
		else
			lua_pushstring(L, arg.c_str());
	}
	return pcall(-1, (int)args.size(), nResult);
}

bool ScriptHelper::pcall(int funcRef, int nArgs, int nResult) const
{
	ScriptBridge::FuncInfo fi;
	if (funcRef >= 0) {
		fi = _bridge.getFuncInfo(funcRef);
		assert(fi.nParams == nArgs);
	}

	lua_State* L = _bridge.getLuaState();
	int err = lua_pcall(L, nArgs, nResult, 0);
	std::string errStr = "";
	if (err == LUA_ERRRUN) errStr = "ERRRUN";
	else if (err == LUA_ERRERR) errStr = "LUA_ERRERR";

	if (err) {
		PLOG(plog::warning) << fmt::format("Lua error {} '{}' from '{}' at line {}", err, errStr, fi.srcName, fi.srcLine);
		std::string e = lua_tostring(L, -1);
		PLOG(plog::warning) << fmt::format("Msg: {}", e);
		assert(false);
	}
	bool r = false;
	Variant vr = Variant::fromLua(L, -1);
	if (nResult) {
		r = lua_toboolean(L, -1) != 0;
	}
	lua_pop(L, nResult);

	if (funcRef >= 0) {
		PLOG(plog::debug) << fmt::format("[TRACE] Call to {} at line {} returned {}. ", fi.srcName, fi.srcLine, vr.toLuaString());
	}

	return r;
}

} // namespace lurp

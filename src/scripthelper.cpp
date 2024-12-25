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
	_bridge.pCallFunc(4, 0);

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
	_bridge.pCallFunc(0, 0);
	_coreData.clearScriptEnv();
}

bool ScriptHelper::boolCall(int ref) const
{
	if (ref < 0) {
		return true;
	}

	lua_State* L = _bridge.getLuaState();
	ScriptBridge::LuaStackCheck check(L);
	ScriptBridge::FuncInfo fi = _bridge.getFuncInfo(ref);

	std::vector<Variant> args;
	args.resize(fi.nParams);
	int nResults = _bridge.callFunc(ref, args);
	REQUIRE(nResults >= 0);

	bool rc = false;
	if (nResults > 0) {
		rc = lua_toboolean(L, -1);
	}
	_bridge.pop(nResults);
	return rc;
}


} // namespace lurp

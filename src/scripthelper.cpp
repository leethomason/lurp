#include "scripthelper.h"
#include "lua.hpp"
#include "scriptbridge.h"
#include "util.h"
#include "coredata.h"

#include <fmt/core.h>
#include <fmt/ostream.h>

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

	int exists = lua_getglobal(L, "script");
	CHECK(exists == LUA_TNIL); 
	lua_pop(L, 1);

	int t = lua_getglobal(L, "SetupScriptEnv");
	CHECK(t == LUA_TFUNCTION);

	lua_pushstring(L, _SCRIPTENV);
	lua_pushstring(L, _scriptEnv.player.c_str());
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
	pcall(-1, 5, 0);
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
		fmt::print("WARNING: Lua error {} '{}' from '{}' at line {}\n", err, errStr, fi.srcName, fi.srcLine);
		std::string e = lua_tostring(L, -1);
		fmt::print("  {}\n", e);
		assert(false);
	}
	bool r = false;
	Variant vr = Variant::fromLua(L, -1);
	if (nResult) {
		r = lua_toboolean(L, -1) != 0;
	}
	lua_pop(L, nResult);

	if (Globals::trace && !err && funcRef >= 0) {
		fmt::print("[TRACE] Call to {} at line {} returned {}. ",
			fi.srcName, fi.srcLine, vr.toLuaString());
		if (nResult == 0) fmt::print("(nil expected) {}\n",
			vr.type == LUA_TNIL ? "" : "[NOTE]");
		else if (nResult == 1) fmt::print("{}\n",
			vr.type == LUA_TNIL ? "[NOTE] non-nil expected" : "");
		else fmt::print("{} results not expected.\n", nResult);
	}

	return r;
}

// This is the read-only fallback to the lua script.
std::string ScriptHelper::pushPath(const std::string& path) const
{
	lua_State* L = _bridge.getLuaState();
	ScriptBridge::LuaStackCheck check(L, 1);

	std::vector<std::string_view> parts = splitSV(path, '.');
	assert(parts.size() > 0);

	for (int i = 0; i<int(parts.size()) - 1; i++) {
		const auto& part = parts[i];
		std::string p(part);
		if (i == 0) {
			// i == 0 is an Entity
			// But is it referenced by entityID or a special value?
			// Check the EntityID first, then drop back to the 'specials'
			_bridge.pushGlobal("Entities");
			lua_pushstring(L, p.c_str());
			lua_gettable(L, -2);
			lua_remove(L, -2);

			if (lua_type(L, -1) == LUA_TTABLE) {
				// we got a lua table from the Entities table - good to go.
			}
			else {
				// See if it refers to a special, named entity? (script, player, npc, etc.)
				lua_pop(L, 1);
				_bridge.pushGlobal(p);
			}
			if (Globals::trace) {
				bool isCoreTable = _bridge.getBoolField("_isCoreTable", { false });
				if (!isCoreTable)
					fmt::print("[TRACE][ERROR] pushPath() trying to read {} and it is not a core table\n", p);
			}
		}
		else {
			_bridge.pushTable(p);
		}
	}

	// we only need the final table. everything else can come off the stack
	// ex: npc.subDesc.accessory
	//		npc: table
	//		subDesc: table
	//		accessory: key (string)
	// has 2 tables and a key. (3 parts). One table can be removed.
	for (int i = 0; i<int(parts.size()) - 2; i++) {
		lua_remove(L, -2);
	}
	assert(lua_type(L, -1) == LUA_TTABLE);
	return parts.empty() ? std::string("") : std::string(parts.back());
}

void ScriptHelper::corePath(const std::string& in, EntityID& entityID, std::string& path) const
{
	entityID.clear();
	path.clear();

	size_t pos = in.find('.');
	if (pos == std::string::npos) {
		assert(false); // not sure how this would happen
		return;
	}

	std::string first = in.substr(0, pos);
	if (first == "script") {
		first = std::string(_SCRIPTENV);
	}
	else if (first == "player") {
		if (_scriptEnv.player.empty()) {
			fmt::print("WARNING: Attempt to access player variable '{}' with no player\n", in);
		}
		first = _scriptEnv.player;
	}
	else if (first == "npc") {
		if (_scriptEnv.npc.empty()) {
			fmt::print("WARNING: Attempt to access npc variable '{}' with no npc\n", in);
		}
		first = _scriptEnv.npc;
	}
	else if (first == "zone") {
		if (_scriptEnv.zone.empty()) {
			fmt::print("WARNING: Attempt to access zone variable '{}' with no zone\n", in);
		}
		first = _scriptEnv.zone;
	}
	else if (first == "room") {
		if (_scriptEnv.room.empty()) {
			fmt::print("WARNING: Attempt to access room variable '{}' with no room\n", in);
		}
		first = _scriptEnv.room;
	}
	entityID = first;
	path = in.substr(pos + 1);
}

Variant ScriptHelper::get(const std::string& path) const
{
	EntityID entity;
	std::string var;
	corePath(path, entity, var);

	std::pair<bool, Variant> p = _coreData.coreGet(entity, var);
	if (p.first) {
		if (Globals::trace)
			fmt::print("[TRACE] CoreData get {}.{} -> {}\n", entity, var, p.second.toLuaString());
		return p.second;
	}
	lua_State* L = _bridge.getLuaState();
	ScriptBridge::LuaStackCheck check(L);

	std::string key = pushPath(path);
	Variant v = ScriptBridge::getField(L, key, -1, true);
	lua_pop(L, 1);
	if (Globals::trace)
		fmt::print("[TRACE] Lua get {}.{} -> {}\n", entity, var, v.toLuaString());
	return v;
}

void ScriptHelper::set(const std::string& path, const Variant& v) const
{
	EntityID entity;
	std::string var;
	corePath(path, entity, var);

	_coreData.coreSet(entity, var, v, false);
	if (Globals::trace)
		fmt::print("[TRACE] CoreData set {}.{} = {}\n", entity, var, v.toLuaString());
}

void ScriptHelper::save(std::ostream& stream) const
{
	fmt::print(stream, "ScriptHelper = {{\n");
	fmt::print(stream, "  script = '{}',\n", _scriptEnv.script);
	fmt::print(stream, "  zone = '{}',\n", _scriptEnv.zone);
	fmt::print(stream, "  room = '{}',\n", _scriptEnv.room);
	fmt::print(stream, "  player = '{}',\n", _scriptEnv.player);
	fmt::print(stream, "  npc = '{}',\n", _scriptEnv.npc);
	fmt::print(stream, "}}\n");
}

} // namespace lurp

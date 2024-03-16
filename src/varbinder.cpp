#include "varbinder.h"
#include "scriptbridge.h"
#include "coredata.h"
#include "scriptasset.h"

namespace lurp {

VarBinder::VarBinder(const ScriptAssets& assets, ScriptBridge& bridge, CoreData& coreData, const ScriptEnv& env)
	: _assets(assets), _bridge(bridge), _coreData(coreData), _env(env)
{
}

VarBinder::~VarBinder()
{
}

// This is the read-only fallback to the lua script.
std::string VarBinder::pushPath(const std::string& path) const
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
            bool isCoreTable = _bridge.getBoolField("_isCoreTable", { false });
            if (!isCoreTable) {
                PLOG(plog::debug) << "pushPath() trying to read " << p << " and it is not a CoreTable";
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

void VarBinder::corePath(const std::string& in, EntityID& entityID, std::string& path) const
{
	entityID.clear();
	path.clear();

	size_t pos = in.find('.');
	if (pos == std::string::npos) {
		assert(false); // not sure how this would happen
		return;
	}

	std::string first = in.substr(0, pos);
	// VarBinder can't do the substitutions:
	assert(first != "script");
	//assert(first != "player");	// player often called "player". oops.
	assert(first != "npc");
	assert(first != "zone");
	assert(first != "room");

	entityID = first;
	path = in.substr(pos + 1);
}

Variant VarBinder::get(const std::string& in) const
{

	std::string path = evalPath(_env, in);
	EntityID entity;
	std::string var;
	corePath(path, entity, var);

	// 1. const var on the entity
	if (_assets.isAsset(entity)) {
		const Entity* e = _assets.get(entity);
		std::pair<bool, Variant> p = e->getVar(var);
		if (p.first) {
			PLOG(plog::debug) << fmt::format("Entity get{}.{} -> {}", entity, var, p.second.toLuaString());
			return p.second;
		}
	}

	// 2. core data
	std::pair<bool, Variant> p = _coreData.coreGet(entity, var);
	if (p.first) {
		PLOG(plog::debug) << fmt::format("CoreData get{}.{} -> {}", entity, var, p.second.toLuaString());
		return p.second;
	}

	// 3. lua script
	lua_State* L = _bridge.getLuaState();
	ScriptBridge::LuaStackCheck check(L);

	std::string key = pushPath(in);
	Variant v = ScriptBridge::getField(L, key, -1, true);
	lua_pop(L, 1);
	PLOG(plog::debug) << fmt::format("Lua get {}.{} -> {}", entity, var, v.toLuaString());
	return v;
}

void VarBinder::set(const std::string& in, const Variant& v) const
{
	std::string path = evalPath(_env, in);

	EntityID entity;
	std::string var;
	corePath(path, entity, var);

	// 1. const var on the entity
	if (_assets.isAsset(entity)) {
		const Entity* e = _assets.get(entity);
		std::pair<bool, Variant> p = e->getVar(var);
		// Can't set immutable data.
		if (p.first)
			return;
	}

	// 2. core data
	_coreData.coreSet(entity, var, v, false);
	PLOG(plog::debug) << fmt::format("CoreData set {}.{} = {}", entity, var, v.toLuaString());
}

std::string VarBinder::evalPath(const ScriptEnv& env, const std::string& in) const
{
	size_t pos = in.find('.');
	if (pos == std::string::npos) {
		assert(false); // not sure how this would happen
		return in;
	}

	std::string first = in.substr(0, pos);
	if (first == "script") {
		// Note we don't use the actual name of the script, because scripts
		// can call other scripts and what a mess. Use the special magic name.
		first = std::string(_SCRIPTENV);
	}
	else if (first == "player") {
		if (env.player.empty()) {
			PLOG(plog::warning) << "Attempt to access player variable '" << in << "' with no " << first;
			assert(false);
		}
		first = env.player;
	}
	else if (first == "npc") {
		if (env.npc.empty()) {
			PLOG(plog::warning) << "Attempt to access player variable '" << in << "' with no " << first;
			assert(false);
		}
		first = env.npc;
	}
	else if (first == "zone") {
		if (env.zone.empty()) {
			PLOG(plog::warning) << "Attempt to access player variable '" << in << "' with no " << first;
			assert(false);
		}
		first = env.zone;
	}
	else if (first == "room") {
		if (env.room.empty()) {
			PLOG(plog::warning) << "Attempt to access player variable '" << in << "' with no " << first;
			assert(false);
		}
		first = env.room;
	}
	return first + "." + in.substr(pos + 1);
}

} // namespace lurp

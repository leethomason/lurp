#include "coredata.h"
#include "scriptasset.h"
#include "scriptbridge.h"
#include "scripthelper.h"

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <algorithm>

#define TRACK 0

namespace lurp {

CoreData::CoreData(ScriptBridge* bridge)
	: _bridge(bridge)
{
	if (_bridge)
		_bridge->setICore(this);
}

CoreData::~CoreData()
{
	if (_bridge)
		_bridge->setICore(nullptr);
}

void CoreData::clearScriptEnv()
{
	auto start = std::partition_point(_coreData.begin(), _coreData.end(),
		[](const auto& d) { return d.first.entity < _SCRIPTENV; });
	auto end = std::partition_point(start, _coreData.end(),
		[](const auto& d) { return d.first.entity == _SCRIPTENV; });
	_coreData.erase(start, end);
}

void CoreData::dump() const
{
	fmt::print("Core Data:\n");
	for (const auto& d : _coreData) {
		fmt::print("  {}.{} = ", d.first.entity, d.first.path);
		d.second.dump();
		fmt::print("\n");
	}
}

void CoreData::dump(const std::string& scope) const
{
	fmt::print("Core Data:\n");
	for (const auto& d : _coreData) {
		if (scope != d.first.entity)
			continue;
		fmt::print("  {}.{} = ", d.first.entity, d.first.path);
		d.second.dump();
		fmt::print("\n");
	}
}

void CoreData::coreSet(const std::string& entity, const std::string& key, Variant val, bool mutableUser)
{
	assert(!key.empty());

	if (!mutableUser) {
		// If the value is unchanged, don't bother setting it
		const auto it = _coreData.find({ entity, key });
		if (it != _coreData.end()) {
			if (it->first.mutableUser) {
				// the data is now NOT mutable, so we can see if it updates.
				if (it->second != val) {
					_coreData.erase(it);
				}
			}
		}
	}

	Flag flag{ entity, key, mutableUser };
	_coreData[flag] = val;

#if 0
	fmt::print("CoreData set {}:{} = ", entity, path);
	val.dump();
	fmt::print("\n");
#endif
}

std::pair<bool, Variant> CoreData::coreGet(const std::string& entity, const std::string& key) const
{
	const auto it = _coreData.find({ entity, key });
	if (it == _coreData.end()) {
		return { false, Variant() };
	}
#if 0
	fmt::print("CoreData get {}:{} = ", entity, key);
	it->second.dump();
	fmt::print("\n");
#endif
	return { true, it->second };
}

bool CoreData::coreBool(const std::string& scope, const std::string& flag, bool defaultValue) const
{
	const auto it = _coreData.find({ scope, flag });
	if (it == _coreData.end()) {
		return defaultValue;
	}
	return it->second.asBool();
}


void CoreData::save(std::ostream& stream) const
{
	/*
	CoreData = {
		{ "entity", "path", value },
	}
	*/
	fmt::print(stream, "CoreData = {{\n");

	for (const auto it : _coreData) {
		if (it.first.mutableUser)
			continue;
		fmt::print(stream, "  {{ '{}', '{}', {} }},\n", it.first.entity, it.first.path, it.second.toLuaString());
	}
	fmt::print(stream, "}}\n");
}

void CoreData::load(ScriptBridge& loader)
{
	lua_State* L = loader.getLuaState();
	ScriptBridge::LuaStackCheck check(L);

	loader.pushGlobal("CoreData");
	assert(lua_type(L, -1) == LUA_TTABLE);

	for (TableIt entry(L, -1); !entry.done(); entry.next()) {
		if (entry.vType() == LUA_TTABLE) {
			lua_geti(L, -1, 1);
			EntityID entity = lua_tostring(L, -1);
			lua_pop(L, 1);

			lua_geti(L, -1, 2);
			std::string path = lua_tostring(L, -1);
			lua_pop(L, 1);

			lua_geti(L, -1, 3);
			Variant v = Variant::fromLua(L, -1);
			lua_pop(L, 1);

			_coreData[{entity, path}] = v;
		}
	}
	lua_pop(L, 1);
}

} // namespace lurp
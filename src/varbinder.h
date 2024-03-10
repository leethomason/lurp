#pragma once

#include "scripttypes.h"

#include <vector>
#include <string>

namespace lurp
{
class ScriptBridge;
class CoreData;
struct ScriptEnv;

class VarBinder
{
public:
	VarBinder(
		ScriptBridge& bridge,		// connects to Lua
		CoreData& coreData,			// connects to the core variables
		const ScriptEnv& env);		// Allows substitution: zone.name, player.fighting, room.desc, etc.
									// Can be the default object, but then no substitution is possible.
	~VarBinder();

	Variant get(const std::string& path) const;
	Variant get(const std::string& path, const std::string& var) const {
		return get(path + "." + var);
	}

	void set(const std::string& path, const Variant& value) const;
	void set(const std::string& path, const std::string& var, const Variant& value) const {
		return set(path + "." + var, value);
	}

private:
	std::string pushPath(const std::string& path) const;
	void corePath(const std::string& in, EntityID& entityID, std::string& path) const;
	std::string evalPath(const ScriptEnv& env, const std::string& in) const;

	ScriptBridge& _bridge;
	CoreData& _coreData;
	const ScriptEnv& _env;
};

} // namespace lurp
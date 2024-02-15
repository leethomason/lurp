#pragma once

#include "scripttypes.h"
#include "util.h"
#include "iscript.h"

namespace lurp {

class ScriptBridge;
class CoreData;

class ScriptHelper
{
public:
	ScriptHelper(ScriptBridge& bridge,
		CoreData& coreData,
		const ScriptEnv& env);
	~ScriptHelper();

	// ex: npc.subDesc.accessory
	Variant get(const std::string& path) const;

	// ex: npc.subDesc.accessory = "hat"
	void set(const std::string& path, const Variant& value) const;

	// eval(script, player, npc) -> bool
	// code(script, player, npc) -> nil
	bool call(int funcRef, int nResult) const;

	void save(std::ostream& stream) const;

	// fixme: hacky - for the driver to get access
	ScriptBridge& bridge() const { return _bridge; }

	// When used by the ScriptDriver
	void pushScriptContext();
	void popScriptContext();
	int contextDepth() const { return _scriptContextCount; }

private:
	bool pcall(int funcRef, int nArgs, int nResult) const;
	void setupScriptEnv();
	void tearDownScriptEnv();

	ScriptBridge& _bridge;
	CoreData& _coreData;

	ScriptEnv _scriptEnv;
	int _scriptContextCount = 0;

	std::string pushPath(const std::string& path) const;
	void corePath(const std::string& in, EntityID& entityID, std::string& path) const;
};

} // namespace lurp

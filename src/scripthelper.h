#pragma once

#include "scripttypes.h"
#include "util.h"
#include "iscript.h"
#include "varbinder.h"

namespace lurp {

class ScriptBridge;
class CoreData;

class ScriptHelper
{
public:
	ScriptHelper(
		ScriptBridge& bridge,
		CoreData& coreData,
		const ScriptEnv& env);
	~ScriptHelper();

	// eval(script, player, npc) -> bool
	// code(script, player, npc) -> nil
	bool call(int funcRef, int nResult) const;

	bool callGlobal(const std::string& funcName, const std::vector<std::string>& args, int nResult) const;

	const ScriptEnv& env() const { return _scriptEnv; }

private:
	bool pcall(int funcRef, int nArgs, int nResult) const;
	void setupScriptEnv();
	void tearDownScriptEnv();

	ScriptBridge& _bridge;
	CoreData& _coreData;
	const ScriptEnv& _scriptEnv;
};

} // namespace lurp

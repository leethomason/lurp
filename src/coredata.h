#pragma once

#include "defs.h"
#include "iscript.h"

#include <map>
#include <string>
#include <iostream>

namespace lurp{

struct ScriptAssets;
class ScriptBridge;
class ScriptHelper;

class CoreData : public ICoreHandler
{
	// FIXME: sholudn't need a script bridge
	// FIXME: shouldn't call setICore
public:
	// The ScriptBridge* will set up callbacks to supply the coredata to the scripting system
	CoreData();
	~CoreData();

	void clearScriptEnv();
	void dump() const;
	void dump(const std::string& scope) const;

	virtual void coreSet(const std::string& scope, const std::string& flag, Variant val, bool mutableUser);
	virtual std::pair<bool, Variant> coreGet(const std::string& scope, const std::string& flag) const;
	bool coreBool(const std::string& scope, const std::string& flag, bool defaultValue) const;

	// If the 'bridge' is not null, it will be used to check if coreData is unchanged.
	void save(std::ostream& stream) const;
	void load(ScriptBridge& loader);

private:
	struct Flag {
		EntityID entity;
		std::string path;
		bool mutableUser = false;

		bool operator< (const Flag& rhs) const {
			return entity < rhs.entity || (entity == rhs.entity && path < rhs.path);
		}
		bool operator== (const Flag& rhs) const {
			return entity == rhs.entity && path == rhs.path;
		}
		bool operator!= (const Flag& rhs) const {
			return !(*this == rhs);
		}
	};

	std::map<Flag, Variant> _coreData;
	//ScriptBridge* _bridge = nullptr;
};

} // namespace lurp
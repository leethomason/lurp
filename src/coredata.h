#pragma once

#include "defs.h"
#include "iscript.h"

#include <map>
#include <string>
#include <iostream>
#include <stdint.h>

namespace lurp{

struct ScriptAssets;
class ScriptBridge;
class ScriptHelper;

class CoreData : public ICoreHandler
{
public:
	CoreData();
	~CoreData();

	void clearScriptEnv();
	void dump() const;
	void dump(const std::string& scope) const;

	virtual void coreSet(const std::string& scope, const std::string& flag, Variant val, bool mutableUser);
	virtual std::pair<bool, Variant> coreGet(const std::string& scope, const std::string& flag) const;
	bool coreBool(const std::string& scope, const std::string& flag, bool defaultValue) const;

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
};

} // namespace lurp
#pragma once

#include "defs.h"

namespace lurp {

// Provided by the Map
class IMapHandler {
public:
	virtual uint32_t getRandom() = 0;
	virtual bool isLocked(const EntityID& id) const = 0;
	virtual void setLocked(const EntityID&, bool locked) = 0;

	// add/remove is delta
	virtual void deltaItem(const EntityID& id, const EntityID& item, int n) = 0;
	virtual int numItems(const EntityID& id, const EntityID& item) const = 0;

	virtual void movePlayer(const EntityID& dst, bool teleport) = 0;
	virtual void endGame(const std::string& reason) = 0;
};

class IAssetHandler {
public:
	// returns true an asset & path exists
	virtual std::pair<bool, Variant> assetGet(const std::string& entity, const std::string& path) const = 0;
};

class ICoreHandler {
public:
	virtual void coreSet(const std::string& entity, const std::string& path, Variant val, bool initial) = 0;
	virtual std::pair<bool, Variant> coreGet(const std::string& entity, const std::string& path) const = 0;
};

// Provided by the ScriptDriver
class ITextHandler {
public:
	virtual bool allTextRead(const EntityID& id) const = 0;
};

} // namespace lurp

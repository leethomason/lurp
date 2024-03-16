#pragma once

#include "scripttypes.h"
#include "zone.h"
#include "util.h"
#include "coredata.h"
#include "mapdata.h"
#include "items.h"
#include "scriptasset.h"

#include <vector>
#include <map>
#include <optional>
#include <set>

namespace lurp {

struct ScriptAssets;
class ScriptBridge;
class ScriptDriver;

using ContainerVec = std::vector<const Container*>;
using InteractionVec = std::vector<const Interaction*>;

// World navigation, containers, and high level map/zone/room structure.
// Also holds the coreData.
class ZoneDriver : public IMapHandler {
public:
	enum class MoveResult {
		kNoConnection,
		kLocked,
		kSuccess,
	};

	// FIXME: why 2 constructors?
	// FIXME: hides a MapData getting constructed with a default seed

	// Create a map with navigation, containers, and interactions.
	// This defines a small game. For interactions w/ script,
	// and calls into Scripts, a ScriptBridge is required.
	// Sometimes useful to test the base map functionality without the bridge.
	ZoneDriver(ScriptAssets& assets, ScriptBridge& bridge, const EntityID& player);
	ZoneDriver(ScriptAssets& assets, ScriptBridge& bridge, const EntityID& zone, const EntityID& player);
	~ZoneDriver();

	// ------ Queries ------
	const Room& currentRoom() const;
	const Zone& currentZone() const;
	const Actor& getPlayer();

	// ------ Driver ------
	enum class Mode { kText, kChoices, kNavigation, kBattle };
	Mode mode() const;

	// Mode: Text
	TextLine text() const;
	void advance();

	// Mode: Choices
	const Choices& choices() const;
	void choose(int n);

	// Mode: Navigation
	std::vector<DirEdge> dirEdges(EntityID room = "") const;
	ContainerVec getContainers(EntityID room = "");
	InteractionVec getInteractions(EntityID room = "");

	const Container* getContainer(const EntityID& id);
	void startInteraction(const Interaction* interaction);

	void setZone(const EntityID& zone, EntityID room);

	template<typename ENTITY>
	bool locked(const ENTITY& e) {
		return mapData.coreData.coreBool(e.entityID, "locked", e.locked);
	}
	bool unlock(const Edge& e);
	bool unlock(const Container& c);

	enum class TransferResult {
		kSuccess,
		kLocked
	};

	template<typename E>
	const Inventory& getInventory(const E& e) const {
		return _assets.getInventory(e);
	}

	template<typename S, typename T>
	TransferResult transferAll(const S& srcEntity, const T& dstEntity);

	template<typename S, typename T>
	TransferResult transfer(const Item& item, const S& srcEntity, const T& dstEntity, int n);

	// Mode: Battle
	const Battle& battle() const;
	VarBinder battleVarBinder() const;
	void battleDone();	// advance past the battle; if the battle is lost, you need to call endGame("You lost", -1)

	// ------ Internal / Here Be Dragons ------
	// empty 'room' will return edges for the current room
	const std::vector<EntityID>& entities(EntityID room = "") const;

	const Interaction* getRequiredInteraction();
	ScriptEnv getScriptEnv(const Interaction* interaction);
	void markRequiredInteractionComplete(const Interaction*);
	void markRequiredInteractionComplete(const EntityID& scriptID);

	// checks for locks and connections
	MoveResult move(const EntityID& roomID);
	MoveResult move(const Edge& edge);

	// go anywhere; don't check for locks
	void teleport(const EntityID& roomID);
	// IMapHandler
	virtual uint32_t getRandom() { return mapData.random.rand(); }
	virtual bool isLocked(const EntityID& id) const;
	virtual void setLocked(const EntityID&, bool locked);
	virtual void deltaItem(const EntityID& id, const EntityID& item, int n);
	virtual int numItems(const EntityID& id, const EntityID& item) const;
	virtual void movePlayer(const EntityID& dst, bool teleport);
	virtual void endGame(const std::string& msg, int bias);

	bool isGameOver() const { return !_endGameMsg.empty(); }
	std::string endGameMsg() const { return _endGameMsg; }
	int endGameBias() const { return _endGameBias; }

	// Note that for save AND load, the ScriptAssets must already be loaded.
	// The save and load apply a delta on the ScriptAssets.
	void save(std::ostream& stream) const;
	EntityID load(ScriptBridge& loader);

	static void saveTextRead(std::ostream& stream, const std::unordered_set<uint64_t>& text);
	static void loadTextRead(ScriptBridge& loader, std::unordered_set<uint64_t>& text);

	const ScriptAssets& assets() const { return _assets; }

private:
	std::vector<Edge> edges(EntityID room = "") const;
	bool filterInteraction(const Interaction&);	// true if interaction should be included
	void checkScriptDriver();

	const EntityID& zoneID() const;
	const EntityID& roomID() const;

	ScriptAssets& _assets;
	ScriptBridge& _bridge;
	EntityID _playerID;
public:
	MapData mapData;
private:
	ScriptRef _zone;
	ScriptRef _room;
	ScriptDriver* _scriptDriver = nullptr;
	std::string _endGameMsg;
	int _endGameBias = 0;
};

template<typename S, typename T>
ZoneDriver::TransferResult ZoneDriver::transferAll(const S& srcEntity, const T& dstEntity) {
	if (isLocked(srcEntity.entityID))
		unlock(srcEntity);

	if (isLocked(srcEntity.entityID) || isLocked(dstEntity.entityID))
		return TransferResult::kLocked;

	Inventory& src = _assets.getInventory(srcEntity);
	while (!src.emtpy()) {
		this->transfer(*src.items()[0].pItem, srcEntity, dstEntity, INT_MAX);
	}
	return TransferResult::kSuccess;
}

template<typename S, typename T>
ZoneDriver::TransferResult ZoneDriver::transfer(const Item& item, const S& srcEntity, const T& dstEntity, int n) {
	if (isLocked(srcEntity.entityID) || isLocked(dstEntity.entityID))
		return TransferResult::kLocked;

	Inventory& src = _assets.getInventory(srcEntity);
	Inventory& dst = _assets.getInventory(dstEntity);
	int delta = src.numItems(item);
	lurp::Inventory::transfer(item, src, dst, n);
	int count = dst.numItems(item);

	const EntityID& playerID = getPlayer().entityID;
	if (srcEntity.entityID == playerID || dstEntity.entityID == playerID) {
		mapData.newsQueue.push(NewsItem::itemDelta(item, delta, count));
	}
	return TransferResult::kSuccess;
}


} // namespace lurp
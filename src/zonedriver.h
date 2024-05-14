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
	ZoneDriver(ScriptAssets& assets, ScriptBridge& bridge);
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

	const Inventory& getInventory(const Entity& e) const {
		return _assets.getInventory(e);
	}

	enum class TransferResult {
		kSuccess,
		kLocked
	};
	TransferResult transferAll(const EntityID& srcEntity, const EntityID& dstEntity);
	TransferResult transfer(const Item& item, const EntityID& srcEntity, const EntityID&, int n);

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

	bool isLocked(const Entity& e) const { return isLocked(e.entityID);  }
	bool tryUnlock(const Entity& e);
	bool tryUnlock(const EntityID& id) {
		CHECK(_assets.isAsset(id));
		return tryUnlock(*_assets.get(id));
	}

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
public:
	MapData mapData;
private:
	ScriptRef _zone;
	ScriptRef _room;
	ScriptDriver* _scriptDriver = nullptr;
	std::string _endGameMsg;
	int _endGameBias = 0;
};

} // namespace lurp
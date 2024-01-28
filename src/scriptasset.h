#pragma once

#include <map>

#include "zone.h"
#include "scripttypes.h"
#include "iscript.h"

class ScriptBridge;

struct ConstScriptAssets
{
	std::vector<Script> scripts;
	std::vector<Text> texts;
	std::vector<Choices> choices;
	std::vector<Item> items;
	std::vector<Interaction> interactions;
	std::vector<Room> rooms;
	std::vector<Zone> zones;
	std::vector<Actor> actors;
	std::vector<Battle> battles;
	std::vector<Container> containers;
	std::vector<Edge> edges;
	std::vector<CallScript> callScripts;
};

struct ScriptAssets : public IAssetHandler
{
	ScriptAssets(const ConstScriptAssets& csa);

	const std::vector<Script>& scripts;
	const std::vector<Text>& texts;
	const std::vector<Choices>& choices;
	const std::vector<Item>& items;
	const std::vector<Interaction>& interactions;
	const std::vector<Room>& rooms;
	const std::vector<Zone>& zones;
	const std::vector<Actor>& actors;
	const std::vector<Battle>& battles;
	const std::vector<Container>& containers;
	const std::vector<Edge>& edges;
	const std::vector<CallScript>& callScripts;

	std::map<EntityID, Inventory> inventories;

	ScriptRef get(const EntityID& entityID) const {
		return entityIDToIndex.at(entityID);
	}
	bool isAsset(const EntityID& entityID) const {
		return entityIDToIndex.find(entityID) != entityIDToIndex.end();
	}

	const Script& getScript(const EntityID& entityID) const;
	const Text& getText(const EntityID& entityID) const;
	const Choices& getChoices(const EntityID& entityID) const;
	const Item& getItem(const EntityID& entityID) const;
	const Interaction& getInteraction(const EntityID& entityID) const;
	const Room& getRoom(const EntityID& entityID) const;
	const Zone& getZone(const EntityID& entityID) const;
	const Battle& getBattle(const EntityID& entityID) const;
	const CallScript& getCallScript(const EntityID& entityID) const;
	const Actor& getActor(const EntityID& entityID) const;
	const Container& getContainer(const EntityID& entityID) const;
	const Edge& getEdge(const EntityID& entityID) const;

	template <typename T>
	Inventory& getInventory(const T& entity) {
		return inventories[entity.entityID];
	}

	const ConstScriptAssets& getConst() const { return _csa; }

	void save(std::ostream& stream);
	void load(ScriptBridge& loader);

	virtual std::pair<bool, Variant> assetGet(const std::string& entity, const std::string& path) const;

private:
	std::map<EntityID, ScriptRef> entityIDToIndex;
	const ConstScriptAssets& _csa;

	void scan();
};
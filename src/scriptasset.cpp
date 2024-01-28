#include "scriptasset.h"
#include "scriptbridge.h"

#include <fmt/core.h>
#include <fmt/ostream.h>

namespace lurp {

ScriptAssets::ScriptAssets(const ConstScriptAssets& csa) :
	_csa(csa),
	scripts(csa.scripts),
	texts(csa.texts),
	choices(csa.choices),
	items(csa.items),
	interactions(csa.interactions),
	rooms(csa.rooms),
	zones(csa.zones),
	containers(csa.containers),
	edges(csa.edges),
	actors(csa.actors),
	battles(csa.battles),
	callScripts(csa.callScripts)
{
	scan();
}

std::pair<bool, Variant> ScriptAssets::assetGet(const std::string& entity, const std::string& path) const
{
	if (!isAsset(entity)) return { false, Variant() };
	if (path == "entityID") return { true, Variant(entity) };
	ScriptRef ref = get(entity);
	switch (ref.type) {
	case ScriptType::kContainer:
		return containers[ref.index].get(path);
	case ScriptType::kEdge:
		return edges[ref.index].get(path);
	case ScriptType::kActor:
		return actors[ref.index].get(path);
	case ScriptType::kBattle:
		return battles[ref.index].get(path);
	default:
		return { false, Variant() };
	}
}

void ScriptAssets::save(std::ostream& stream)
{
	/*
		Inventories = {
			{ entityID = "Grom", items = { "Sword" }},
			{ entityID = "THIEF_01", items = { "Dagger", {"GOLD", 10} }},
		},
	*/
	fmt::print(stream, "Inventories = {{\n");

	for (auto& [entityID, inventory] : inventories) {
		ScriptRef ref = get(entityID);
		if (ref.type == ScriptType::kContainer) {
			const Container& container = containers[ref.index];
			if (container.inventory == inventory) continue;
		}
		if (ref.type == ScriptType::kActor) {
			const Actor& actor = actors[ref.index];
			if (actor.inventory == inventory) continue;
		}

		fmt::print(stream, "    {{ entityID = '{}', ", entityID);
		inventory.save(stream);
		fmt::print(stream, "\t}},\n");
	}
	fmt::print(stream, "}}\n");
}

void ScriptAssets::load(ScriptBridge& loader)
{
	//inventories.clear(); // fixme clear existing???
	lua_State* L = loader.getLuaState();
	ScriptBridge::LuaStackCheck check(L);

	lua_getglobal(L, "Inventories");
	assert(lua_type(L, -1) == LUA_TTABLE);

	for (TableIt it(L, -1); !it.done(); it.next()) {
		if (it.kType() != LUA_TNUMBER) continue;
		if (it.vType() != LUA_TTABLE) continue;

		EntityID id = loader.getStrField("entityID", {});
		Inventory inv = loader.readInventory();
		inventories[id] = inv;
	}
	lua_pop(L, 1);
}

void ScriptAssets::scan()
{
	for (int i = 0; i < scripts.size(); ++i) entityIDToIndex[scripts[i].entityID] = { ScriptType::kScript, i };
	for (int i = 0; i < texts.size(); ++i) entityIDToIndex[texts[i].entityID] = { ScriptType::kText, i };
	for (int i = 0; i < choices.size(); ++i) entityIDToIndex[choices[i].entityID] = { ScriptType::kChoices, i };
	for (int i = 0; i < items.size(); ++i) entityIDToIndex[items[i].entityID] = { ScriptType::kItem, i };
	for (int i = 0; i < interactions.size(); ++i) entityIDToIndex[interactions[i].entityID] = { ScriptType::kInteraction, i };
	for (int i = 0; i < rooms.size(); ++i) entityIDToIndex[rooms[i].entityID] = { ScriptType::kRoom, i };
	for (int i = 0; i < zones.size(); ++i) entityIDToIndex[zones[i].entityID] = { ScriptType::kZone, i };
	for (int i = 0; i < containers.size(); ++i) entityIDToIndex[containers[i].entityID] = { ScriptType::kContainer, i };
	for (int i = 0; i < edges.size(); ++i) entityIDToIndex[edges[i].entityID] = { ScriptType::kEdge, i };
	for (int i = 0; i < actors.size(); ++i) entityIDToIndex[actors[i].entityID] = { ScriptType::kActor, i };
	for (int i = 0; i < battles.size(); ++i) entityIDToIndex[battles[i].entityID] = { ScriptType::kBattle, i };
	for (int i = 0; i < callScripts.size(); ++i) entityIDToIndex[callScripts[i].entityID] = { ScriptType::kCallScript, i };

	for (size_t i = 0; i < containers.size(); ++i) {
		inventories[containers[i].entityID] = containers[i].inventory;
	}
	for (size_t i = 0; i < actors.size(); ++i) {
		inventories[actors[i].entityID] = actors[i].inventory;
	}
}

#define TYPE_BODY(vecName, itemEnum) \
	ScriptRef ref = get(entityID); \
	assert(ref.type == ScriptType::itemEnum); \
	return vecName[ref.index];

const Script& ScriptAssets::getScript(const EntityID& entityID) const { TYPE_BODY(scripts, kScript); }
const Text& ScriptAssets::getText(const EntityID& entityID) const { TYPE_BODY(texts, kText); }
const Choices& ScriptAssets::getChoices(const EntityID& entityID) const { TYPE_BODY(choices, kChoices); }
const Item& ScriptAssets::getItem(const EntityID& entityID) const { TYPE_BODY(items, kItem); }
const Interaction& ScriptAssets::getInteraction(const EntityID& entityID) const { TYPE_BODY(interactions, kInteraction); }
const Room& ScriptAssets::getRoom(const EntityID& entityID) const { TYPE_BODY(rooms, kRoom); }
const Zone& ScriptAssets::getZone(const EntityID& entityID) const { TYPE_BODY(zones, kZone); }
const Battle& ScriptAssets::getBattle(const EntityID& entityID) const { TYPE_BODY(battles, kBattle); }
const CallScript& ScriptAssets::getCallScript(const EntityID& entityID) const { TYPE_BODY(callScripts, kCallScript); }
const Actor& ScriptAssets::getActor(const EntityID& entityID) const { TYPE_BODY(actors, kActor); }
const Container& ScriptAssets::getContainer(const EntityID& entityID) const { TYPE_BODY(containers, kContainer); }
const Edge& ScriptAssets::getEdge(const EntityID& entityID) const { TYPE_BODY(edges, kEdge); }

} // namespace lurp
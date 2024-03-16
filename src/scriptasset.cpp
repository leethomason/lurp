#include "scriptasset.h"
#include "scriptbridge.h"

#include <fmt/core.h>
#include <fmt/ostream.h>

namespace lurp {

ScriptAssets::ScriptAssets(const ConstScriptAssets& csa) :
	_csa(csa)
{
	scan();
}

std::pair<bool, Variant> ScriptAssets::assetGet(const std::string& entity, const std::string& path) const
{
	if (!isAsset(entity)) return { false, Variant() };
	if (path == "entityID") return { true, Variant(entity) };
	const Entity* e = get(entity);
	return e->getVar(path);
}

std::string ScriptAssets::desc(const EntityID& entityID) const
{
	const Entity* e = get(entityID);
	return e->description();
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
		ScriptRef ref = getScriptRef(entityID);
		if (ref.type == ScriptType::kContainer) {
			const Container& container = _csa.containers[ref.index];
			if (container.inventory == inventory) continue;
		}
		if (ref.type == ScriptType::kActor) {
			const Actor& actor = _csa.actors[ref.index];
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
	scan(_csa.scripts);
	scan(_csa.texts);
	scan(_csa.choices);
	scan(_csa.items);
	scan(_csa.powers);
	scan(_csa.interactions);
	scan(_csa.rooms);
	scan(_csa.zones);
	scan(_csa.actors);
	scan(_csa.combatants);
	scan(_csa.battles);
	scan(_csa.containers);
	scan(_csa.edges);
	scan(_csa.callScripts);

	for (size_t i = 0; i < _csa.containers.size(); ++i) {
		inventories[_csa.containers[i].entityID] = _csa.containers[i].inventory;
	}
	for (size_t i = 0; i < _csa.actors.size(); ++i) {
		inventories[_csa.actors[i].entityID] = _csa.actors[i].inventory;
	}
}

#define TYPE_BODY(vecName, itemEnum) \
	ScriptRef ref = getScriptRef(entityID); \
	assert(ref.type == ScriptType::itemEnum); \
	return _csa.vecName[ref.index];

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
const Combatant& ScriptAssets::getCombatant(const EntityID& entityID) const { TYPE_BODY(combatants, kCombatant); }
const Container& ScriptAssets::getContainer(const EntityID& entityID) const { TYPE_BODY(containers, kContainer); }
const Edge& ScriptAssets::getEdge(const EntityID& entityID) const { TYPE_BODY(edges, kEdge); }
const Power& ScriptAssets::getPower(const EntityID& entityID) const { TYPE_BODY(powers, kPower); }

} // namespace lurp
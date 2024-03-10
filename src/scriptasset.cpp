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
	ScriptRef ref = get(entity);
	switch (ref.type) {
	case ScriptType::kContainer:
		return _csa.containers[ref.index].get(path);
	case ScriptType::kEdge:
		return _csa.edges[ref.index].get(path);
	case ScriptType::kActor:
		return _csa.actors[ref.index].get(path);
	case ScriptType::kBattle:
		return _csa.battles[ref.index].get(path);
	default:
		assert(false);
		return { false, Variant() };
	}
}

std::string ScriptAssets::desc(const EntityID& entityID) const
{
	ScriptRef ref = get(entityID);
	switch (ref.type) {

	case ScriptType::kScript: {
		const Script& script = _csa.scripts[ref.index];
		return fmt::format("Script '{}' npc='{}' code={} nEvents={}", script.entityID, script.npc, script.code, script.events.size());
	}
	case ScriptType::kText: {
		const Text& text = _csa.texts[ref.index];
		return fmt::format("Text '{}'", text.entityID);
	}
	case ScriptType::kChoices: {
		const Choices& choice = _csa.choices[ref.index];
		return fmt::format("Choice '{}' nChoice={}", choice.entityID, choice.choices.size());
	}
	case ScriptType::kItem: {
		const Item& item = _csa.items[ref.index];
		return fmt::format("Item '{}' name={}", item.entityID, item.name);
	}
	case ScriptType::kPower: {
		const Power& power = _csa.powers[ref.index];
		return fmt::format("Power '{}' name={}", power.entityID, power.name);
	}
	case ScriptType::kInteraction: {
		const Interaction& interaction = _csa.interactions[ref.index];
		return fmt::format("Interaction '{}' name={}", interaction.entityID, interaction.name);
	}
	case ScriptType::kRoom: {
		const Room& room = _csa.rooms[ref.index];
		return fmt::format("Room '{}' name={}", room.entityID, room.name);
	}
	case ScriptType::kZone: {
		const Zone& zone = _csa.zones[ref.index];
		return fmt::format("Zone '{}' name={}", zone.entityID, zone.name);
	}
	case ScriptType::kBattle: {
		const Battle& battle = _csa.battles[ref.index];
		return fmt::format("Battle '{}' name={}", battle.entityID, battle.name);
	}
	case ScriptType::kContainer: {
		const Container& container = _csa.containers[ref.index];
		return fmt::format("Container '{}' name={}", container.entityID, container.name);
	}
	case ScriptType::kEdge: {
		const Edge& edge = _csa.edges[ref.index];
		return fmt::format("Edge '{}' name={}", edge.entityID, edge.name);
	}
	case ScriptType::kActor: {
		const Actor& actor = _csa.actors[ref.index];
		return fmt::format("Actor '{}' name={}", actor.entityID, actor.name);
	}
	case ScriptType::kCombatant: {
		const Combatant& combatant = _csa.combatants[ref.index];
		return fmt::format("Combatant '{}' name={} count={}", combatant.entityID, combatant.name, combatant.count);
	}
	case ScriptType::kCallScript: {
		const CallScript& callScript = _csa.callScripts[ref.index];
		return fmt::format("CallScript '{}' scriptID='{}' npc='{}'", callScript.entityID, callScript.scriptID, callScript.npc);
	}
	default:
		assert(false);
		return "Unknown";
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
	ScriptRef ref = get(entityID); \
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
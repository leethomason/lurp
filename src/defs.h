#pragma once

#include "lua.hpp"
#include "lurpvariant.h"

#include <stdint.h>
#include <string>

namespace lurp {

using EntityID = std::string;
struct Entity;

struct ScriptEnv {
	EntityID script;
	EntityID zone;
	EntityID room;
	EntityID npc;

	bool complete() const {
		return !script.empty() && !zone.empty() && !room.empty() && !npc.empty();
	}
};

static constexpr char _SCRIPTENV[] = "_ScriptEnv";

static constexpr char NO_ENTITY[1] = {0};

enum class ScriptType {
	kNone,

	kScript,
	kText,
	kChoices,
	kItem,
	kPower,
	kInteraction,
	kRoom,
	kZone,
	kBattle,
	kContainer,
	kEdge,
	kActor,
	kCombatant,
	kCallScript,

	// Not technically scripts, but used in the same way.
	kInventory,
};

const char* scriptTypeName(ScriptType type);

struct ScriptRef {
	const Entity* entity = nullptr;
	ScriptType type = ScriptType::kNone;
	int index = 0;

	bool operator==(const ScriptRef& rhs) const {
		return type == rhs.type && index == rhs.index;
	}
	bool operator!=(const ScriptRef& rhs) const {
		return !(*this == rhs);
	}
};

struct Entity {
	Entity() = default;
	Entity(const EntityID& id) : entityID(id) {}
	virtual ~Entity() = default;

	EntityID entityID;

	virtual std::string description() const = 0;
	virtual std::pair<bool, Variant> getVar(const std::string& k) const = 0;
	virtual ScriptType getType() const = 0;
};

} // namespace lurp
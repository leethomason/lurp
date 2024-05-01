#pragma once

#include "lua.hpp"

#include <string>

namespace lurp {

using EntityID = std::string;
struct Entity;

struct ScriptEnv {
	EntityID script;
	EntityID zone;
	EntityID room;
	EntityID player;
	EntityID npc;

	bool complete() const {
		return !script.empty() && !zone.empty() && !room.empty() && !player.empty() && !npc.empty();
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

struct Variant {
	Variant() = default;
	Variant(const Variant&) = default;
	Variant& operator=(const Variant&) = default;

	Variant(const std::string& s) : type(LUA_TSTRING), str(s) {}
	Variant(const char* s) : type(LUA_TSTRING), str(s) {}
	Variant(double n) : type(LUA_TNUMBER), num(n) {}
	Variant(int n) : type(LUA_TNUMBER), num(n) {}
	Variant(bool b) : type(LUA_TBOOLEAN), boolean(b) {}

	bool operator==(const Variant& rhs) const {
		return type == rhs.type && str == rhs.str && num == rhs.num && boolean == rhs.boolean;
	}
	bool operator!=(const Variant& rhs) const {
		return !(*this == rhs);
	}

	bool isTruthy() const;
	bool asBool() const { return isTruthy(); }

	static Variant fromLua(lua_State* L, int index);
	void pushLua(lua_State* L) const;

	std::string toLuaString() const;
	void dump() const;

	int type = LUA_TNIL;
	std::string str;
	double num = 0;
	bool boolean = false;
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
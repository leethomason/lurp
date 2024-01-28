#pragma once

#include <string>
#include "lua.hpp"

namespace lurp {

using EntityID = std::string;

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

static const char* _SCRIPTENV = "_ScriptEnv";

static constexpr char NO_ENTITY[1] = {0};

enum class ScriptType {
	kNone,

	kScript,
	kText,
	kChoices,
	kItem,
	kInteraction,
	kRoom,
	kZone,
	kBattle,
	kContainer,
	kEdge,
	kActor,
	kCallScript,
};

const char* scriptTypeName(ScriptType type);

struct ScriptRef {
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

} // namespace lurp
#pragma once

#include <string>
#include <vector>
#include <fmt/core.h>
#include "defs.h"
#include "items.h"

namespace lurp {

struct ScriptAssets;
class ScriptBridge;

struct Container : Entity {
	std::string name;
	int eval = -1;
	bool locked = false;
	EntityID key;
	Inventory inventory;

	static constexpr ScriptType type{ ScriptType::kContainer };
	
	virtual std::string description() const override {
		return fmt::format("Container {} {}", entityID, name);
	};

	virtual std::pair<bool, Variant> getVar(const std::string& k) const override {
		if (k == "name") return { true, Variant(name) };
		if (k == "key") return { true, Variant(key) };
		return { false ,Variant() };
	}

	virtual ScriptType getType() const override {
		return type;
	}
};

struct Edge : Entity {
	enum class Dir {
		kNorth,
		kNortheast,
		kEast,
		kSoutheast,
		kSouth,
		kSouthwest,
		kWest,
		kNorthwest,
		kUnknown
	};

	std::string name;
	Dir dir = Dir::kUnknown;
	EntityID room1;
	EntityID room2;
	EntityID key;
	bool locked = false;

	static constexpr ScriptType type{ ScriptType::kEdge };
	
	virtual std::string description() const override {
		return fmt::format("Edge {}   {} <-> {}", name, room1, room2);
	};

	virtual std::pair<bool, Variant> getVar(const std::string& k) const override {
		if (k == "name") return { true, Variant(name) };
		if (k == "dir") return { true, Variant(dirToShortName(dir)) };
		if (k == "room1") return { true, Variant(room1) };
		if (k == "room2") return { true, Variant(room2) };
		if (k == "key") return { true, Variant(key) };
		if (k == "locked") return { true, Variant()};	// the original value is never the desired value
		return { false, Variant() };
	}

	virtual ScriptType getType() const override {
		return type;
	}

	static std::string dirToShortName(Dir d);
	static std::string dirToLongName(Dir d);
};

struct DirEdge
{
	EntityID entityID;
	std::string name;
	Edge::Dir dir = Edge::Dir::kUnknown;
	std::string dirShort;
	std::string dirLong;
	EntityID dstRoom;
	bool locked = false;
	EntityID key;
};

struct Room : Entity {
	std::string name;
	std::string desc;
	std::vector<EntityID> objects;

	static constexpr ScriptType type{ ScriptType::kRoom };

	std::string description() const override {
		return fmt::format("Room {} {}", entityID, name);
	};

	virtual std::pair<bool, Variant> getVar(const std::string& k) const override {
		if (k == "name") return { true, Variant(name) };
		if (k == "desc") return { true, Variant(desc) };
		return { false, Variant() };
	}

	virtual ScriptType getType() const override {
		return type;
	}
};

struct Zone : Entity {
	std::string name;
	std::vector<EntityID> objects;

	// By convention, the first room is the starting room.
	const Room* firstRoom(const ScriptAssets& assets) const;

	static constexpr ScriptType type{ ScriptType::kZone };

	virtual std::string description() const override {
		return fmt::format("Zone name: {}", entityID);
	}

	virtual std::pair<bool, Variant> getVar(const std::string&) const override {
		return { false, Variant() };
	};

	virtual ScriptType getType() const override {
		return type;
	}
};

struct Interaction : Entity {
	std::string name;
	EntityID next;
	EntityID npc;
	int eval = -1;
	int code = -1;
	bool required = false;

	bool active(bool done) const { return !required || !done; }

	static constexpr ScriptType type{ ScriptType::kInteraction };

	virtual std::string description() const override {
		return fmt::format("Interaction entityID: {} '{}'", entityID, name);
	}

	virtual std::pair<bool, Variant> getVar(const std::string& k) const override {
		if (k == "name") return { true, Variant(name) };
		if (k == "next") return { true, Variant(next) };
		if (k == "npc") return { true, Variant(npc) };
		if (k == "required") return { true, Variant(required) };
		return { false, Variant() };
	}

	virtual ScriptType getType() const override {
		return type;
	}


};

struct CallScript : Entity {
	EntityID scriptID;
	EntityID npc;
	int code = -1;
	int eval = -1;

	static constexpr ScriptType type{ ScriptType::kCallScript };
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("CallScript {} calls={}\n", entityID, scriptID);
	}

	virtual std::string description() const override {
		return fmt::format("CallScript entityID: {} npc={} scriptID={}", entityID, npc, scriptID);
	}

	virtual std::pair<bool, Variant> getVar(const std::string& k) const override {
		if (k == "scriptID") return { true, Variant(scriptID) };
		if (k == "npc") return { true, Variant(npc) };
		return { false, Variant() };
	}

	virtual ScriptType getType() const override {
		return type;
	}
};

} // namespace lurp
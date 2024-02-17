#pragma once

#include <string>
#include <vector>
#include <fmt/core.h>
#include "defs.h"
#include "items.h"

namespace lurp {

struct ScriptAssets;
class ScriptBridge;

struct Container {
	EntityID entityID;
	std::string name;
	int eval = -1;
	bool locked = false;
	EntityID key;
	Inventory inventory;

	std::pair<bool, Variant> get(const std::string& k) const {
		if (k == "locked") return { true, Variant(locked) };
		if (k == "name") return { true, Variant(name) };
		return { false, Variant() };
	}
	static constexpr ScriptType type{ ScriptType::kContainer };
	void dump(int d) const {
		fmt::print("{: >{}}Container {} {}\n", "", d * 2, entityID, name);
	};
};

struct Edge {
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

	EntityID entityID;
	std::string name;
	Dir dir = Dir::kUnknown;
	EntityID room1;
	EntityID room2;
	EntityID key;
	bool locked = false;

	std::pair<bool, Variant> get(const std::string& k) const {
		if (k == "locked") return { true, Variant(locked) };
		if (k == "name") return { true, Variant(name) };
		if (k == "key") return { true, Variant(key) };
		return { false, Variant() };
	}

	static constexpr ScriptType type{ ScriptType::kEdge };
	void dump(int d) const {
		fmt::print("{: >{}}Edge {}   {} <-> {}\n", "", d * 2, name, room1, room2);
	};

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

struct Room {
	EntityID entityID;
	std::string name;
	std::string desc;
	std::vector<EntityID> objects;

	static constexpr ScriptType type{ ScriptType::kRoom };
	void dump(int d) const {
		fmt::print("{: >{}}Room {} {}\n", "", d * 2, entityID, name);
	};
};

struct Zone {
	EntityID entityID;
	std::string name;
	std::vector<EntityID> objects;

	// By convention, the first room is the starting room.
	const Room* firstRoom(const ScriptAssets& assets) const;

	static constexpr ScriptType type{ ScriptType::kZone };
	void dump(int d) const {
		fmt::print("{: >{}}Zone name: {}\n", "", d * 2, entityID);
		for (const auto& r : objects) {
			fmt::print("{: >{}}  {}\n", "", d * 2, r);
		}
	}
};

struct Interaction {
	EntityID entityID;
	std::string name;
	EntityID next;
	EntityID actor;
	int eval = -1;
	int code = -1;
	bool _required = false;

	bool active(bool done) const { return !_required || !done; }

	static constexpr ScriptType type{ ScriptType::kInteraction };
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("Interaction entityID: {} '{}'\n", entityID, name);
	}
};

struct CallScript {
	EntityID entityID;
	EntityID scriptID;
	int code = -1;
	int eval = -1;

	static constexpr ScriptType type{ ScriptType::kCallScript };
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("CallScript {} calls={}\n", entityID, scriptID);
	}
};

} // namespace lurp
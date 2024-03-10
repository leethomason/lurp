#pragma once

#include <string>
#include <vector>
#include <map>
#include <fmt/core.h>

#include "debug.h"
#include "defs.h"
#include "items.h"
#include "battle.h"

namespace lurp {

class Inventory;
struct ScriptAssets;
class ScriptBridge;

struct Script
{
	struct Event {
		EntityID entityID;
		ScriptType type;
	};
	EntityID entityID;
	EntityID npc;
	int code = -1;	// called when script is started
	std::vector<Event> events;

	static constexpr ScriptType type{ ScriptType::kScript };
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("Script entityID: {}\n", entityID);
		for (const auto& e : events) {
			fmt::print("{: >{}}", "", depth * 2 + 2);
			fmt::print("Event: {}\n", e.entityID);
		}
	}
};

struct TextLine {
	std::string speaker;
	std::string text;
	bool alreadyRead = false;
};

struct Text {
	struct Line {
		std::string speaker;
		std::string text;

		int eval = -1;
		std::string test;
		int code = -1;		// called when this line is read
	};

	EntityID entityID;
	std::vector<Line> lines;

	int eval = -1;		// return true if this option should be presented
	std::string test;
	int code = -1;		// called when this object is read

	Text copyWithoutLines() const {
		Text t;
		t.entityID = entityID;
		t.eval = eval;
		t.test = test;
		t.code = code;
		return t;
	}

	static constexpr ScriptType type{ ScriptType::kText };
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("Text entityID: {}\n", entityID);
		for (const auto& line : lines) {
			fmt::print("{: >{}}", "", depth * 2 + 2);
			fmt::print("{}: {}\n", line.speaker, line.text);
		}
	}
};

struct Choices {
	enum class Action {
		kDone,		// Done with choices. Move to next event.
		kRewind,	// Move to the start of the current script.
		kRepeat,	// Repeat the current choices.
		kPop,		// Pop the current script context.
	};

	struct Choice {
		std::string text;
		EntityID next;
		bool read = false;
		int eval = -1;	// called to view choice
		int code = -1;	// called if choice is selected
		int unmapped = 0;	// original un-mapped index. not serialized.
	};

	EntityID entityID;
	std::vector<Choice> choices;
	Action action = Action::kDone;

	Choices copyWithoutChoices() const {
		Choices c;
		c.entityID = entityID;
		c.action = action;
		return c;
	}

	static constexpr ScriptType type{ ScriptType::kChoices };
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("Choice entityID: {}\n", entityID);
		for (const auto& c : choices) {
			fmt::print("{: >{}}", "", depth * 2 + 2);
			fmt::print("{} -> {}\n", c.text, c.next);
		}
	}
};

struct Actor {
	EntityID entityID;
	std::string name;
	bool wild = false;
	int fighting = 0;
	int shooting = 0;
	int arcane = 0;
	Inventory inventory;
	std::vector<EntityID> powers;

	static constexpr ScriptType type{ ScriptType::kActor };
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("Actor {} '{}'\n", entityID, name);
	}

	std::pair<bool, Variant> get(const std::string& key) const {
		if (key == "name") return { true, Variant(name) };
		return { false, Variant() };
	}
};

struct Combatant {
	EntityID entityID;
	std::string name;
	bool wild = false;
	int count = 1;
	int fighting = 0;
	int shooting = 0;
	int arcane = 0;
	int bias = 0;

	Inventory inventory;
	std::vector<EntityID> powers;

	static constexpr ScriptType type{ ScriptType::kCombatant };
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("Combatant {} '{}'\n", entityID, name);
	}
};

struct Battle {
	EntityID entityID;

	std::string name;
	std::vector<lurp::swbattle::Region> regions;
	std::vector<EntityID> combatants;

	std::pair<bool, Variant> get(const std::string&) const {
		return { false, Variant() };
	}

	static constexpr ScriptType type{ ScriptType::kBattle };
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		//fmt::print("Battle {} 'player' v {}\n", entityID, enemy);
	}
};

} // namespace lurp
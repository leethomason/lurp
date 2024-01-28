#pragma once

#include <string>
#include <vector>
#include <map>
#include <fmt/core.h>

#include "debug.h"
#include "defs.h"
#include "items.h"

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
	int code = -1;	// called when script is started
	std::vector<Event> events;

	std::string type() const { return "Script"; }
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

	std::string type() const { return "Text"; }
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

	std::string type() const { return "Choices"; }
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
	Inventory inventory;

	std::string type() const { return "Actor"; }
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("Actor {} '{}'\n", entityID, name);
	}

	std::pair<bool, Variant> get(const std::string& key) const {
		if (key == "name") return { true, Variant(name) };
		return { false, Variant() };
	}
};

struct Battle {
	EntityID entityID;
	EntityID enemy;

	std::pair<bool, Variant> get(const std::string&) const {
		return { false, Variant() };
	}

	std::string type() const { return "Battle"; }
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("Battle {} 'player' v {}\n", entityID, enemy);
	}
};

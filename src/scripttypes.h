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

struct Script : Entity
{
	struct Event {
		EntityID entityID;
		ScriptType type;
	};
	EntityID npc;
	int code = -1;	// called when script is started
	std::vector<Event> events;

	static constexpr ScriptType type{ ScriptType::kScript };

	virtual std::string description() const override {
		return fmt::format("Script entityID: {}\n", entityID);
	}

	virtual std::pair<bool, Variant> getVar(const std::string& k) const override {
		if (k == "npc") return { true, Variant(npc) };
		return { false, Variant() };
	}

	virtual ScriptType getType() const override {
		return type;
	}
};

struct TextLine {
	std::string speaker;
	std::string text;
	bool alreadyRead = false;
};

struct Text : Entity {
	struct Line {
		std::string speaker;
		std::string text;

		int eval = -1;
		std::string test;
		int code = -1;		// called when this line is read
	};

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

	virtual std::string description() const override {
		return fmt::format("Text entityID: {}\n", entityID);
	}

	virtual std::pair<bool, Variant> getVar(const std::string&) const override {
		return { false, Variant() };
	}

	virtual ScriptType getType() const override {
		return type;
	}

	struct SubLine {
		std::string speaker;
		std::string test;
		std::string text;
	};
	static std::vector<SubLine> subParse(const std::string& str);

private:
	static size_t findSubLine(size_t start, const std::string& str);
};

struct Choices : Entity {
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

	std::vector<Choice> choices;
	Action action = Action::kDone;

	Choices copyWithoutChoices() const {
		Choices c;
		c.entityID = entityID;
		c.action = action;
		return c;
	}

	static constexpr ScriptType type{ ScriptType::kChoices };

	virtual std::string description() const override {
		return fmt::format("Choice entityID: {}\n", entityID);
	}

	virtual std::pair<bool, Variant> getVar(const std::string&) const override {
		return { false, Variant() };
	}

	virtual ScriptType getType() const override {
		return type;
	}
};

struct Actor : Entity {
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

	virtual std::string description() const override {
		return fmt::format("Actor entityID: {}\n", entityID);
	}

	virtual std::pair<bool, Variant> getVar(const std::string& k) const override {
		if (k == "name") return { true, Variant(name) };
		if (k == "wild") return { true, Variant(wild) };
		// These can be changed.
		//if (k == "fighting") return { true, Variant(fighting) };
		//if (k == "shooting") return { true, Variant(shooting) };
		//if (k == "arcane") return { true, Variant(arcane) };
		return { false, Variant() };
	}

	virtual ScriptType getType() const override {
		return type;
	}
};

struct Combatant : Entity {
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

	virtual std::string description() const override {
		return fmt::format("Combatant entityID: {}\n", entityID);
	}

	virtual std::pair<bool, Variant> getVar(const std::string& k) const override {
		if (k == "name") return { true, Variant(name) };
		if (k == "wild") return { true, Variant(wild) };
		if (k == "count") return { true, Variant(count) };
		if (k == "fighting") return { true, Variant(fighting) };
		if (k == "shooting") return { true, Variant(shooting) };
		if (k == "arcane") return { true, Variant(arcane) };
		if (k == "bias") return { true, Variant(bias) };
		return { false, Variant() };
	}

	virtual ScriptType getType() const override {
		return type;
	}
};

struct Battle : Entity {
	std::string name;
	std::vector<lurp::swbattle::Region> regions;
	std::vector<EntityID> combatants;

	std::pair<bool, Variant> get(const std::string&) const {
		return { false, Variant() };
	}

	static constexpr ScriptType type{ ScriptType::kBattle };

	virtual std::string description() const override {
		return fmt::format("Battle entityID: {}\n", entityID);
	}

	virtual std::pair<bool, Variant> getVar(const std::string& k) const override {
		if (k == "name") return { true, Variant(name) };
		return { false, Variant() };
	}

	virtual ScriptType getType() const override {
		return type;
	}
};

} // namespace lurp
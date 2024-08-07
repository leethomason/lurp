#pragma once

#include <string>
#include <vector>
#include <map>
#include <fmt/core.h>

#include "debug.h"
#include "defs.h"
#include "items.h"
#include "battle.h"
#include "markdown.h"

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
		return fmt::format("Script entityID: {} nEvents={}", entityID, events.size());
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
		return fmt::format("Text entityID: {} nLines={}", entityID, lines.size());
	}

	virtual std::pair<bool, Variant> getVar(const std::string&) const override {
		return { false, Variant() };
	}

	virtual ScriptType getType() const override {
		return type;
	}

	// Parses a small `md = "..."` string into a vector of Text::Line.
	static std::vector<Text::Line> parseMarkdown(const std::string& md);

	// Parses a full file
	static std::vector<Text> parseMarkdownFile(const std::string& md);
		
private:
	struct ParseData {
		std::string entityID;
		std::string speaker;
		std::string test;
		std::vector<Text::Line> lines;
	};
	static Text flushParseData(ParseData& data);

	static bool isMDTag(const std::string& str);
	static size_t findMDTag(size_t start, const std::string& str);
	static void parseMDTag(const std::string& str, std::string& speaker, std::string& test);

	static void paragraphHandler(const MarkDown&, const std::vector<MarkDown::Span>&, int level);
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
		return fmt::format("Choice entityID: {}", entityID);
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
		return fmt::format("Actor entityID: {}", entityID);
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
		return fmt::format("Combatant entityID: {}", entityID);
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

	static constexpr ScriptType type{ ScriptType::kBattle };

	virtual std::string description() const override {
		return fmt::format("Battle entityID: {}", entityID);
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
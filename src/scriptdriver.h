#pragma once

#include "debug.h"
#include "scripttypes.h"
#include "tree.h"
#include "iscript.h"
#include "mapdata.h"

#include <string>
#include <vector>
#include <unordered_set>

namespace lurp {

struct ScriptAssets;
class ScriptHelper;
class CoreData;
class ZoneDriver;

class ScriptDriver : public ITextHandler
{
public:
	// The ScriptHelper binds the player/npc/map/zone to the scripting environment.
	//		It is optional; if not provided, the script has minimal functionality.
	// The readText set is used to track which text has been read by the player.
	//		It is optional; if not provided, all text is considered unread. This will
	//      certainly break some scripts. But useful to be null for testing.
	ScriptDriver(
		const ScriptAssets& assets,
		const ScriptEnv& env,
		MapData& mapData,
		ScriptBridge& bridge,
		int initCode = -1);

	ScriptDriver(ZoneDriver& zoneDriver, const EntityID& scriptID);

	// Loader version.
	ScriptDriver(
		const ScriptAssets& assets,
		const EntityID& scriptID,
		MapData& mapData,
		ScriptBridge& bridge,
		ScriptBridge& loader);

	~ScriptDriver();

	bool done() const;
	void clear();
	bool valid() const { return _tree.size() > 0; }
	const ScriptEnv& env() const { return _scriptEnv; }

	// kText
	// kChoices
	// kBattle
	ScriptType type() const;

	TextLine line();					// kText
	const Choices& choices() const;		// kChoices
	const Battle& battle() const;		// kBattle

	void advance();
	void choose(int i);					// kChoices

	const Tree* getTree() const { return &_tree;}
	void save(std::ostream& stream) const;
	virtual bool allTextRead(const EntityID& id) const;

	const ScriptHelper* helper() const { return _helper.get(); }

private:
	ScriptEnv loadEnv(ScriptBridge& loader) const;
	bool loadContext(ScriptBridge& loader);

	bool parseAction(const std::string& str, Choices::Action& action) const;

	// eval() the active choices and do text substitution
	Text filterText(const Text& text) const;
	Choices filterChoices(const Choices& choices) const;

	void processTree(bool step);
	std::string substitute(const std::string& in) const;
	bool textTest(const std::string& test) const;

	const ScriptAssets& _assets;
	ScriptEnv _scriptEnv;
	MapData& _mapData;
	
	std::unique_ptr<ScriptHelper> _helper;
	Tree _tree;
	TreeIt _treeIt;

	int _textSubIndex = 0;
	std::vector<Choices::Action> _choicesStack;

	Choices _mappedChoices;		// Choices: filter down to what passed 'eval'
	Text _mappedText;			// Text: filter down to what passed 'eval' and 'test'
};

} // namespace lurp
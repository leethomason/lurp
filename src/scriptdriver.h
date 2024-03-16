#pragma once

#include "debug.h"
#include "scripttypes.h"
#include "tree.h"
#include "iscript.h"
#include "mapdata.h"
#include "varbinder.h"

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
	ScriptDriver(ZoneDriver& zoneDriver, ScriptBridge& bridge, const EntityID& scriptID, int initLuaFunc = -1);
	ScriptDriver(ZoneDriver& zoneDriver, ScriptBridge& bridge, const ScriptEnv& env, int initLuaFunc = -1);

	virtual ~ScriptDriver();

	bool done() const;
	void abort();
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
	virtual bool allTextRead(const EntityID& id) const;

	const ScriptHelper* helper() const { return _helper.get(); }
	VarBinder varBinder() const;

	void save(std::ostream& stream) const;
	bool load(ScriptBridge& loader);
	static void saveScriptEnv(std::ostream& stream, const ScriptEnv& env);
	static ScriptEnv loadScriptEnv(ScriptBridge& loader);


private:
	bool parseAction(const std::string& str, Choices::Action& action) const;

	// eval() the active choices and do text substitution
	Text filterText(const Text& text) const;
	Choices filterChoices(const Choices& choices) const;

	void processTree(bool step);
	std::string substitute(const std::string& in) const;
	bool textTest(const std::string& test) const;

	const ScriptAssets& _assets;
	ScriptBridge& _bridge;
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
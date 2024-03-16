#include "scriptdriver.h"
#include "debug.h"
#include "util.h"
#include "scriptasset.h"
#include "scripthelper.h"
#include "battle.h"
#include "scriptbridge.h"	// fixme: only needed for "bridge()" which should be refactored
#include "zonedriver.h"

#include <fmt/core.h>
#include <fmt/ostream.h>

namespace lurp {

bool ScriptDriver::parseAction(const std::string& s, Choices::Action& action) const
{
	action = Choices::Action::kDone;
	if (s == "done") action = Choices::Action::kDone;
	else if (s == "pop") action = Choices::Action::kPop;
	else if (s == "rewind") action = Choices::Action::kRewind;
	else if (s == "repeat") action = Choices::Action::kRepeat;
	else {
		return false;
	}
	return true;
}

/*
ScriptDriver::ScriptDriver(
	const ScriptAssets& assets,
	const ScriptEnv& env,
	MapData& mapData,
	ScriptBridge& bridge,
	int initCode)
	:	_scriptEnv(env),
		_assets(assets),
		_mapData(mapData),
		_tree(assets, env.script),
		_treeIt(_tree)
{
	assert(!env.script.empty());
	assert(!env.player.empty());

	_helper = std::make_unique<ScriptHelper>(bridge, _mapData.coreData, env);
	_helper->bridge().setIText(this);
	_helper->pushScriptContext();
	_helper->call(initCode, 0);

	processTree(false);
}
*/

ScriptDriver::ScriptDriver(ZoneDriver& zoneDriver, ScriptBridge& bridge, const EntityID& scriptID, int func)
	:	_assets(zoneDriver.assets()),
		_bridge(bridge),
		_mapData(zoneDriver.mapData),
		_tree(zoneDriver.assets(), scriptID),
		_treeIt(_tree)
{
	if (scriptID.empty()) {
		assert(func < 0);	// shouldn't call a func on load
	}
	else {
		assert(_assets.get(scriptID)->getType() == ScriptType::kScript);
	}

	_scriptEnv.script = scriptID;
	_scriptEnv.zone = zoneDriver.currentZone().entityID;
	_scriptEnv.room = zoneDriver.currentRoom().entityID;
	_scriptEnv.player = zoneDriver.getPlayer().entityID;

	_helper = std::make_unique<ScriptHelper>(bridge, _mapData.coreData, _scriptEnv);
	_helper->bridge().setIText(this);
	_helper->pushScriptContext();
	_helper->call(func, 0);

	processTree(false);
}

ScriptDriver::ScriptDriver(ZoneDriver& zoneDriver, ScriptBridge& bridge, const ScriptEnv& env, int func)
	: _assets(zoneDriver.assets()),
	_bridge(bridge),
	_mapData(zoneDriver.mapData),
	_tree(zoneDriver.assets(), env.script),
	_treeIt(_tree)
{
	_scriptEnv = env;

	_helper = std::make_unique<ScriptHelper>(bridge, _mapData.coreData, _scriptEnv);
	_helper->bridge().setIText(this);
	_helper->pushScriptContext();
	_helper->call(func, 0);

	processTree(false);
}

/*
ScriptDriver::ScriptDriver(
	const ScriptAssets& assets,
	const EntityID& scriptID,
	MapData& mapData,
	ScriptBridge& bridge,
	ScriptBridge& loader)
	:	_assets(assets),
		_mapData(mapData),
		_tree(assets, scriptID),
		_treeIt(_tree)
{
	assert(!scriptID.empty());
	assert(_assets.get(scriptID).type == ScriptType::kScript);

	_scriptEnv = loadEnv(loader);
	assert(_scriptEnv.script == scriptID);
	_helper = std::make_unique<ScriptHelper>(bridge, _mapData.coreData, _scriptEnv);

	_helper->bridge().setIText(this);
	_helper->pushScriptContext();

	bool okay = loadContext(loader);
	if (!okay) {
		clear();
	}
}
*/

ScriptDriver::~ScriptDriver()
{
	if (_helper.get()) {
		_helper->popScriptContext();
		_helper->bridge().setIText(nullptr);
	}
	//assert(_choicesStack.empty());
}

void ScriptDriver::abort()
{
	_treeIt.setIndex(_tree.size());
	_choicesStack.clear();
}

void ScriptDriver::clear()
{
	_scriptEnv = ScriptEnv();
	_tree.clear();
	_treeIt.setIndex(0);
	_textSubIndex = 0;
	_choicesStack.clear();
}

bool ScriptDriver::done() const
{
	return _treeIt.done();
}

// The *outer* code() needs to be run if eval passes. It may mutate the lines,
// which is an obvious thing to do. The *inner* code() is run for each line
// when the driver gets to the line.
Text ScriptDriver::filterText(const Text& text) const
{
	Text result = text.copyWithoutLines();

	bool outerEval = true;
	outerEval = _helper->call(text.eval, 1);
	outerEval = outerEval && textTest(text.test);
	if (!outerEval) {
		return result;
	}
	_helper->call(text.code, 0);

	for (const Text::Line& line : text.lines) {
		bool eval = true;
		eval = _helper->call(line.eval, 1);
		eval = eval && textTest(line.test);

		if (!eval) continue;

		Text::Line tl;
		tl.speaker = substitute(line.speaker);
		tl.text = substitute(line.text);
		result.lines.push_back(tl);
	}
	return result;
}

Choices ScriptDriver::filterChoices(const Choices& choices) const
{
	Choices result = choices.copyWithoutChoices();

	for (size_t i = 0; i < choices.choices.size(); ++i) {
		const Choices::Choice& c = choices.choices[i];

		bool pass = true;
		if (_helper && c.eval >= 0) {
			pass = _helper->call(c.eval, 1);
		}
		if (pass) {
			result.choices.push_back(c);
			result.choices.back().text = substitute(c.text);
			result.choices.back().unmapped = (int)i;
		}
	}
	return result;
}

ScriptType ScriptDriver::type() const
{
	if (done()) return ScriptType::kNone;
	return _treeIt.get().type;
}

std::string ScriptDriver::substitute(const std::string& in) const
{
	if (!_helper) return in;
	std::vector<size_t> open;
	std::vector<size_t> close;

	for (size_t i = 0; i < in.size(); ++i) {
		if (in[i] == '{') 
			open.push_back(i);
		else if (in[i] == '}') 
			close.push_back(i);
	}
	assert(open.size() == close.size());
	if (open.empty())
		return in;

	std::string out;
	size_t prev = 0;
	for (size_t i = 0; i < open.size(); i++) {
		out += in.substr(prev, open[i] - prev);
		prev = close[i] + 1;

		// Subtle. This doesn't work:
		//		VarBinder binder = _helper->varBinder();
		// because this ScriptDriver might have modified the npc.
		VarBinder binder(_bridge, _mapData.coreData, _scriptEnv);

		Variant v = binder.get(in.substr(open[i] + 1, close[i] - open[i] - 1));
		if (v.type == LUA_TSTRING)
			out += v.str;
		else if (v.type == LUA_TNUMBER)
			out += std::to_string(int(v.num));
		else
			assert(false);
	}
	out += in.substr(prev);
	return out;
}

bool ScriptDriver::textTest(const std::string& test) const
{
	if (!_helper) return true;
	if (test.size() < 2) return true;
	std::string t = test.substr(1, test.size() - 2);	// remove the brackets
	bool invert = false;
	if (t[0] == '~') {
		invert = true;
		t = t.substr(1);
	}
	VarBinder binder(_bridge, _mapData.coreData, _scriptEnv);
	Variant v = binder.get(t);
	bool truthy = v.isTruthy();
	return invert ? !truthy : truthy;
}


TextLine ScriptDriver::line()
{
	assert(!done());
	assert(type() == ScriptType::kText);
	assert(_textSubIndex < int(_mappedText.lines.size()));
	Text::Line& line = _mappedText.lines[_textSubIndex];
	TextLine textLine;
	textLine.speaker = line.speaker;
	textLine.text = line.text;

	// FIXME: refactor into MapData
	uint64_t hash = Hash(line.speaker, line.text);

	if (_mapData.textRead.find(hash) == _mapData.textRead.end()) {
		_mapData.textRead.insert(hash);
		textLine.alreadyRead = false;
	}
	else {
		textLine.alreadyRead = true;
	}
	return textLine;
}

void ScriptDriver::processTree(bool step)
{
	bool done = false;

	while (!done) {
		if (step)
			_treeIt.next();
		if (_treeIt.done())
			break;
		step = true;

		//fmt::print("Processing: {:2}\n", _tree->index());

		NodeRef node = _treeIt.getNode();
		if (node.leading) {
			ScriptRef ref = node.ref;
			if (ref.type == ScriptType::kScript) {
				const Script& script = _assets._csa.scripts[ref.index];
				if (!script.npc.empty()) {
					bool okay = _helper->callGlobal("SetupNPCEnv", { script.npc }, 1);
					if (okay) 
						_scriptEnv.npc = script.npc;
				}
				_helper->call(script.code, 0);
			}
			else if (ref.type == ScriptType::kText) {
				const Text& text = _assets._csa.texts[ref.index];
				_mappedText = filterText(text);
				_textSubIndex = 0;
				done = !_mappedText.lines.empty();
			}
			else if (ref.type == ScriptType::kChoices) {
				const Choices& choices = _assets._csa.choices[ref.index];
				_mappedChoices = filterChoices(choices);
				_choicesStack.push_back(choices.action);

				if (_mappedChoices.choices.empty()) {
					_treeIt.forwardTE();
					step = false;
				}
				else {
					done = true;
					_choicesStack.back() = _mappedChoices.action;
				}
			}
			else if (ref.type == ScriptType::kBattle) {
				done = true;
			}
			else if (ref.type == ScriptType::kCallScript) {
				const CallScript& callScript = _assets._csa.callScripts[ref.index];
				bool eval = true;
				eval = _helper->call(callScript.eval, true);
				if (eval) {
					if (!callScript.npc.empty()) {
						bool okay = _helper->callGlobal("SetupNPCEnv", { callScript.npc }, 1);
						if (okay)
							_scriptEnv.npc = callScript.npc;
					}
					_helper->call(callScript.code, false);
				}
				else {
					_treeIt.forwardTE();
					step = false;
				}
			}
			else {
				assert(false);	// not actually fatal, just unexpected - what's in the tree??
				done = true;
			}
		}
		else {
			if (node.ref.type == ScriptType::kScript) {
				//popScriptContext();
			}
			else if (node.ref.type == ScriptType::kChoices) {
				Choices::Action action = _choicesStack.back();
				_choicesStack.pop_back();
				if (action == Choices::Action::kDone) {
					// do nothing; natural course of traversal.
				}
				else if (action == Choices::Action::kRepeat) {
					// repeat the choices 
					_treeIt.rewindLE();
					step = false;
				}
				else if (action == Choices::Action::kRewind) {
					// back to start of script
					_treeIt.firstSibLE();
					step = false;
				}
				else if (action == Choices::Action::kPop) {
					_treeIt.lastSibTE();
					step = true;	// step because the trailing edge shouldn't be processed without a leading edge
				}
				else {
					assert(false);
				}
			}
			// Handle Choices. If the parent is a Choice, then we don't want to run the siblings.
			ScriptRef parentRef = _treeIt.getParent();
			if (parentRef.type == ScriptType::kChoices) {
				_treeIt.parentTE();
				step = false;
			}
		}
	}
}

void ScriptDriver::advance()
{
	NodeRef node = _treeIt.getNode();
	ScriptRef ref = node.ref;

	if (ref.type == ScriptType::kText) {
		assert(!_mappedText.lines.empty());
		_textSubIndex++;
		if (_textSubIndex >= int(_mappedText.lines.size())) {
			processTree(true);
		}
	}
	else {
		processTree(true);
	}

	// if the advance() moved us to a line of text, need to run the code()
	if (_helper && !done()) {
		ScriptRef newRef = _treeIt.get();
		if (newRef.type == ScriptType::kText) {
			_helper->call(_mappedText.lines[_textSubIndex].code, 0);
		}
	}
}

const Choices& ScriptDriver::choices() const
{
	assert(!done());
	assert(type() == ScriptType::kChoices);
	assert(!_mappedChoices.choices.empty());
	return _mappedChoices;
}

const Battle& ScriptDriver::battle() const
{
	assert(!done());
	assert(type() == ScriptType::kBattle);
	ScriptRef ref = _treeIt.get();
	assert(ref.type == ScriptType::kBattle);
	return _assets._csa.battles[ref.index];
}

void ScriptDriver::choose(int i)
{
	assert(!done());
	assert(type() == ScriptType::kChoices);
	assert(!_mappedChoices.choices.empty());

	Choices::Choice c = _mappedChoices.choices[i];

	if (_helper && c.code >= 0) {
		_helper->call(c.code, 0);
	}
	Choices::Action action = Choices::Action::kDone;
	if (parseAction(c.next, action)) {
		_choicesStack.back() = action;
		_treeIt.forwardTE();
		processTree(false);
	}
	else {
		_treeIt.childLE(c.unmapped);
		processTree(false);
	}
}

bool ScriptDriver::allTextRead(const EntityID& id) const
{
	ScriptRef ref = _assets.getScriptRef(id);
	int index = -1;

	const TreeVec& tree = _tree.tree();
	for (size_t i = 0; i < tree.size(); ++i) {
		if (tree[i].ref == ref) {
			index = (int)i;
			break;
		}
	}
	if (index < 0) return true;

	int end = _tree.getNodeTE(index) + 1;
	for (int i = index; i < end; ++i) {
		if (tree[i].leading && tree[i].ref.type == ScriptType::kText) {
			const Text& text = _assets._csa.texts[tree[i].ref.index];
			Text filtered = filterText(text);
			for (const Text::Line& line : filtered.lines) {
				uint64_t hash = Hash(line.speaker, line.text);
				if (_mapData.textRead.find(hash) == _mapData.textRead.end()) {
					return false;
				}
			}
		}
	}
	return true;
}

/*static*/ void ScriptDriver::saveScriptEnv(std::ostream& stream, const ScriptEnv& env)
{
	fmt::print(stream, "ScriptEnv = {{\n");
	fmt::print(stream, "  script = '{}',\n", env.script);
	fmt::print(stream, "  zone = '{}',\n", env.zone);
	fmt::print(stream, "  room = '{}',\n", env.room);
	fmt::print(stream, "  player = '{}',\n", env.player);
	fmt::print(stream, "  npc = '{}',\n", env.npc);
	fmt::print(stream, "}}\n");
}

/*static*/ ScriptEnv ScriptDriver::loadScriptEnv(ScriptBridge& loader)
{
	lua_State* L = loader.getLuaState();
	ScriptBridge::LuaStackCheck check(L);

	loader.pushGlobal("ScriptEnv");
	ScriptEnv env;
	env.script = loader.getStrField("script", { "" });
	env.zone = loader.getStrField("zone", { "" });
	env.room = loader.getStrField("room", { "" });
	env.player = loader.getStrField("player", { "" });
	env.npc = loader.getStrField("npc", { "" });
	loader.pop();

	return env;
}

void ScriptDriver::save(std::ostream& stream) const
{
	saveScriptEnv(stream, _scriptEnv);

	// Always true? But needs to be a leading edge
	// so that when the load occurs, the state is set
	// up correctly.
	assert(_treeIt.getNode().leading == true);	

	fmt::print(stream, "ScriptDriver = {{\n");
	fmt::print(stream, "  treeItIndex = {}, textSubIndex = {}, treeSize = {},\n", _treeIt.index(), _textSubIndex, _tree.size());
	fmt::print(stream, "  entityID = '{}',\n", _treeIt.getNode().entityID);
	fmt::print(stream, "  choicesStack = {{\n");
	for(auto& c : _choicesStack) {
		fmt::print(stream, "    {},\n", (int)c);
	}
	fmt::print(stream, "  }},\n");
	fmt::print(stream, "}}\n");
}

bool ScriptDriver::load(ScriptBridge& loader)
{
	lua_State* L = loader.getLuaState();
	ScriptBridge::LuaStackCheck check(L);

	_scriptEnv = loadScriptEnv(loader);

	loader.pushGlobal("ScriptDriver");

	EntityID entityID = loader.getStrField("entityID", { "" });
	int treeItIndex = loader.getIntField("treeItIndex", { 0 });
	int textSubIndex = loader.getIntField("textSubIndex", { 0 });

	_tree = Tree(_assets, _scriptEnv.script);
	//_tree.dump(_assets);

	loader.pushTable("choicesStack");

	for (TableIt it(L, -1); !it.done(); it.next()) {
		if (it.kType() == LUA_TNUMBER) {
			int v = (int) lua_tointeger(L, -1);
			_choicesStack.push_back((Choices::Action)v);
		}
	}
	lua_pop(L, 1);
	lua_pop(L, 1);

	const NodeRef& nodeRef = _tree.getNode(treeItIndex);

	if (treeItIndex < _tree.size()
		&& nodeRef.entityID == entityID
		&& nodeRef.leading)
	{
		_treeIt.setIndex(treeItIndex);
		_textSubIndex = textSubIndex;

		processTree(false);

		return true;
	}
	_treeIt.setIndex(0);
	_choicesStack.clear();
	_tree.clear();

	fmt::print("WARNING could not load ScriptDriver. Rolling back to Map.\n");
	return false;
}

VarBinder ScriptDriver::varBinder() const
{
	VarBinder binder(_bridge, _mapData.coreData, _scriptEnv);
	return binder;
}

const char* scriptTypeName(ScriptType type)
{
	switch (type) {
	case ScriptType::kNone: return "None";
	case ScriptType::kScript: return "Script";
	case ScriptType::kText: return "Text";
	case ScriptType::kChoices: return "Choices";
	case ScriptType::kItem: return "Item";
	case ScriptType::kInteraction: return "Interaction";
	case ScriptType::kRoom: return "Room";
	case ScriptType::kZone: return "Zone";
	case ScriptType::kBattle: return "Battle";
	case ScriptType::kContainer: return "Container";
	case ScriptType::kEdge: return "Edge";
	case ScriptType::kActor: return "Actor";
	case ScriptType::kCallScript: return "CallScript";
	default:
		assert(false);
	}
	return "error";
}

} // namespace lurp

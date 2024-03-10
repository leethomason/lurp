#include "tree.h"
#include "scriptasset.h"

#include <assert.h>

namespace lurp {

void walkTree(const ScriptAssets& assets, const EntityID& entityID, int depth, std::vector<NodeRef>& tree)
{
	if (entityID.empty()) {
		// happens when we create an empty script for loading.
		return;
	}

	ScriptRef ref = assets.get(entityID);

	// If the driver can't point at it, doesn't need to be in tree.
	switch (ref.type) {
	case ScriptType::kItem:
	case ScriptType::kRoom:	// a room has objects in it, but not scripts
	case ScriptType::kContainer:
	case ScriptType::kEdge:
	case ScriptType::kActor:
		return;
	default:
		break;
	}

	tree.push_back({ ref, entityID, depth, true });

	switch (ref.type) {
	case ScriptType::kScript: {
		const Script& script = assets._csa.scripts[ref.index];
		for (const Script::Event& e : script.events) {
			walkTree(assets, e.entityID, depth + 1, tree);
		}
		break;
	}
	case ScriptType::kChoices: {
		const Choices& choices = assets._csa.choices[ref.index];
		for (const Choices::Choice& c : choices.choices) {
			walkTree(assets, c.next, depth + 1, tree);
		}
		break;
	}
	case ScriptType::kInteraction: {
		const Interaction& interaction = assets._csa.interactions[ref.index];
		walkTree(assets, interaction.next, depth + 1, tree);
		break;
	}
	case ScriptType::kZone: {
		const Zone& zone = assets._csa.zones[ref.index];
		for (const EntityID& e : zone.objects) {
			walkTree(assets, e, depth + 1, tree);
		}
		break;
	}
	case ScriptType::kCallScript: {
		const CallScript& callScript = assets._csa.callScripts[ref.index];
		walkTree(assets, callScript.scriptID, depth + 1, tree);
		break;
	}
	case ScriptType::kBattle:
	case ScriptType::kText: {
		// do nothing -- simply record these are here.
		break;
	}
	{
	default:
		assert(false);	// all cases should have been handled!!

	}
	}
	tree.push_back({ ref, entityID, depth, false });
}

std::vector<NodeRef> createTree(const ScriptAssets& assets, const EntityID& scriptID)
{
	std::vector<NodeRef> tree;
	walkTree(assets, scriptID, 0, tree);
	return tree;
}

Tree::Tree(const ScriptAssets& assets, const EntityID& entityID)
{
	_tree = createTree(assets, entityID);
}

ScriptRef TreeIt::next()
{
	_index++;
	return _index < _tree.size() ? _tree._tree[_index].ref : ScriptRef();
}

bool TreeIt::done() const
{
	return _index >= _tree.size();
}

void Tree::dump(const ScriptAssets& assets) const
{
	//int idx = 0;
	for (const NodeRef& ref : _tree) {
		//fmt::print("{:2}: ", idx++);
		fmt::print("{: >{}}", "", 2 + ref.depth * 2);
		std::string name = scriptTypeName(ref.ref.type);
		std::string id;
		if (ref.ref.type == ScriptType::kScript)
			id = assets._csa.scripts[ref.ref.index].entityID;
		else if (ref.ref.type == ScriptType::kChoices)
			id = assets._csa.choices[ref.ref.index].entityID;
		fmt::print("{} {} {} {}\n", name, ref.ref.index, id, ref.leading ? "L" : "T");
	}
}

void TreeIt::rewindLE()
{
	int depth = cDepth(_index);
	while (true) {
		if (_tree._tree[_index].depth == depth && _tree._tree[_index].leading == true)
			break;
		_index--;
	}
}

void TreeIt::forwardTE()
{
	int depth = cDepth(_index);
	while (true) {
		if (_tree._tree[_index].depth == depth && _tree._tree[_index].leading == false)
			break;
		_index++;
	}
}

void TreeIt::firstSibLE()
{
	int depth = cDepth(_index);
	if (depth == 0) {
		_index = 0;
		return;
	}
	parentLE();
	_index++;
}

void TreeIt::lastSibTE()
{
	int depth = cDepth(_index);
	if (depth == 0) {
		_index = (int)(_tree.size() - 1);
		return;
	}
	parentTE();
	_index--;
}

void TreeIt::parentTE()
{
	_index = _tree.getParentTE(_index);
}

void TreeIt::parentLE()
{
	_index = _tree.getParentLE(_index);
}

int Tree::getParentTE(int index) const
{
	int depth = cDepth(index);
	if (depth == 0) {
		return (int)_tree.size() - 1;
	}
	while (true) {
		if (_tree[index].depth == depth - 1 && _tree[index].leading == false)
			break;
		index++;
	}
	return index;
}

int Tree::getParentLE(int index) const
{
	int depth = cDepth(index);
	if (depth == 0) {
		return 0;
	}
	while (true) {
		if (_tree[index].depth == depth - 1 && _tree[index].leading == true)
			break;
		index--;
	}
	return index;
}

void TreeIt::childLE(int n)
{
	int depth = cDepth(_index);
	rewindLE();
	assert(cDepth(_index) == depth);
	while (true) {
		if (_tree._tree[_index].depth == depth + 1 && _tree._tree[_index].leading == true) {
			if (n == 0)
				return;
			n--;
		}
		_index++;
	}
}

int Tree::getNodeTE(int index) const
{
	assert(_tree[index].leading == true);
	int depth = cDepth(index);
	while (true) {
		if (_tree[index].depth == depth && _tree[index].leading == false)
			break;
		index++;
	}
	return index;
}


ScriptRef TreeIt::getParent() const
{
	int index = _tree.getParentTE(_index);
	return _tree.get(index);
}

} // namespace lurp

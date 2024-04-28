#pragma once

#include "defs.h"

#include <assert.h>
#include <vector>

namespace lurp {

struct ScriptAssets;
class TreeIt;

struct NodeRef {
	ScriptRef ref;
	EntityID entityID;
	int depth = 0;
	bool leading = true;
};

using TreeVec = std::vector<NodeRef>;

TreeVec createTree(const ScriptAssets& assets, const EntityID& entityID);

class Tree {
	friend TreeIt;
public:
	Tree(const ScriptAssets& assets, const EntityID& entityID);

	const TreeVec& tree() const { return _tree; }
	void clear() { _tree.clear(); }
	void dump(const ScriptAssets& assets) const;

	int size() const { return (int)_tree.size(); }

	int getNodeTE(int index) const;	// given the LE, return TE index
	int getParentTE(int index) const;
	int getParentLE(int index) const;

	ScriptRef get(int index) const { assert(index >= 0 && index < size()); return _tree[index].ref; }
	NodeRef getNode(int index) const { assert(index >= 0 && index < size());  return _tree[index]; }

	void log() const;

private:
	int cDepth(int index) const { return _tree[index].depth; }
	TreeVec _tree;
};

class TreeIt {
public:
	TreeIt(const Tree& tree) : _tree(tree) {}

	ScriptRef get() const { return _tree.get(_index); }
	ScriptRef getParent() const;
	NodeRef getNode() const { return _tree.getNode(_index); }

	ScriptRef next();
	bool done() const;

	void rewindLE();		// back to leading edge of this node
	void forwardTE();		// forward to trailing edge of this node
	void firstSibLE();		// rewind to first leading edge of sibling 0 (which could be the current one)
	void lastSibTE();		// forward to the trailing edge of theh last sibling (which could be the current one)
	void childLE(int n);    // move to the leading edge of the nth child
	void parentTE();		// move to the trailing edge of the parent
	void parentLE();		// move to the leading edge of the parent

	int index() const { return _index; }
	void setIndex(int i) { _index = i; }
private:
	int cDepth(int index) const { return _tree._tree[index].depth; }

	const Tree& _tree;
	int _index = 0;
};

} // namespace lurp
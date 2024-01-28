#pragma once

#include "defs.h"

#include <fmt/core.h>
#include <string>
#include <vector>
#include <assert.h>

struct ConstScriptAssets;

struct Item {
	EntityID entityID;
	std::string name;

	bool operator==(const Item& rhs) const { return entityID == rhs.entityID; }

	std::string type() const { return "Item"; }
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("Item entityID: {} '{}'\n", entityID, name);
	}
};

struct ItemRef {
	const Item* pItem = nullptr;
	int count = 1;

	bool operator==(const ItemRef& rhs) const { return pItem == rhs.pItem && count == rhs.count; }
};

class Inventory {
public:
	Inventory() = default;
	Inventory(const Inventory&) = default;
	~Inventory() = default;
	Inventory& operator=(const Inventory&) = default;
	bool operator==(const Inventory& rhs) const { return _items == rhs._items; }
	bool operator!=(const Inventory& rhs) const { return !(*this == rhs); }
	
	bool emtpy() const { return _items.empty(); }
	void clear() { _items.clear(); }
	
	void addInitItem(const EntityID & entityID, int n) { _initItems.push_back({ entityID, n }); }
	void convert(const ConstScriptAssets& assets);

	bool hasItem(const Item& item) const;
	int findItem(const Item& item) const;
	int numItems(const Item& entityID) const;

	void addItem(const Item& item, int n = 1);
	void removeItem(const Item& item, int n = 1);
	void deltaItem(const Item& item, int n);

	int size() const { return (int)_items.size(); }

	const std::vector<ItemRef>& items() const { return _items; }

	// items = { { "GOLD", 10 }, "SWORD" }
	void save(std::ostream& stream);

	std::string type() const { return "Inventory"; }
	void dump(int d) const {
		fmt::print("{: >{}}Inventory", "", d * 2);
	};

	friend void transfer(const Item& item, Inventory& src, Inventory& dst, int n = INT_MAX);
	friend void transferAll(Inventory& src, Inventory& dst);

private:
	auto findIt(const Item& item) {
		return std::find_if(_items.begin(), _items.end(), [&](const ItemRef& it) {
			if (it.pItem->entityID == item.entityID) {
				assert(it.pItem == &item);	// should be unique
				return true;
			}
			return false;
			});
	}
	const auto findIt(const Item& item) const {
		return std::find_if(_items.begin(), _items.end(), [&](const ItemRef& it) {
			if (it.pItem->entityID == item.entityID) {
				assert(it.pItem == &item);	// should be unique
				return true;
			}
			return false;
			});
	}

	ItemRef findRef(const Item& item);
	std::vector<ItemRef> _items;
	std::vector<std::pair<EntityID, int>> _initItems;
};

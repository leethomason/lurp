#pragma once

#include "defs.h"
#include "die.h"

#include <fmt/core.h>
#include <string>
#include <vector>
#include <assert.h>

namespace lurp {

struct ConstScriptAssets;

struct Item {
	EntityID entityID;
	std::string name;
	std::string desc;
	int range = 0;
	int armor = 0;
	int ap = 0;
	Die damage = { 0, 0, 0 };

	bool isMeleeWeapon() const { return range == 0 && damage.d > 0 && armor == 0; }
	bool isRangedWeapon() const { return range > 0 && damage.d > 0 && armor == 0; }
	bool isArmor() const { return armor > 0 && damage.d == 0; }
	
	static constexpr ScriptType type{ ScriptType::kItem };
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("Item entityID: {} '{}'\n", entityID, name);
	}
};

struct Power {
	EntityID entityID;
	std::string name;
	std::string effect;
	int cost = 1;			// 1, 2, 3
	int range = 1;			// 0, 1, 2+
	int strength = 1;		// -3, -2, -1, 1, 2, 3

	static constexpr ScriptType type{ ScriptType::kPower };
	void dump(int depth) const {
		fmt::print("{: >{}}", "", depth * 2);
		fmt::print("Power entityID: {} '{}' {} cost={} range={} strength={}\n", entityID, name, effect, cost, range, strength);
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

	void addInitItem(const EntityID& entityID, int n) { _initItems.push_back({ entityID, n }); }
	void convert(const ConstScriptAssets& assets);

	bool hasItem(const Item& item) const;
	int findItem(const Item& item) const;
	int numItems(const Item& entityID) const;

	void addItem(const Item& item, int n = 1);
	void removeItem(const Item& item, int n = 1);
	void deltaItem(const Item& item, int n);

	const Item* meleeWeapon() const;
	const Item* rangedWeapon() const;
	const Item* armor() const;

	int size() const { return (int)_items.size(); }

	const std::vector<ItemRef>& items() const { return _items; }

	// items = { { "GOLD", 10 }, "SWORD" }
	void save(std::ostream& stream);

	static constexpr ScriptType type{ ScriptType::kInventory };
	void dump(int d) const {
		fmt::print("{: >{}}Inventory", "", d * 2);
	};

	friend void transfer(const Item& item, Inventory& src, Inventory& dst, int n = INT_MAX);

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

} // namespace lurp
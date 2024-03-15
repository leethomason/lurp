#pragma once

#include "defs.h"
#include "die.h"

#include <fmt/core.h>
#include <string>
#include <vector>
#include <assert.h>

namespace lurp {

struct ConstScriptAssets;

struct Item : public Entity {
	std::string name;
	std::string desc;
	int range = 0;
	int armor = 0;
	int ap = 0;
	Die damage = { 0, 0, 0 };

	Item() = default;
	Item(const std::string& name, const std::string& desc) : name(name), desc(desc) {}

	bool isMeleeWeapon() const { return range == 0 && damage.d > 0 && armor == 0; }
	bool isRangedWeapon() const { return range > 0 && damage.d > 0 && armor == 0; }
	bool isArmor() const { return armor > 0 && damage.d == 0; }
	
	static constexpr ScriptType type{ ScriptType::kItem };

	virtual std::string description() const override {
		return fmt::format("Item entityID: {} '{}'\n", entityID, name);
	}
	virtual std::pair<bool, Variant> getVar(const std::string& k) const override {
		if (k == "name") return { true, Variant(name) };
		if (k == "desc") return { true, Variant(desc) };
		if (k == "range") return { true, Variant(range) };
		if (k == "armor") return { true, Variant(armor) };
		if (k == "ap") return { true, Variant(ap) };
		if (k == "damage") return { true, Variant(damage.toString()) };
		return { false, Variant() };
	}
	virtual ScriptType getType() const override {
		return type; 
	}

};

struct Power : Entity {
	std::string name;
	std::string effect;
	int cost = 1;			// 1, 2, 3
	int range = 1;			// 0, 1, 2+
	int strength = 1;		// -3, -2, -1, 1, 2, 3

	static constexpr ScriptType type{ ScriptType::kPower };
	
	virtual std::string description() const override {
		return fmt::format("Power entityID: {} '{}' {} cost={} range={} strength={}\n", entityID, name, effect, cost, range, strength);
	}
	virtual std::pair<bool, Variant> getVar(const std::string& k) const override {
		if (k == "name") return { true, Variant(name) };
		if (k == "effect") return { true, Variant(effect) };
		if (k == "cost") return { true, Variant(cost) };
		if (k == "range") return { true, Variant(range) };
		if (k == "strength") return { true, Variant(strength) };
		return { false, Variant() };
	}
	virtual ScriptType getType() const override {
		return type;
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
	std::string description() const {
		return "Inventory";
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
#include "items.h"
#include "scriptasset.h"
#include "scripttypes.h"

#include <fmt/core.h>
#include <fmt/ostream.h>

namespace lurp {

int Inventory::findItem(const Item& item) const {
	auto it = findIt(item);
	if (it == _items.end()) return 0;
	return it->count;
}

ItemRef Inventory::findRef(const Item& item)
{
	auto it = findIt(item);
	if (it == _items.end()) return ItemRef();
	return *it;
}


void Inventory::addItem(const Item& item, int n) {
	auto it = findIt(item);
	if (it == _items.end()) {
		ItemRef ref{ &item, n };
		_items.push_back(ref);
	}
	else {
		it->count += n;
	}
}

void Inventory::deltaItem(const Item& item, int n)
{
	if (n > 0) addItem(item, n);
	else removeItem(item, -n);
}

bool Inventory::hasItem(const Item& item) const {
	return numItems(item) > 0;
}

int Inventory::numItems(const Item& item) const
{
	auto it = findIt(item);
	if (it == _items.end()) return 0;
	return it->count;
}

void Inventory::removeItem(const Item& item, int n) {
	auto it = findIt(item);
	if (it == _items.end()) return;
	it->count -= n;
	if (it->count > 0) return;
	_items.erase(it);
}

void transfer(const Item& item, Inventory& src, Inventory& dst, int n)
{
	ItemRef ref = src.findRef(item);
	if (n > ref.count) n = ref.count;
	src.removeItem(item, n);
	dst.addItem(item, n);
}

void transfer(Inventory& src, Inventory& dst)
{
	while (!src.items().empty()) {
		ItemRef ref = src.items()[0];
		transfer(*ref.pItem, src, dst);
	}
}

void Inventory::convert(const ConstScriptAssets& assets)
{
	std::map<EntityID, const Item*> itemMap;
	for (auto& item : assets.items) {
		itemMap[item.entityID] = &item;
	};

	for (auto& ii : _initItems) {
		const Item* pItem = itemMap[ii.first];
		assert(pItem);
		const Item& item = *pItem;
		addItem(item, ii.second);
	}
	_initItems.clear();
}

void Inventory::save(std::ostream& stream)
{
	fmt::print(stream, "items = {{ ");
	for (const auto& item : this->items()) {
		if (item.count == 1)
			fmt::print(stream, "\"{}\", ", item.pItem->entityID);
		else
			fmt::print(stream, "{{ \"{}\", {} }}, ", item.pItem->entityID, item.count);
	}
	fmt::print(stream, "}} ");
}

} // namespace lurp
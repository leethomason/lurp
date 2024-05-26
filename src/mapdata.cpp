#include "mapdata.h"
#include "scriptbridge.h"

namespace lurp {

MapData::MapData(uint32_t seed)
{
	assert(seed);
	random.setSeed(seed);
}

NewsItem NewsItem::itemDelta(const Item& item, int delta, int count) 
{
	NewsItem ni;
	ni.type = NewsType::kItemDelta;
	ni.item = &item;
	ni.delta = delta;
	ni.count = count;

	if (delta == 1) {
		ni.text = fmt::format("One {} added to inventory.", item.name);
	} 
	else if (delta > 1) {
		ni.text = fmt::format("{} {}s added to inventory.", delta, item.name);
	}
	else if (delta == -1) {
		ni.text = fmt::format("One {} removed from inventory.", item.name);
	}
	else if (delta < -1) {
		ni.text = fmt::format("{} {}s removed from inventory.", -delta, item.name);
	}
	return ni;
}

NewsItem NewsItem::lock(bool lock, const Container& c, const Item* item) 
{
	NewsItem ni;
	ni.type = lock ? NewsType::kLocked : NewsType::kUnlocked;
	ni.container = &c;
	ni.item = item;

	if (!item) {
		if (lock)
			ni.text = fmt::format("The {} is now locked.", c.name);
		else
			ni.text = fmt::format("The {} is unlocked.", c.name);
	}
	else {
		if (lock)
			ni.text = fmt::format("The {} is locked with the {}.", c.name, item->name);
		else
			ni.text = fmt::format("The {} is unlocked by the {}.", c.name, item->name);
	}

	return ni;
}

NewsItem NewsItem::lock(bool lock, const Edge& e, const Item* item) 
{
	NewsItem ni;
	ni.type = lock ? NewsType::kLocked : NewsType::kUnlocked;
	ni.edge = &e;
	ni.item = item;

	if (!item) {
		if (lock)
			ni.text = fmt::format("The {} is now locked.", e.name);
		else
			ni.text = fmt::format("The {} is unlocked.", e.name);
	}
	else {
		if (lock)
			ni.text = fmt::format("The {} is locked with the {}.", e.name, item->name);
		else
			ni.text = fmt::format("The {} is unlocked by the {}.", e.name, item->name);
	}

	return ni;
}

NewsItem NewsItem::edgeLocked(const Edge& e) 
{
	NewsItem ni;
	ni.type = NewsType::kEdgeLocked;
	ni.edge = &e;
	ni.text = fmt::format("The {} is locked.", e.name);
	return ni;
}

NewsItem NewsItem::containerLocked(const Container& c) 
{
	NewsItem ni;
	ni.type = NewsType::kContainerLocked;
	ni.container = &c;
	ni.text = fmt::format("The {} is locked.", c.name);
	return ni;
}

} // namespace lurp

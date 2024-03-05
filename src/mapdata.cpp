#include "mapdata.h"
#include "scriptbridge.h"

namespace lurp {

MapData::MapData(ScriptBridge* bridge, uint32_t seed) : coreData(bridge)
{
	assert(seed);
	random.setSeed(seed);
}

std::string NewsItem::noun() const {
	if (type == NewsType::kItemDelta) return item->name;
	if (type == NewsType::kLocked || type == NewsType::kUnlocked) {
		if (edge) return edge->name;
		if (container) return container->name;
	}
	return "";
}

std::string NewsItem::verb() const {
	if (type == NewsType::kItemDelta) return delta > 0 ? "gained" : "lost";
	if (type == NewsType::kLocked) return "locked";
	if (type == NewsType::kUnlocked) return "unlocked";
	return "";
}

std::string NewsItem::object() const {
	if (type == NewsType::kItemDelta) return item->name;
	if (type == NewsType::kLocked || type == NewsType::kUnlocked) {
		return item ? item->name : "";
	}
	return "";
}

} // namespace lurp

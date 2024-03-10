#pragma once

#include "util.h"
#include "coredata.h"

#include <stdint.h>
#include <unordered_set>
#include <queue>

namespace lurp {

class ScriptBridge;
struct Item;
struct Edge;
struct Container;

enum class NewsType {
	kItemDelta,		// You gained/lost 'delta' 'item'. For a total of 'count'.
	kLocked,		// The 'edge/container' was locked by 'null/item'
	kUnlocked,		// The 'edge/container' was unlocked by 'null/item'
};

struct NewsItem {
	NewsType type = NewsType::kItemDelta;
	const Item* item = nullptr;
	const Edge* edge = nullptr;
	const Container* container = nullptr;
	int delta = 0;
	int count = 0;

	std::string noun() const;
	std::string verb() const;
	std::string object() const;

	static NewsItem itemDelta(const Item& item, int delta, int count) {
		NewsItem ni;
		ni.type = NewsType::kItemDelta;
		ni.item = &item;
		ni.delta = delta;
		ni.count = count;
		return ni;
	}
	static NewsItem lock(bool lock, const Container& c, const Item* item) {
		NewsItem ni;
		ni.type = lock ? NewsType::kLocked : NewsType::kUnlocked;
		ni.container = &c;
		ni.item = item;
		return ni;
	}
	static NewsItem lock(bool lock, const Edge& e, const Item* item) {
		NewsItem ni;
		ni.type = lock ? NewsType::kLocked : NewsType::kUnlocked;
		ni.edge = &e;
		ni.item = item;
		return ni;
	}
};

using NewsQueue  = Queue<NewsItem>;

struct MapData
{
	static constexpr uint32_t kSeed = 0x12345678;

	MapData(uint32_t seed);
	Random random;
	std::unordered_set<uint64_t> textRead;
	CoreData coreData;
	NewsQueue newsQueue;
};

} // namespace lurp
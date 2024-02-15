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

struct NewsQueue {
	static constexpr size_t kMaxSize = 16;

	void push(const NewsItem& ni) {
		while (queue.size() >= kMaxSize)
			queue.pop();
		queue.push(ni);
	}

	NewsItem pop() {
		if (queue.empty())
			return NewsItem();
		NewsItem ni = queue.front();
		queue.pop();
		return ni;
	}

	bool empty() const {
		return queue.empty();
	}

	size_t size() const {
		return queue.size();
	}

	std::queue<NewsItem> queue;
};

struct MapData
{
	MapData(ScriptBridge* bridge, uint32_t seed = 0);
	Random random;
	std::unordered_set<uint64_t> textRead;
	CoreData coreData;
	NewsQueue newsQueue;
};

} // namespace lurp
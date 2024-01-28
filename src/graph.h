#pragma once

#include <vector>
#include <algorithm>

template<typename NODE, typename EDGE>
class Graph
{
public:
	struct Segment {
		NODE node0;
		NODE node1;
		EDGE edge;

		bool operator==(const Segment& rhs) const { return rhs.node0 == this->node0; }
		bool operator!=(const Segment& rhs) const { return rhs.node0 != this->node0; }
		bool operator<(const Segment& rhs) const { return node0 < rhs.node0; }
	};

	void addEdge(const NODE& a, const NODE& b, const EDGE& e, bool biDir = false) {
		segments.push_back({ a, b, e });
		if (biDir) {
			segments.push_back({ b, a, e });
		}
    	std::sort(segments.begin(), segments.end());
	}

	bool hasNode(const NODE& a) const {
		Segment s = { a, a, EDGE{} };
		return std::binary_search(segments.begin(), segments.end(), s);
	}
    
	const auto adjacent(const NODE& a) const {
		Segment s = { a, a, EDGE{} };
		return std::equal_range(segments.begin(), segments.end(), s);
	}

	std::vector<Segment> adjacentVec(const NODE& a) const {
		std::vector<Segment> result;
		const auto range = adjacent(a);
		result.reserve(std::distance(range.first, range.second));
		for (auto it = range.first; it != range.second; ++it) {
			result.push_back(*it);
		}
		return result;
	}

private:
	std::vector<Segment> segments;
};

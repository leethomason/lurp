#pragma once

#include <assert.h>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <queue>

namespace lurp {

std::vector<std::string_view> splitSV(const std::string& str, char delim);
std::vector<std::string> split(const std::string& str, char delim);

inline std::string toLower(const std::string& str) {
	std::string s = str;
	std::transform(str.begin(), str.end(), s.begin(),
		[](unsigned char c) { return (unsigned char)std::tolower(c); });
	return s;
}

struct Globals
{
	static bool debugSave;
};

struct DieRoll
{
	DieRoll() {}
	DieRoll(int n, int d, int b = 0) : n(n), d(d), b(b) {}

	// default: 1d6+0
	int n = 1;
	int d = 6;
	int b = 0;
};

class Random
{
public:
	Random() : s(1234) {}
	Random(int seed) {
		setSeed(seed);
	}

	void setSeed(uint32_t seed) {
		s = (seed > 0) ? seed : 1;
		rand();	rand(); rand();
	}

	void setRandomSeed();

	uint32_t rand() {
		// xorshift
		s ^= s << 13;
		s ^= s >> 7;
		s ^= s << 17;
		return uint32_t(s);
	}

	int32_t rand(int32_t limit) { return static_cast<int32_t>(rand() % limit); }
	uint32_t rand(uint32_t limit) { return rand() % limit; }

	int dice(int count, int sides, int bonus = 0) {
		int total = 0;
		for (int i = 0; i < count; ++i) {
			total += rand(sides) + 1;
		}
		return total + bonus;
	}

	int dice(const DieRoll& roll) {
		return dice(roll.n, roll.d, roll.b);
	}

	template<typename T>
	void shuffle(T begin, T end) {
		for (T it = begin; it != end; ++it) {
			std::swap(*it, *(begin + rand(int(end - begin))));
		}
	}

	double uniform() {
		return rand(10'001) / 10'000.0f;
	}

private:
	uint64_t s;
};

template <typename T>
T clamp(T value, T min, T max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

uint64_t Hash(const std::string& a, const std::string& b = "", const std::string& c = "", const std::string& d = "");

// NOTE: checks if value is in [low, high)
template <typename T>
bool within(const T& value, const T& low, const T& high) {
	return !(value < low) && (value < high);
}

template <typename T>
T sign(T a) {
	assert(T(-1) < T(0));
	if (a > 0) return T(1);
	if (a < 0) return T(-1);
	return T(0);
}

template<typename T>
struct Queue {
	static constexpr size_t kMaxSize = 32;

	void push(const T& ni) {
		while (queue.size() >= kMaxSize)
			queue.pop();
		queue.push(ni);
	}

	T pop() {
		if (queue.empty())
			return T();
		T ni = queue.front();
		queue.pop();
		return ni;
	}

	bool empty() const {
		return queue.empty();
	}

	size_t size() const {
		return queue.size();
	}

	void clear() {
		while (!queue.empty())
			queue.pop();
	}

	std::queue<T> queue;
};


} // namespace lurp
#pragma once

#include <assert.h>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <queue>
#include <mutex>

namespace lurp {

std::vector<std::string_view> splitSV(const std::string& str, char delim);
std::vector<std::string> split(const std::string& str, char delim);

inline std::string toLower(const std::string& str) {
	std::string s = str;
	std::transform(str.begin(), str.end(), s.begin(),
		[](unsigned char c) { return (unsigned char)std::tolower(c); });
	return s;
}

// Given: "aaa{B=c, {D=e}}"
// start=3, returns length of string (one past the closing brace)
size_t parseRegion(const std::string& str, size_t start, char open, char close);

// Given: A=1, B = {22},C=3, D= \"4 4\", E='55 55' 
// Returns: [A=1, B=22, C=3, D="4 4", E="55 55"]
std::vector<std::string> parseKVPairs(const std::string& str);

// Given: E='55 55'
// Returns: E="55 55"
std::pair<std::string, std::string> parseKV(const std::string& str, char* sepOut = nullptr);

static constexpr char kWhitespace[] = " \t\n\r";

inline std::string trimRight(const std::string& s) {
	size_t p = s.find_last_not_of(kWhitespace);
	return s.substr(0, p + 1);
}

inline std::string trimLeft(const std::string& s) {
	size_t p = s.find_first_not_of(kWhitespace);
	return p == std::string::npos ? "" : s.substr(p);
}

inline std::string trim(const std::string& s) {
	return trimRight(trimLeft(s));
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

	// Used to create a seed for the random number generator
	static uint32_t getTime();

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
			std::swap(*it, *(begin + rand(uint32_t(end - begin))));
		}
	}

	// Tricky
	// Want [0.0, 1.0]
	// double (64 bits) has 52 bits of mantissa, which is huge. Might as well use a 
	// full 32-bit random number. There's tricks to pack the bits, but they usually
	// are over the range [0.0, 1.0) and I doubt it makes a difference.
	double uniform() {
		return rand() / double(std::numeric_limits<uint32_t>::max());
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

// Thread safe, multi-reader multi-writer queue
// Not very efficient, but fine for small queues
template<typename T>
struct QueueMT {

	void push(const T& t) {
		std::lock_guard<std::mutex> lock(mutex);
		queue.push(t);
	}

	bool tryPop(T& t) {
		std::lock_guard<std::mutex> lock(mutex);
		if (queue.empty())
			return false;
		t = queue.front();
		queue.pop();
		return true;
	}

private:
	std::queue<T> queue;
	std::mutex mutex;
};

template<typename T, size_t N>
class RollingAverage {
public:
	RollingAverage() {
		for (size_t i = 0; i < N; ++i)
			values[i] = 0;
	}

	void add(T value) {
		sum -= values[index];
		values[index] = value;
		sum += value;
		index = (index + 1) % N;
	}

	T average() const {
		return sum / N;
	}
private:
	T values[N];
	T sum = 0;
	size_t index = 0;
};

} // namespace lurp
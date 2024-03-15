#pragma once

#include <limits>
#include <algorithm>
#include <string>

namespace lurp {

class Random;

static constexpr int kMaxNumAces = 4;

struct Die {
	Die() {}
	Die(int n, int d, int b) : n(n), d(d), b(b) {}
	int roll(Random&, int* nAce);

	int n = 1;
	int d = 4;
	int b = 0;

	bool isValid() const { return n > 0 && d > 0; }
	int summary() const { return n * d + b; }
	std::string toString() const;

	static Die noDie() { return Die(0, 0, 0); }
	static Die parse(const std::string& str);

	bool operator==(const Die& rhs) const {
		return n == rhs.n && d == rhs.d && b == rhs.b;
	}
	bool operator!=(const Die& rhs) const {
		return !(*this == rhs);
	}
};

struct Roll {
	// Note that the ace system is only used for the n=1
	Die die;				// what was rolled

	int total[2] = { std::numeric_limits<int>::min(), std::numeric_limits<int>::min() };

	int value() const {
		int v = std::max(total[0], total[1]);
		if (v == std::numeric_limits<int>::min()) return 0;
		return v;
	}
	Die selected(int which) const {
		if (which == 0) return die;
		Die d = die;
		d.d = 6;
		return d;
	}
	int nAce(int which) const {
		Die sd = selected(which);
		int base = total[which] - sd.b;	// roll w/ aces
		return base / (sd.d * sd.n);
	}
	int finalRoll(int which) const {
		Die sd = selected(which);
		return total[which] - sd.b - nAce(which) * sd.d;
	}
	bool hasWild() const { return total[1] != std::numeric_limits<int>::min(); }

	bool criticalFailure() const {
		for (int i = 0; i < 2; i++) {
			if (total[i] == std::numeric_limits<int>::min()) continue;
			if (nAce(i) > 0) return false;
			if (finalRoll(i) != 1) return false;
		}
		return true;
	}
};


} // namespace lurp

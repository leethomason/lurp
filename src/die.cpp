#include "die.h"
#include "util.h"

#include <vector>
#include <variant>
#include <assert.h>

namespace lurp {

int Die::roll(Random& random, int* _nAce)
{
	int total = 0;
	int nAce = 0;

	while (true) {
		// All rolls can ace
		int roll = random.dice(n, d, 0);
		// But computers can't ace forever
		if (nAce == kMaxNumAces && roll == n * d) {
			roll = n * d - 1;
		}
		total += roll;
		if (roll < n * d)
			break;
		nAce++;
	}
	if (_nAce)
		*_nAce = nAce;

	return total + b;	// bonus only applies to total
}

Die Die::parse(const std::string& str)
{
	Die d = { 0, 0, 0 };
	// 6
	// d6
	// 1d6
	// 1d6+2

	// Actually kind of a pain to parse
	std::vector<std::variant<char, int>> tokens;

	for(size_t i=0; i<str.size(); ++i)
	{
		if (str[i] == 'd') {
			tokens.push_back({'d'});
		} else if (str[i] == '+') {
			tokens.push_back({'+' });
		} else if (str[i] == '-') {
			tokens.push_back({'-'});
		}
		else if (isdigit(str[i])) {
			std::string num;
			while (isdigit(str[i])) {
				num += str[i];
				++i;
			}
			--i;
			tokens.push_back(std::stoi(num));
		}
	}
	// Brute force.
	if (tokens.size() == 1) {
		// 6
		d.n = 1;
		d.d = std::get<int>(tokens[0]);
	} else if (tokens.size() == 2) {
		// d6
		d.n = 1;
		d.d = std::get<int>(tokens[1]);
	} else if (tokens.size() == 3) {
		// 1d6
		d.n = std::get<int>(tokens[0]);
		assert(std::get<char>(tokens[1]) == 'd');
		d.d = std::get<int>(tokens[2]);
	} else if (tokens.size() == 4) {
		// 1d6+2
		d.n = std::get<int>(tokens[0]);
		assert(std::get<char>(tokens[1]) == 'd');
		d.d = std::get<int>(tokens[2]);
		if (std::get<char>(tokens[3]) == '+') {
			d.b = std::get<int>(tokens[4]);
		} else if (std::get<char>(tokens[3]) == '-') {
			d.b = -std::get<int>(tokens[4]);
		}
	}
	return d;
}


} // namespace lurp
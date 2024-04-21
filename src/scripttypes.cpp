#include "scripttypes.h"

namespace lurp {

/*static*/ std::vector<Text::SubLine> Text::subParse(const std::string& str)
{
	// Currently uses very particular parsing. This could be made more robust
	// by using RegEx.
	std::vector<SubLine> lines;

	std::vector<size_t> starts;
	size_t start = 0;
	constexpr int NUM_PATTERNS = 2;
	static const char* pattern[NUM_PATTERNS] = { "{s=", "{test=" };

	for (int i = 0; i < NUM_PATTERNS; i++) {
		while (start < str.size()) {
			start = str.find(pattern[i], start);
			if (start == std::string::npos)
				break;
			starts.push_back(start);
			start++;
		}
	}
	std::sort(starts.begin(), starts.end());
}

} // namespace lurp
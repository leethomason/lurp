#include "scripttypes.h"

namespace lurp {

/*static*/ size_t Text::findSubLine(size_t start, const std::string& str)
{
	size_t s = str.find("{s=", start);
	size_t t = str.find("{test=", start);

	if (s < str.size() && t < str.size())
		return std::min(s, t);
	if (s < str.size())
		return s;
	return t;
}

/*static*/ std::vector<Text::SubLine> Text::subParse(const std::string& str)
{
	// Currently uses very particular parsing. This could be made more robust
	// by using RegEx.
	std::vector<SubLine> lines;

	std::string speaker;
	std::string test;

	size_t start = 0;
	size_t next = 0;
	while (start < str.size()) {
		next = findSubLine(start, str);
		if (next == std::string::npos)
			break;

		// Flush the current subline
		if (next > start) {
			SubLine subLine{ speaker, test, str.substr(start, next - start) };
			lines.push_back(subLine);
			speaker.clear();
			test.clear();
		}

		size_t end = parseRegion(str, next, '{', '}');
		std::string region = str.substr(next + 1, end - next - 2);
		std::vector<std::string> pairs = parseKVPairs(region);
		for (const std::string& pair : pairs) {
			std::pair<std::string, std::string> kv = parseKV(pair);
			if (kv.first == "s")
				speaker = kv.second;
			else if (kv.first == "test")
				test = kv.second;
		}
		start = end;
	}
	// final flush
	if (next > start) {
		SubLine subLine{ speaker, test, str.substr(start, next - start) };
		lines.push_back(subLine);
	}
	return lines;
}

} // namespace lurp
#include "scripttypes.h"

namespace lurp {

/*static*/ bool Text::isMDTag(const std::string& str)
{
	std::string s;
	std::copy_if(str.begin(), str.end(), std::back_inserter(s), [](char c) {return !std::isspace(c); });
	return s.find("[s=") == 0 || s.find("[test=") == 0;
}


/*static*/ size_t Text::findMDTag(size_t start, const std::string& str)
{
	size_t s = str.find("{s=", start);
	size_t t = str.find("{test=", start);

	if (s < str.size() && t < str.size())
		return std::min(s, t);
	if (s < str.size())
		return s;
	return t;
}

/*static*/ void Text::parseMDTag(const std::string& str, std::string& speaker, std::string& test)
{
	size_t end = parseRegion(str, 0, '[', ']');
	std::string region = str.substr(1, end - 2);
	std::vector<std::string> pairs = parseKVPairs(region);
	for (const std::string& pair : pairs) {
		std::pair<std::string, std::string> kv = parseKV(pair);
		if (kv.first == "s")
			speaker = kv.second;
		else if (kv.first == "test")
			test = kv.second;
	}
}

} // namespace lurp
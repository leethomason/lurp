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


/*static*/ size_t Text::parseSubLine(size_t start, const std::string& str, std::string& speaoker, std::string& test)
{
	size_t end = start + 1;
	int count = 0;

	while (end < str.size()) {
		if (str[end] == '{')
			count++;
		else if (str[end] == '}')
			count--;
		end++;
	}
	if (end == str.size()) {
		std::string msg = fmt::format("Unmatched curly braces in Text '{}'", str.substr(0, 20));
		FatalError(msg);
	}
	std::string sub = str.substr(start, end - start);
}


/*static*/ std::vector<Text::SubLine> Text::subParse(const std::string& str)
{
	// Currently uses very particular parsing. This could be made more robust
	// by using RegEx.
	std::vector<SubLine> lines;
	
	std::string speaker;
	std::string test;

	size_t start = 0;
	while (start < str.size()) {
		size_t next = findSubLine(start, str);
		if (next > start) {
			SubLine subLine;
			subLine.speaker = speaker;
			subLine.test = test;
			subLine.text = str.substr(start, next - start);
			lines.push_back(subLine);
			speaker.clear();
			test.clear();
		}
		start = next;
		if (start < str.size()) {
			next = parseSubLine(start, str, speaker, test);
		}
	}
} // namespace lurp
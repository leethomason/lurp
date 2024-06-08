#include "scripttypes.h"
#include "markdown.h"

#define DEBUG_MD 0

namespace lurp {

std::vector<Text::Line> Text::parseMarkdown(const std::string& t)
{
	MarkDown md;

	std::vector<Text::Line> lines;

	std::string speaker;
	std::string test;

	md.paragraphHandler = [&](const MarkDown&, const std::vector<MarkDown::Span>& spans, int level) {
		std::string text;

		for (const MarkDown::Span& span : spans) {
			if (span.text.empty())
				continue;	// superfluous?

			if (span.isText()) {
				if (!text.empty() && !std::isspace(text.back())) {
					text += ' '; // superfluous?
				}
				text += span.text;
			}
			else if (span.code) {
				parseMDTag(span.text, speaker, test);
			}
		}

		Text::Line line;
		line.speaker = speaker;
		line.text = text;
		line.test = test;
		lines.push_back(line);
		};

	md.process(t);
	return lines;
}

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

/*static*/ void Text::parseMDTag(const std::string& _str, std::string& speaker, std::string& test)
{
	std::string str = trim(_str);
	std::vector<std::string> pairs = parseKVPairs(str);
	for (const std::string& pair : pairs) {
		std::pair<std::string, std::string> kv = parseKV(pair);
		if (kv.first == "s")
			speaker = kv.second;
		else if (kv.first == "test")
			test = kv.second;
	}
}

} // namespace lurp
#include "scripttypes.h"
#include "markdown.h"

#define DEBUG_MD 0

namespace lurp {

/*static*/ void Text::paragraphHandler(const MarkDown& md, const std::vector<MarkDown::Span>& spans, int)
{
	std::string text;
	ParseData* data = (ParseData*)md.user;

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
			data->speaker.clear();
			data->test.clear();
			parseMDTag(span.text, data->speaker, data->test);
		}
	}

	if (!text.empty()) {
		Text::Line line;
		line.speaker = data->speaker;
		line.text = text;
		line.test = data->test;
		data->lines.push_back(line);
	}
}

std::vector<Text::Line> Text::parseMarkdown(const std::string& t)
{
	MarkDown md;
	ParseData data;
	md.user = &data;

	md.paragraphHandler = paragraphHandler;
	md.process(t);

	return data.lines;
}

/*static*/ std::vector<Text> Text::parseMarkdownFile(const std::string& t)
{
	MarkDown md;
	ParseData data;
	md.user = &data;
	std::vector<Text> textVec;
	std::string entityID;

	md.paragraphHandler = paragraphHandler;
	md.headingHandler = [&](const MarkDown&, const std::vector<MarkDown::Span>& span, int) {
		//if (data.lines.empty())
		//	return;

		// Flush the current text
		//Text text;
		//text.entityID = entityID;
		//text.lines = data.lines;
		//textVec.push_back(text);

		//data.lines.clear();

		assert(data.lines.empty());	// should be already flushed
		data.speaker.clear();
		data.test.clear();

		entityID = span[0].text;
		};
	md.process(t);

	if (!data.lines.empty()) {
		Text text;
		text.entityID = entityID;
		text.lines = data.lines;
		textVec.push_back(text);
	}
	return textVec;
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
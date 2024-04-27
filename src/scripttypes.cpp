#include "scripttypes.h"
#include "md4c.h"

#define DEBUG_MD 0

namespace lurp {

struct MarkDownHandler
{
	MarkDownHandler() {}
	~MarkDownHandler() {
		CHECK(blockStack.empty());
	}

	std::vector<MD_BLOCKTYPE> blockStack;
	std::vector<Text::Line> lines;

	std::string text;
	std::string speaker;
	std::string test;

	void flush() {
		if (!text.empty()) {
			Text::Line line;
			line.speaker = speaker;
			line.test = test;
			line.text = text;

			lines.push_back(line);
			text.clear();
		}
	}

	static int enterBlock(MD_BLOCKTYPE block, void*, void* user) {
		MarkDownHandler* self = (MarkDownHandler*)user;
#if DEBUG_MD
		fmt::print("Enter block: {}\n", (int)block);
#endif
		self->blockStack.push_back(block);
		return 0;
	}
	static int leaveBlock(MD_BLOCKTYPE block, void*, void* user) {
		MarkDownHandler* self = (MarkDownHandler*)user;
#if DEBUG_MD
		fmt::print("Leave block: {}\n", (int)block);
#endif
		assert(self->blockStack.back() == block);
		self->blockStack.pop_back();

		self->flush();
		return 0;
	}
	static int enterSpan(MD_SPANTYPE span, void*, void* user) {
		(void)span;
		(void)user;
#if DEBUG_MD
		fmt::print("Enter span: {}\n", (int)span);
#endif
		return 0;
	}
	static int leaveSpan(MD_SPANTYPE span, void*, void* user) {
		(void)span;
		(void)user;
#if DEBUG_MD
		fmt::print("Leave span: {}\n", (int)span);
#endif
		return 0;
	}
	static int textHandler(MD_TEXTTYPE type, const MD_CHAR* p, MD_SIZE size, void* user) {
		MarkDownHandler* self = (MarkDownHandler*)user;
		std::string str(p, size);

		if (type == MD_TEXT_NORMAL && self->blockStack.back() == MD_BLOCK_P) {
			if (Text::isMDTag(str)) {
				self->flush();
				Text::parseMDTag(trim(str), self->speaker, self->test);
			}
			else {
				if (!self->text.empty())
					self->text += " ";
				self->text += str;
			}
#if DEBUG_MD
			fmt::print("Normal text: '{}'\n", str);
#endif
		}
		return 0;
	}
};

std::vector<Text::Line> Text::parseMarkdown(const std::string& md)
{
	MD_PARSER parser;
	memset(&parser, 0, sizeof(parser));

	MarkDownHandler handler;
	parser.enter_block = MarkDownHandler::enterBlock;
	parser.leave_block = MarkDownHandler::leaveBlock;
	parser.enter_span = MarkDownHandler::enterSpan;
	parser.leave_span = MarkDownHandler::leaveSpan;
	parser.text = MarkDownHandler::textHandler;

	md_parse(md.c_str(), (MD_SIZE)md.size(), &parser, &handler);

#if DEBUG_MD
	fmt::print("sublines:\n");
	for (const auto& sub : handler.lines) {
		fmt::print("{}: t='{}' '{}'\n", sub.speaker, sub.test, sub.text);
	}
#endif
	return handler.lines;
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
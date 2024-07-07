#pragma once

#include "debug.h"
#include "md4c.h"

#include <functional>

namespace lurp {

class MarkDown
{
public:
	MarkDown() {}
	~MarkDown() {
		CHECK(blockStack.empty());
	}

	struct Span {
		bool bold = false;
		bool italic = false;
		bool image = false;
		bool code = false;
		uint32_t flags() const {
			uint32_t f = 0;
			if (bold) f |= 1;
			if (italic) f |= 2;
			if (image) f |= 4;
			if (code) f |= 8;
			return f;
		}
		size_t stackSize = 0;

		bool isText() const { return !code && !image; }

		std::string text;
	};

	void* user = nullptr;
	std::vector<MD_BLOCKTYPE> blockStack;
	std::vector<Span> spans;

	std::function<void(const MarkDown&, const std::string&, MD_TEXTTYPE)> textHandler;

	using HeadingHandlerFunc = std::function<void(const MarkDown&, const std::vector<Span>&, int level)>;
	using ParagraphHandlerFunc = std::function<void(const MarkDown&, const std::vector<Span>&, int level)>;

	HeadingHandlerFunc headingHandler;
	ParagraphHandlerFunc paragraphHandler;

	void process(const std::string& str);

private:
	static int enterBlock(MD_BLOCKTYPE block, void*, void* user);
	static int leaveBlock(MD_BLOCKTYPE block, void*, void* user);
	static int enterSpan(MD_SPANTYPE span, void*, void* user);
	static int leaveSpan(MD_SPANTYPE span, void*, void* user);
	static int textCallback(MD_TEXTTYPE type, const MD_CHAR* p, MD_SIZE size, void* user);

	void printBlockStack();
	std::vector<MD_SPANTYPE> spanStack;	// tracked but not used
	void printSpanStack();

	Span fromSpanStack() const;
	bool inQuoteBlock() const;

	int _heading = 0;
	int _lastHeading = 0;
};

} // namespace lurp

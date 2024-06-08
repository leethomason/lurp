#pragma once

#include "debug.h"
#include "md4c.h"

#include <functional>

class MarkDownHandler
{
public:
	MarkDownHandler() {}
	~MarkDownHandler() {
		CHECK(blockStack.empty());
	}

	struct Span {
		bool bold = false;
		bool italic = false;
		bool image = false;
		bool code = false;
		size_t stackSize = 0;
		std::string text;
	};

	std::vector<MD_BLOCKTYPE> blockStack;
	std::vector<Span> spans;

	std::function<void(const MarkDownHandler&, const std::string&, MD_TEXTTYPE)> textHandler;

	std::function<void(const MarkDownHandler&, const std::vector<Span>&, int level)> headingHandler;
	std::function<void(const MarkDownHandler&, const std::vector<Span>&, int level)> paragraphHandler;

	void process(const std::string& str);

	static int enterBlock(MD_BLOCKTYPE block, void*, void* user);
	static int leaveBlock(MD_BLOCKTYPE block, void*, void* user);
	static int enterSpan(MD_SPANTYPE span, void*, void* user);
	static int leaveSpan(MD_SPANTYPE span, void*, void* user);
	static int textCallback(MD_TEXTTYPE type, const MD_CHAR* p, MD_SIZE size, void* user);

private:
	void printBlockStack();
	std::vector<MD_SPANTYPE> spanStack;	// tracked but not used
	void printSpanStack();

	Span fromSpanStack() const;
	bool inQuoteBlock() const;

	int _heading = 0;
	int _lastHeading = 0;
};


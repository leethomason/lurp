#include "markdown.h"
#include "debug.h"

#include <fmt/core.h>

#define DEBUG_MD 1

/*static*/ int MarkDownHandler::enterBlock(MD_BLOCKTYPE block, void* arg, void* user) 
{
	MarkDownHandler* self = (MarkDownHandler*)user;
	self->blockStack.push_back(block);

	if (block == MD_BLOCK_H) {
		MD_BLOCK_H_DETAIL* detail = (MD_BLOCK_H_DETAIL*)arg;
		assert(self->_heading == 0);	// heading in heading
		self->_heading = detail->level;
		self->_lastHeading = detail->level;
	}

#if DEBUG_MD
	fmt::print("Enter block: ");
	self->printBlockStack();
	fmt::print("\n");
#endif
	return 0;
}

/*static*/ int MarkDownHandler::leaveBlock(MD_BLOCKTYPE block, void*, void* user)
{
	MarkDownHandler* self = (MarkDownHandler*)user;
	assert(self->blockStack.back() == block);
	self->blockStack.pop_back();

	if (block == MD_BLOCK_H) {
		assert(self->_heading != 0);

		if (self->headingHandler && !self->inQuoteBlock()) 
			self->headingHandler(*self, self->spans, self->_heading);
		self->_heading = 0;
	}
	else if (block == MD_BLOCK_P && !self->inQuoteBlock()) {
		if (self->paragraphHandler)
			self->paragraphHandler(*self, self->spans, self->_lastHeading);
	}
	self->spans.clear();

#if DEBUG_MD
	fmt::print("Leave block: ");
	self->printBlockStack();
	fmt::print("\n");
#endif
	return 0;
}

/*static*/ int MarkDownHandler::enterSpan(MD_SPANTYPE span, void*, void* user)
{
	MarkDownHandler* self = (MarkDownHandler*)user;
	self->spanStack.push_back(span);

#if DEBUG_MD
	fmt::print("Enter span: ");
	self->printSpanStack();
	fmt::print("\n");
#endif
	return 0;
}

/*static*/ int MarkDownHandler::leaveSpan(MD_SPANTYPE span, void*, void* user)
{
	MarkDownHandler* self = (MarkDownHandler*)user;
	assert(self->spanStack.back() == span);
	self->spanStack.pop_back();

#if DEBUG_MD
	fmt::print("Enter span: ");
	self->printSpanStack();
	fmt::print("\n");
#endif
	return 0;
}

/*static*/ int MarkDownHandler::textCallback(MD_TEXTTYPE type, const MD_CHAR* p, MD_SIZE size, void* user)
{
	MarkDownHandler* self = (MarkDownHandler*)user;
	std::string str(p, size);
	if (self->textHandler)
		self->textHandler(*self, str, type);

	if (type == MD_TEXT_SOFTBR) {
		if (!self->spans.empty()) {
			self->spans.back().text += ' ';
		}
	}
	else if (type == MD_TEXT_NORMAL) {
		if (!self->spans.empty() && self->spans.back().stackSize == self->spanStack.size()) {
			// append
			self->spans.back().text += str;
		}
		else {
			Span span = self->fromSpanStack();
			span.text = str;
			self->spans.push_back(span);
		}
	}

#if DEBUG_MD
	fmt::print("Text: '{}'\n", str);
#endif
	return 0;
}

MarkDownHandler::Span MarkDownHandler::fromSpanStack() const
{
	Span span;
	for (MD_SPANTYPE type : spanStack) {
		switch (type) {
		case MD_SPAN_EM: span.italic = true; break;
		case MD_SPAN_STRONG: span.bold = true; break;
		case MD_SPAN_A: span.image = true; break;
		case MD_SPAN_CODE: span.code = true; break;
		default: break;
		}
	}
	span.stackSize = spanStack.size();
	return span;
}

bool MarkDownHandler::inQuoteBlock() const
{
	for (MD_BLOCKTYPE block : blockStack) {
		if (block == MD_BLOCK_QUOTE)
			return true;
	}
	return false;
}

void MarkDownHandler::printBlockStack()
{
	for (MD_BLOCKTYPE block : blockStack) {
		switch (block) {
		case MD_BLOCK_DOC: fmt::print("DOC "); break;
		case MD_BLOCK_QUOTE: fmt::print("QUOTE "); break;
		case MD_BLOCK_UL: fmt::print("UL "); break;
		case MD_BLOCK_OL: fmt::print("OL "); break;
		case MD_BLOCK_LI: fmt::print("LI "); break;
		case MD_BLOCK_HR: fmt::print("HR "); break;
		case MD_BLOCK_H: fmt::print("H "); break;
		case MD_BLOCK_CODE: fmt::print("CODE "); break;
		case MD_BLOCK_HTML: fmt::print("HTML "); break;
		case MD_BLOCK_P: fmt::print("P "); break;
		default: fmt::print("UNKNOWN "); break;
		}
	}
}

void MarkDownHandler::printSpanStack()
{
	for (MD_SPANTYPE span : spanStack) {
		switch (span) {
		case MD_SPAN_EM: fmt::print("EM "); break;
		case MD_SPAN_STRONG: fmt::print("STRONG "); break;
		case MD_SPAN_A: fmt::print("A "); break;
		case MD_SPAN_IMG: fmt::print("IMG "); break;
		case MD_SPAN_CODE: fmt::print("CODE "); break;
		default: fmt::print("UNKNOWN "); break;
		}
	}
}

void MarkDownHandler::process(const std::string& str)
{
	MD_PARSER parser;
	memset(&parser, 0, sizeof(parser));

	parser.enter_block = MarkDownHandler::enterBlock;
	parser.leave_block = MarkDownHandler::leaveBlock;
	parser.enter_span = MarkDownHandler::enterSpan;
	parser.leave_span = MarkDownHandler::leaveSpan;
	parser.text = MarkDownHandler::textCallback;

	md_parse(str.c_str(), (MD_SIZE)str.size(), &parser, this);
}

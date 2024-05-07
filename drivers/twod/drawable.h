#pragma once

#include <stdint.h>

class XFormer;
struct nk_context;

class IDrawable
{
public:
	virtual void load() = 0;
	virtual void draw(const XFormer& xformer, uint64_t frame) = 0;
	virtual void layoutGUI(nk_context* nukCtx, float fontSize, const XFormer& xformer, uint64_t frame) = 0;
};


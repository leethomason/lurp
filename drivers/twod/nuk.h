#pragma once

#include <SDL2/SDL.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#include "nuklear.h"
#include "nuklear_sdl_renderer.h"

#include <string>
#include <vector>

class NukFontAtlas {
public:
	NukFontAtlas(nk_context* ctx) : _ctx(ctx) {}
	void load(const std::string& path, const std::vector<float>& sizes);
	nk_font* select(float size);

private:
	struct Entry {
		float size = 0.0f;
		nk_font* font = nullptr;
	};
	nk_context* _ctx;
	nk_font_atlas* _atlas = nullptr;
	struct nk_font_config nukConfig;
	std::vector<Entry> _entries;
};


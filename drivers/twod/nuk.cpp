#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuk.h"

#include "debug.h"

#include <fmt/core.h>

void NukFontAtlas::load(const std::string& path, const std::vector<float>& sizes)
{
	assert(!sizes.empty());

	nukConfig = nk_font_config(0);
	nk_sdl_font_stash_begin(&_atlas);
	for (float s : sizes) {
		Entry e;
		e.size = s;
		e.font = nk_font_atlas_add_from_file(_atlas, path.c_str(), s, &nukConfig);

		if (!e.font) {
			FatalError(fmt::format("Could not load font '{}'", path));
		}

		_entries.push_back(e);
	}
	nk_sdl_font_stash_end();
	nk_style_set_font(_ctx, &_entries[0].font->handle);
}

nk_font* NukFontAtlas::select(float size, float* sizeUsed)
{
	nk_font* font = nullptr;
	float err = FLT_MAX;

	for (Entry& e : _entries) {
		float err2 = fabs(e.size - size);
		if (err2 < err) {
			err = err2;
			font = e.font;
			if (sizeUsed)	
				*sizeUsed = e.size;
		}
	}
	return font;
}

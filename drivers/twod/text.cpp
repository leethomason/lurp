#include "text.h"
#include "texture.h"
#include "xform.h"
#include "../task.h"

#include <SDL.h>

#include <fmt/core.h>
#include <plog/Log.h>

#define DEBUG_TEXT 1

static int gQuality = 0;

class RenderFontTask : public lurp::SelfDeletingTask
{
public:
	virtual ~RenderFontTask() {}

	virtual void ExecuteRange(enki::TaskSetPartition /*range_*/, uint32_t /*threadnum_*/) override 
	{
		static std::mutex gTTFMutex;
		SDL_Surface* surface = nullptr;
		{
			// TTF, it turns out, is not thread safe.
			std::lock_guard<std::mutex> lock(gTTFMutex);
			const char* s = _text.c_str();
			if (!*s) {
				s = " "; // TTF doesn't like empty strings
				// fixme: return null and clear the texture
			}

			if (_hqOpaque) {
				//if (gQuality == 0)
				//	surface = TTF_RenderUTF8_Shaded_Wrapped(_font, s, _color, _bg, _texture->width());
				//else
				// Lots of fiddling about - liked LCD best in the end.
				surface = TTF_RenderUTF8_LCD_Wrapped(_font, s, _color, _bg, _texture->width());
			}
			else {
				surface = TTF_RenderUTF8_Blended_Wrapped(_font, s, _color, _texture->width());
			}
			assert(surface);
		}

#if DEBUG_TEXT
		fmt::print("Rendered text: '{}' {} chars at {}x{} hq={}\n", _text.substr(0, 20), _text.size(), surface->w, surface->h, _hqOpaque ? 1 : 0);
#endif
		TextureUpdate update{ _texture, surface, _generation, _hqOpaque, _bg };
		_queue->push(update);
	}

	std::shared_ptr<Texture> _texture;
	TextureLoadQueue* _queue = nullptr;
	int _generation = 0;

	TTF_Font* _font = nullptr;
	std::string _text;
	SDL_Color _color;
	SDL_Color _bg;
	bool _hqOpaque = false;
};

FontManager::~FontManager()
{
	for (Font* f : _fonts) {
		TTF_CloseFont(f->font);
		delete f;
	}
}

const Font* FontManager::loadFont(const std::string& path, int virtualSize)
{
	for (Font* f : _fonts) {
		if (f->path == path && f->size == virtualSize) {
			return f;
		}
	}

	Font* font = new Font();
	font->size = virtualSize;
	font->path = path;
	font->font = TTF_OpenFont(path.c_str(), virtualSize);
	assert(font->font);	
	_fonts.push_back(font);

	PLOG(plog::info) << "Loaded font '" << path << "' at size " << virtualSize;
	return font;
}

void FontManager::update(const XFormer& xf)
{
	// Fonts are kept current on the main thread.
	// This also means no tasks can be in flight when the fonts are changed out.
	// So changing fonts is an expensive & blocking operation.
	// Keep the number of fonts low!

	_xf = xf;
	bool change = false;

	// Stage 1: update the fonts if the screen size changed.
	for (Font* f : _fonts) {
		int realSize = xf.s(f->size);
		if (realSize != f->realSize) {
			// Flush the open tasks:
			_pool.WaitforAll();
			change = true;

#if DEBUG_TEXT
			fmt::print("Loading font {} at size {}...", f->path, realSize);
#endif

			TTF_SetFontSize(f->font, realSize);

#if DEBUG_TEXT
			fmt::print("done\n");
#endif

			assert(f->font);
			f->realSize = realSize;
		}
	}

	// Stage 2: update the text fields if the screen size changed.
	if (change) {
		for (auto& tf : _textFields) {
			// The texture needs to be kept 1:1 with the real size, so text doesn't look fuzzy.
			Point size = xf.s(Point{ tf->_virtualSize });
			tf->_texture = _textureManager.createTextField(size.x, size.y);
			tf->_needUpdate = true;
		}
	}

	// Stage 3: update the text fields if they need it, because of font, color, or text changes.
	for (auto& tf : _textFields) {
		if (tf->_needUpdate) {
			tf->_needUpdate = false;
			RenderFontTask* task = new RenderFontTask();
			task->_texture = tf->_texture;
			task->_queue = &_textureManager._loadQueue;
			task->_generation = ++_textureManager._generation;

			task->_font = tf->_font->font;
			task->_color = tf->_color;
			task->_text = tf->_text;

			task->_hqOpaque = tf->_hqOpaque;
			task->_bg = tf->_bg;

			_textureManager._pool.AddTaskSetToPipe(task);
		}
	}
}

Font* FontManager::getFont(const Font* font)
{
	for (Font* f : _fonts) {
		if (f == font)
			return f;
	}
	return nullptr;
}


std::shared_ptr<TextField> FontManager::createTextField(const Font* font, int width, int height, bool useOpaqueHQ, SDL_Color bg)
{
	Font* f = getFont(font);
	assert(f);
	if (!f) return nullptr;

	TextField* tf = new TextField();
	tf->_font = f;
	tf->_virtualSize = Point{ width, height };
	tf->_texture = _textureManager.createTextField(width, height);
	tf->_hqOpaque = useOpaqueHQ;
	tf->_bg = bg;

	std::shared_ptr<TextField> ptr(tf);
	_textFields.push_back(ptr);
	return ptr;	
}

TextField::~TextField()
{
}

void FontManager::draw(std::shared_ptr<TextField>& tf, int vx, int vy)
{
	assert(tf->_font);	
	assert(tf->_font->font); // should be kept up to date in update()

	if (tf->_texture->ready()) {
		Point p = _xf.t(Point{ vx, vy });
		SDL_Rect dst = { p.x, p.y, tf->_texture->width(), tf->_texture->height() };
		if (tf->_hqOpaque)
			SDL_SetTextureBlendMode(tf->_texture->sdlTexture(), SDL_BLENDMODE_NONE);
		else
			SDL_SetTextureBlendMode(tf->_texture->sdlTexture(), SDL_BLENDMODE_BLEND);

		SDL_SetTextureScaleMode(tf->_texture->sdlTexture(), SDL_ScaleMode::SDL_ScaleModeNearest);	// straight copy: shouldn't matter
		SDL_RenderCopy(_renderer, tf->_texture->sdlTexture(), nullptr, &dst);
	}
}

void FontManager::toggleQuality()
{
	static constexpr int kQuality = 2;
	gQuality = (gQuality + 1) % kQuality;
	for (auto& tf : _textFields) {
		tf->_needUpdate = true;
	}
	if (gQuality == 0) fmt::print("Shaded font\n");
	else if (gQuality == 1) fmt::print("LCD font\n");
}


void DrawDebugText(const std::string& text, SDL_Renderer* renderer, const Texture* tex, int x, int y, int fontSize, const XFormer& xf)
{
	if (!tex->ready())
		return;

	PreserveColor pc(renderer);

	constexpr int WIDTH = 512;
	constexpr int HEIGHT = 256;
	constexpr int NX = 16;
	constexpr int NY = 8;
	constexpr int DX = WIDTH / NX;
	constexpr int DY = HEIGHT / NY;

	const int xStep = DX * fontSize / 32;
	const int yStep = DY * fontSize / 32;

	constexpr int border = 4;
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	{
		SDL_Rect rect = xf.t(SDL_Rect{ x - border, y - border, int(text.size()) * xStep + border * 2, yStep + border * 2 });
		SDL_RenderFillRect(renderer, &rect);
	}

	for (size_t i = 0; i < text.size(); i++)
	{
		SDL_Rect src;
		int pos= text[i];
		src.x = (pos % NX) * DX;
		src.y = (pos / NX) * DY;
		src.w = DX;
		src.h = DY;

		SDL_Rect dst;
		dst.x = x + int(i) * xStep;
		dst.y = y;
		dst.w = xStep;
		dst.h = yStep;

		SDL_Rect xDst = xf.t(dst);
		SDL_SetTextureScaleMode(tex->sdlTexture(), SDL_ScaleMode::SDL_ScaleModeNearest);
		SDL_RenderCopy(renderer, tex->sdlTexture(), &src, &xDst);
	};
}
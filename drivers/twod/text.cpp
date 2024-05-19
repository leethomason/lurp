#include "text.h"
#include "texture.h"
#include "xform.h"
#include "../task.h"

#include <SDL.h>

#include <fmt/core.h>
#include <plog/Log.h>

#define DEBUG_TEXT 0
#define DEBUG_TEXT_SAVE 0	// set to 1 to write "surface.bmp" and "font.bmp" to disk to compare pixel quality

static int gQuality = 1;
static constexpr int kScale = 1;

class RenderFontTask : public lurp::SelfDeletingTask
{
public:
	virtual ~RenderFontTask() {}

	virtual void ExecuteRange(enki::TaskSetPartition /*range_*/, uint32_t /*threadnum_*/) override 
	{
		static std::mutex gTTFMutex;
		SDL_Surface* surface = nullptr;
		if (!_text.empty()) {
			// TTF, it turns out, is not thread safe.
			std::lock_guard<std::mutex> lock(gTTFMutex);

			if (_hqOpaque) {
				if (gQuality == 0)
					surface = TTF_RenderUTF8_LCD_Wrapped(_font, _text.c_str(), _color, _bg, _texture->pixelSize().w);
				else
					surface = TTF_RenderUTF8_Shaded_Wrapped(_font, _text.c_str(), _color, _bg, _texture->pixelSize().w);
			}
			else {
				surface = TTF_RenderUTF8_Blended_Wrapped(_font, _text.c_str(), _color, _texture->pixelSize().w);
			}
			assert(surface);
		}
#if DEBUG_TEXT
		if (surface) {
#	if	DEBUG_TEXT_SAVE
			SDL_SaveBMP(surface, "surface.bmp");
#	endif
			fmt::print("Rendered text: '{}' {} chars at {}x{} hq={}\n", _text.substr(0, 20), _text.size(), surface->w, surface->h, _hqOpaque ? 1 : 0);
		}
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

const Font* FontManager::loadFont(const std::string& path, int pointSize)
{
	for (Font* f : _fonts) {
		if (f->path == path && f->pointSize == pointSize) {
			return f;
		}
	}

	Font* font = new Font();
	font->pointSize = pointSize;
	font->realSize = pointSize * kScale;
	font->path = path;
	font->font = TTF_OpenFont(path.c_str(), pointSize * kScale);
	assert(font->font);	
	_fonts.push_back(font);

	PLOG(plog::info) << "Loaded font '" << path << "' at size " << pointSize;
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
		int realSize = xf.s(f->pointSize);
		if (realSize != f->realSize) {
			// Flush the open tasks:
			_pool.WaitforAll();
			change = true;

#if DEBUG_TEXT
			fmt::print("Loading font {} at size {}...", f->path, realSize);
#endif

			//int points = realSize * 133 / 100;
			TTF_SetFontSize(f->font, realSize * kScale);

#if DEBUG_TEXT
#	if DEBUG_TEXT_SAVE
			SDL_Surface* ts = TTF_RenderUTF8_LCD_Wrapped(f->font, "The quick brown fox jumped over the lazy dog", { 255, 255, 255, 255 }, { 0, 0, 0, 255 }, 512);
			SDL_SaveBMP(ts, "font.bmp");
			SDL_FreeSurface(ts);
#	endif
			fmt::print("done\n");
#endif
			assert(f->font);
			f->realSize = realSize;
		}
	}

	// Stage 2: update the text fields to the correct size
	for (auto& tf : _textFields) {
		// The texture needs to be kept 1:1 with the real size, so text doesn't look fuzzy.
		Size realSize = xf.t(tf->_virtualSize);
		realSize.w *= kScale;
		realSize.h *= kScale;
		Size texSize = tf->_texture ? tf->_texture->pixelSize() : Size{ 0, 0};

		if (!tf->_texture || realSize != texSize) {
			tf->_texture = _textureManager.createTextField(realSize.w, realSize.h);
			tf->_needUpdate = true;
		}
		tf->_needUpdate |= change;
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


std::shared_ptr<TextField> FontManager::createTextField(const Font* font, int width, int height, bool useOpaqueHQ)
{
	Font* f = getFont(font);
	assert(f);
	if (!f) return nullptr;

	TextField* tf = new TextField();
	tf->_font = f;
	tf->_virtualSize = Size{ width, height };
	// No point in creating the texture because we don't know the real size yet.
	//tf->_texture = _textureManager.createTextField(width, height);
	tf->_hqOpaque = useOpaqueHQ;

	std::shared_ptr<TextField> ptr(tf);
	_textFields.push_back(ptr);
	return ptr;	
}

TextField::~TextField()
{
}

void FontManager::Draw(std::shared_ptr<TextField>& tf, int x, int y)
{
	assert(tf->_font);	
	assert(tf->_font->font); // should be kept up to date in update()

	if (tf->_texture->ready()) {
		//Point p = _xf.t(Point{ vx, vy });
		SDL_Rect dst = { x, y, tf->_texture->width() / kScale, tf->_texture->height() / kScale };
		if (tf->_hqOpaque)
			SDL_SetTextureBlendMode(tf->_texture->sdlTexture(), SDL_BLENDMODE_NONE);
		else
			SDL_SetTextureBlendMode(tf->_texture->sdlTexture(), SDL_BLENDMODE_BLEND);

		SDL_SetTextureScaleMode(tf->_texture->sdlTexture(), SDL_ScaleMode::SDL_ScaleModeNearest);
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

#include "text.h"
#include "texture.h"
#include "xform.h"
#include "../task.h"

#include <SDL.h>

#include <fmt/core.h>
#include <plog/Log.h>

#define DEBUG_TEXT 0
#define DEBUG_TEXT_SAVE 0	// set to 1 to write "surface.bmp" and "font.bmp" to disk to compare pixel quality
#define DEBUG_MOUSE 0

static int gQuality = 1;

class RenderFontTask : public lurp::SelfDeletingTask
{
public:
	virtual ~RenderFontTask() {}

	virtual void ExecuteRange(enki::TaskSetPartition /*range_*/, uint32_t /*threadnum_*/) override 
	{
		TextureUpdate update{ _texture, nullptr, _generation, _hqOpaque, _bg };

		{
			// TTF, it turns out, is not thread safe.
			// I wish it was - there really should be a context object.
			// In any case, do all the texture generation all at once for this object, in case we hop threads.
			static std::mutex gTTFMutex;
			std::lock_guard<std::mutex> lock(gTTFMutex);

			for (size_t i = 0; i < _row.size(); i++) {
				SDL_Surface* surface = nullptr;
				if (!_row[i].text.empty()) {
					const Font* font = _row[i].font;
					const std::string& text = _row[i].text;
					SDL_Color color = _row[i].color;

					if (_hqOpaque) {
						if (gQuality == 0)
							surface = TTF_RenderUTF8_LCD_Wrapped(font->font, text.c_str(), color, _bg, _texture->pixelSize().w);
						else
							surface = TTF_RenderUTF8_Shaded_Wrapped(font->font, text.c_str(), color, _bg, _texture->pixelSize().w);
					}
					else {
						surface = TTF_RenderUTF8_Blended_Wrapped(font->font, text.c_str(), color, _texture->pixelSize().w);
					}
					assert(surface);
				}
				update.surfaceVec.push_back(surface);
			}
		}

#if DEBUG_TEXT
		if (update.surfaceVec[0]) {
#	if	DEBUG_TEXT_SAVE
			SDL_SaveBMP(update.surfaceVec[0], "surface.bmp");
#	endif
			fmt::print("Rendered text: '{}' {} chars at {}x{} hq={}\n", _text[0].substr(0, 20), _text[0].size(), update.surfaceVec[0]->w, update.surfaceVec[0]->h, _hqOpaque ? 1 : 0);
		}
#endif
		_queue->push(update);
	}

	std::shared_ptr<Texture> _texture;
	TextureLoadQueue* _queue = nullptr;
	int _generation = 0;

	std::vector<TextBox::Row> _row;
	SDL_Color _bg;
	bool _hqOpaque = false;
};

TextBox::TextBox()
{
}


TextBox::~TextBox()
{
}

void TextBox::resize(size_t s)
{
	if (s < _row.size()) {
		_row.resize(s);
		_needUpdate = true;
	}
	else if (s > _row.size()) {
		assert(_font0);
		_row.resize(s, { _font0, "", { 255, 255, 255, 255 }, 0 });
	}
}

void TextBox::setFont(size_t i, const Font* font) {
	if (font != _row[i].font) {
		_row[i].font = font;
		_needUpdate = true;
		if (i == 0)
			_font0 = font;
	}
}

void TextBox::setText(size_t i, const std::string& text) {
	if (text != _row[i].text) {
		_row[i].text = text;
		_needUpdate = true;
	}
}

void TextBox::setColor(size_t i, SDL_Color color) {
	if (!ColorEqual(color, _row[i].color)) {
		_row[i].color = color;
		_needUpdate = true;
	}
}

void TextBox::setSpace(size_t i, int space) {
	if (_row[i].virtualSpace != space) {
		_row[i].virtualSpace = space;
		_needUpdate = true;
	}
}

void TextBox::setBgColor(SDL_Color color) {
	if (!ColorEqual(color, _bg)) {
		_bg = color;
		_needUpdate = true;
	}
}

bool TextBox::hitTest(const Point& screen) const
{
	if (!_texture->ready())
		return false;

	Rect rect = { pos.x, pos.y, _texture->surfaceSize().w, _texture->surfaceSize().h };
	if (rect.contains(screen))
		return true;
	return false;
}

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
	font->realSize = pointSize;
	font->path = path;
	font->font = TTF_OpenFont(path.c_str(), pointSize);
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
			TTF_SetFontSize(f->font, realSize);

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

			task->_row = tf->_row;	// copy all the data

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


std::shared_ptr<TextBox> FontManager::createTextBox(const Font* font, int width, int height, bool useOpaqueHQ)
{
	Font* f = getFont(font);
	assert(f);
	if (!f) return nullptr;

	TextBox* tf = new TextBox();
	tf->_font0 = f;
	tf->_virtualSize = Size{ width, height };
	// No point in creating the texture because we don't know the real size yet.
	//tf->_texture = _textureManager.createTextField(width, height);
	tf->_hqOpaque = useOpaqueHQ;

	tf->resize(1);

	std::shared_ptr<TextBox> ptr(tf);
	_textFields.push_back(ptr);
	return ptr;	
}

void FontManager::Draw(const std::shared_ptr<TextBox>& tf) const
{
	if (tf->_texture->ready()) {
		const Texture* tex = tf->_texture.get();

		// Do a little bit of smart clipping so we are filling pixels that aren't used.
		SDL_Rect src = {0, 0, tex->_surfaceSize.w, tex->_surfaceSize.h};
		SDL_Rect dst = { tf->pos.x, tf->pos.y, tex->_surfaceSize.w, tex->_surfaceSize.h };

		if (tf->_hqOpaque)
			SDL_SetTextureBlendMode(tf->_texture->sdlTexture(), SDL_BLENDMODE_NONE);
		else
			SDL_SetTextureBlendMode(tf->_texture->sdlTexture(), SDL_BLENDMODE_BLEND);

		SDL_SetTextureScaleMode(tf->_texture->sdlTexture(), SDL_ScaleMode::SDL_ScaleModeNearest);
		SDL_RenderCopy(_renderer, tf->_texture->sdlTexture(), &src, &dst);
	}
}

void FontManager::Draw(const VBox& vbox, const Point& p) const
{
	int y = p.y;
	for (size_t i = 0; i < vbox.boxes.size(); i++) {
		static constexpr SDL_Color white{ 255, 255, 255, 255 };

		const std::shared_ptr<TextBox>& tf = vbox.boxes[i];
		MouseState state = tf->mouseState();

		switch (state) {
		case MouseState::none:
			SDL_SetTextureColorMod(tf->_texture->sdlTexture(), vbox._upColor.r, vbox._upColor.g, vbox._upColor.b);
			break;
		case MouseState::over:
			SDL_SetTextureColorMod(tf->_texture->sdlTexture(), 255, 255, 255);
			break;
		case MouseState::down:
			SDL_SetTextureColorMod(tf->_texture->sdlTexture(), 192, 192, 192);
		}
		tf->pos = Point{ p.x, y };
		Draw(tf);
		y += tf->surfaceSize().h;
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

void FontManager::doMove(const Point& screen, const Point&)
{
	for (auto& tf : _textFields) {
		tf->_mouseState = MouseState::none;
		if (!tf->_interactive)
			continue;
		if (tf->hitTest(screen)) {
#			if DEBUG_MOUSE
			fmt::print("Mouse over: {}\n", tf->_text[0]);
#			endif 

			if (_mouseBox == tf)
				tf->_mouseState = MouseState::none;
			else if (!_mouseBox)
				tf->_mouseState = MouseState::over;
		}
	}
}

std::shared_ptr<TextBox> FontManager::doButton(const Point& screen, const Point&, bool down)
{
	std::shared_ptr<TextBox> clicked;
	for (auto& tf : _textFields) {
		if (!tf->_interactive)
			continue;
		if (tf->hitTest(screen)) {
			if (down) {
				_mouseBox = tf;
#			if DEBUG_MOUSE
				fmt::print("Captured: {}\n", tf->_text[0]);
#			endif 
			}
			else {
				if (_mouseBox == tf) {
					fmt::print("Clicked: {}\n", tf->_row[0].text);
					clicked = tf;
#			if DEBUG_MOUSE
					fmt::print("Clicked: {}\n", tf->_text[0]);
#			endif 
				}
			}
		}
	}
	if (!down)
		_mouseBox = nullptr;
	doMove(screen, Point());
	return clicked;
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

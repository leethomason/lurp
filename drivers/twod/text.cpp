#include "text.h"
#include "texture.h"
#include "xform.h"
#include "../task.h"

#include <SDL.h>

#include <fmt/core.h>

class RenderFontTask : public lurp::SelfDeletingTask
{
public:
	virtual ~RenderFontTask() {}

	virtual void ExecuteRange(enki::TaskSetPartition /*range_*/, uint32_t /*threadnum_*/) override 
	{
		SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(_font, _text.c_str(), _color, _texture->width());

		// Much higher quality, but not transparent
		// I experimented with converting to an alpha map, but it looks no better than the blended version
		//SDL_Surface* surface = TTF_RenderUTF8_LCD_Wrapped(_font, _text.c_str(), _color, SDL_Color{0, 0, 0, 0}, _texture->width());
		assert(surface);

		fmt::print("Rendered text: {} chars at {}x{}\n", _text.size(), surface->w, surface->h);

		TextureUpdate update{ _texture, surface, _generation };
		_queue->push(update);
	}

	std::shared_ptr<Texture> _texture;
	TextureLoadQueue* _queue = nullptr;
	int _generation = 0;

	TTF_Font* _font = nullptr;
	std::string _text;
	SDL_Color _color;
};

FontManager::~FontManager()
{
	for (Font* f : _fonts) {
		TTF_CloseFont(f->font);
		delete f;
	}
}

void FontManager::loadFont(const std::string& name, const std::string& path, int size)
{
	Font* font = new Font();
	font->size = size;
	font->name = name;
	font->path = path;
	_fonts.push_back(font);
}

void FontManager::update(const XFormer& xf)
{
	// Fonts are kept current on the main thread.
	// This also means no tasks can be in flight when the fonts are changed out.
	// So changing fonts is an expensive & blocking operation.
	// Keep the number of fonts low!

	_xf = xf;
	bool change = false;

	// Update all the fonts.
	for (Font* f : _fonts) {
		int realSize = xf.s(f->size);
		if (realSize != f->realSize) {
			// Flush the open tasks:
			_pool.WaitforAll();
			change = true;

			// Now it is safe to change the font size
			if (f->font)
				TTF_CloseFont(f->font);
			f->font = TTF_OpenFont(f->path.c_str(), realSize);
			assert(f->font);
			f->realSize = realSize;
		}
	}

	// And new the text fields attached to the fonts. 
	// Remember we already flushed the open tasks.
	if (change) {
		for (auto& tf : _textFields) {
			tf->_text = "<--cache invalid-->";

			// This is super exciting! The texture size may change as well. (this is so complex!!)
			Point size = xf.s(Point{ tf->_width, tf->_height });
			if (size.x != tf->_texture->width() || size.y != tf->_texture->height()) {
				tf->_texture = _textureManager.createTextField(tf->_name, size.x, size.y);
			}			
		}
	}
}

Font* FontManager::getFont(const std::string& name)
{
	for (Font* f : _fonts) {
		if (f->name == name) {
			return f;
		}
	}
	return nullptr;
}


std::shared_ptr<TextField> FontManager::createTextField(const std::string& name, const std::string& font, int width, int height)
{
	Font* f = getFont(font);
	assert(f);
	if (!f) return nullptr;

	TextField* tf = new TextField();
	tf->_name = name;
	tf->_font = f;
	tf->_width = width;
	tf->_height = height;
	tf->_fontManager = this;
	tf->_texture = _textureManager.createTextField(name, width, height);

	std::shared_ptr<TextField> ptr(tf);
	_textFields.push_back(ptr);
	return ptr;	
}

TextField::~TextField()
{
}

void TextField::Render(const std::string& text, int x, int y, SDL_Color color)
{
	_fontManager->renderTextField(this, text, x, y, color);
}

void FontManager::renderTextField(const TextField* tf, const std::string& text, int vx, int vy, SDL_Color color)
{
	assert(tf->_font);	
	assert(tf->_font->font); // should be kept up to date in update()

	if (tf->_text != text || !ColorEqual(tf->_color, color))  {
		RenderFontTask* task = new RenderFontTask();
		task->_texture = tf->_texture;
		task->_queue = &_textureManager._loadQueue;
		task->_generation = ++_textureManager._generation;

		task->_font = tf->_font->font;
		task->_color = color;
		task->_text = text;

		tf->_text = text;
		tf->_color = color;

		_textureManager._pool.AddTaskSetToPipe(task);
	}
	if (tf->_texture->ready()) {
		Point p = _xf.t(Point{ vx, vy });
		SDL_Rect dst = { p.x, p.y, tf->_texture->width(), tf->_texture->height() };
		SDL_SetTextureBlendMode(tf->_texture->sdlTexture(), SDL_BLENDMODE_BLEND);
		SDL_RenderCopy(_renderer, tf->_texture->sdlTexture(), nullptr, &dst);
	}
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
		SDL_RenderCopy(renderer, tex->sdlTexture(), &src, &xDst);
	};
}
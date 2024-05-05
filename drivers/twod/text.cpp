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

	Texture* _texture = nullptr;
	TextureLoadQueue* _queue = nullptr;
	int _generation = 0;

	TTF_Font* _font = nullptr;
	std::string _text;
	SDL_Color _color;
};

FontManager::~FontManager()
{
	for (Font& f : _fonts) {
		TTF_CloseFont(f.font);
	}
}

void FontManager::loadFont(const std::string& name, const std::string& path, int size)
{
	Font font;
	font.font = TTF_OpenFont(path.c_str(), size);
	assert(font.font);
	font.size = size;
	font.name = name;
	_fonts.push_back(font);
}

void FontManager::drawText(const std::string& name, Texture* textField, const std::string& text, SDL_Color color)
{
	assert(textField->_textField);
	Font* font = nullptr;
	for (Font& f : _fonts) {
		if (f.name == name) {
			font = &f;
			break;
		}
	}
	assert(font);
	if (!font) return;

	RenderFontTask* task = new RenderFontTask();
	task->_texture = textField;
	task->_queue = &_textureManager._loadQueue;
	task->_generation = ++_textureManager._generation;

	task->_font = font->font;
	task->_color = color;
	task->_text = text;

	_textureManager._pool.AddTaskSetToPipe(task);
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
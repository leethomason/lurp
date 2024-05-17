#pragma once

#include "texture.h"
#include "xform.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <string>
#include <vector>

struct SDL_Renderer;
struct SDL_Texture;
class Texture;
class XFormer;
class FontManager;

struct Font {
	TTF_Font* font = nullptr;	// this is kept current on the main thread and is always valid (the pool will be flushed before this changes)
	int size = 0;				// virtual size
	int realSize = 0;			// render size
	std::string path;			// for reload, which has to be done on resize (keeping fonts 1:1)
};

class TextField {
	friend class FontManager;

public:
	TextField() {}
	~TextField();

	void setText(const std::string& text) {
		if (text != _text) {
			_text = text;
			_needUpdate = true;
		}
	}
	void setColor(SDL_Color color) {
		if (color.r != _color.r || color.g != _color.g || color.b != _color.b || color.a != _color.a) {
			_color = color;
			_needUpdate = true;
		}
	}
	void setBgColor(SDL_Color color) {
		if (color.r != _bg.r || color.g != _bg.g || color.b != _bg.b || color.a != _bg.a) {
			_bg = color;
			_needUpdate = true;
		}
	}

	Size virtualSize() const {
		return _virtualSize;
	}

	Size renderedSize() const {
		return _renderedSize; 
	}
	SDL_Color color() const { return _color; }
	SDL_Color bgColor() const { return _bg; }
	const std::string& text() const { return _text; }

private:
	void update() {
		_needUpdate = true;
		// Not sure about this. Need to think through when the renderedSize
		// can be correct and incorrect.
		//_renderedSize = Point{ 0, 0 };
	}

	bool _needUpdate = false;

	std::shared_ptr<Texture> _texture;
	const Font* _font = nullptr;
	Size _virtualSize;
	Size _renderedSize;	// size of the rendered text; usually a little smaller than _size

	bool _hqOpaque = false;
	SDL_Color _bg = SDL_Color{ 0, 0, 0, 0 };
	std::string _text;
	SDL_Color _color = SDL_Color{ 0, 0, 0, 0 };
};

class FontManager {
public:
	FontManager(SDL_Renderer* renderer, enki::TaskScheduler& pool, TextureManager& textureManager, int virtualW, int virtualH) : _renderer(renderer), _pool(pool), _textureManager(textureManager), _xf(virtualW, virtualH) {}
	~FontManager();

	// Fast to get an existing font.
	// New fonts or resizing is expensive - keep the font count low.
	// Note that a 'const Font*' is returned. The font is never destroyed, and owned by the manager.
	const Font* loadFont(const std::string& path, int virtualSize);

	void update(const XFormer& xf);

	std::shared_ptr<TextField> createTextField(const Font*, int width, int height, bool useOpaqueHQ = false, SDL_Color bg = SDL_Color{0, 0, 0, 255});

	// Note it draws in real pixels (like ::Draw)
	void Draw(std::shared_ptr<TextField>& tf, int x, int y);

	void toggleQuality();


private:
	Font* getFont(const Font*);

	SDL_Renderer* _renderer;
	enki::TaskScheduler& _pool;
	TextureManager& _textureManager;
	std::vector<Font*> _fonts;
	std::vector<std::shared_ptr<TextField>> _textFields;
	XFormer _xf;
};

void DrawDebugText(const std::string& text, SDL_Renderer* renderer, const Texture* tex, int x, int y, int fontSize, const XFormer& xf);
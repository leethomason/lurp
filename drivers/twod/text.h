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
	int pointSize = 0;
	int realSize = 0;
	std::string path;			// for reload, which has to be done on resize (keeping fonts 1:1)
};

class TextBox {
	friend class FontManager;

public:
	TextBox() {}
	~TextBox();

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

	bool _hqOpaque = false;
	SDL_Color _bg = SDL_Color{ 0, 0, 0, 255 };
	std::string _text;
	SDL_Color _color = SDL_Color{ 255, 255, 255, 255 };
};

class VBox {
	friend class FontManager;
public:
	VBox() = default;
	~VBox() = default;

	size_t size() const { return _textBoxes.size(); }
	void add(const Font*);
	void clear();

	void setText(int i, const std::string& text);
	void setColor(int i, SDL_Color color);
	void setBgColor(SDL_Color color);

private:
	FontManager* _fontManager = nullptr;
	Size _virtualSize;
	bool _opaqueHQ = false;
	std::vector<std::shared_ptr<TextBox>> _textBoxes;
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

	std::shared_ptr<TextBox> createTextBox(const Font*, int vWidth, int vHeight, bool useOpaqueHQ);
	std::shared_ptr<VBox> createVBox(int vWidth, int maxVHeightOfBox, bool useOpaqueHQ);

	// Note it draws in real pixels (like ::Draw)
	void Draw(const std::shared_ptr<TextBox>& tf, int x, int y) const;
	void Draw(const std::shared_ptr<TextBox>& tf, const Point& p) const { Draw(tf, p.x, p.y); }

	void Draw(const std::shared_ptr<VBox>& vbox, int x, int y) const;
	void Draw(const std::shared_ptr<VBox>& vbox, const Point& p) const { Draw(vbox, p.x, p.y); }

	void toggleQuality();


private:
	Font* getFont(const Font*);

	SDL_Renderer* _renderer;
	enki::TaskScheduler& _pool;
	TextureManager& _textureManager;
	std::vector<Font*> _fonts;
	std::vector<std::shared_ptr<TextBox>> _textFields;
	std::vector<std::shared_ptr<VBox>> _vBoxes;
	XFormer _xf;
};

void DrawDebugText(const std::string& text, SDL_Renderer* renderer, const Texture* tex, int x, int y, int fontSize, const XFormer& xf);
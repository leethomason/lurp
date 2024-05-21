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
	TextBox();
	~TextBox();

	size_t size() const { return _text.size(); }
	void resize(size_t s);

	void setFont(const Font* font) { setFont(0, font);  }
	void setText(const std::string& text) { setText(0, text); }
	void setColor(SDL_Color color) { setColor(0, color); }
	void setBgColor(SDL_Color color);

	void setFont(size_t i, const Font* font);
	void setText(size_t i, const std::string& text);
	void setColor(size_t i, SDL_Color color);
	void setBgColor(size_t i, SDL_Color color);

	const Font* font(size_t i = 0) const { return _font[i]; }
	const std::string& text(size_t i = 0) const { return _text[i]; }
	SDL_Color color(size_t i = 0) const { return _color[i]; }
	SDL_Color bgColor() const { return _bg; }

	Size virtualSize() const { return _virtualSize;	}

private:
	bool _needUpdate = false;

	std::shared_ptr<Texture> _texture;
	Size _virtualSize;

	bool _hqOpaque = false;
	SDL_Color _bg = SDL_Color{ 0, 0, 0, 255 };

	std::vector<const Font*> _font;
	std::vector<std::string> _text;
	std::vector<SDL_Color> _color;
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

	// Note it draws in real pixels (like ::Draw)
	void Draw(const std::shared_ptr<TextBox>& tf, int x, int y) const;
	void Draw(const std::shared_ptr<TextBox>& tf, const Point& p) const { Draw(tf, p.x, p.y); }

	void toggleQuality();


private:
	Font* getFont(const Font*);

	SDL_Renderer* _renderer;
	enki::TaskScheduler& _pool;
	TextureManager& _textureManager;
	std::vector<Font*> _fonts;
	std::vector<std::shared_ptr<TextBox>> _textFields;
	XFormer _xf;
};

void DrawDebugText(const std::string& text, SDL_Renderer* renderer, const Texture* tex, int x, int y, int fontSize, const XFormer& xf);
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
	//std::string name;
	std::string path;
};

class TextField {
	friend class FontManager;
public:
	TextField() {}
	~TextField();

	const std::string& name() const { return _name; }

	void Render(const std::string& text, int x, int y, SDL_Color color);

private:
	std::string _name;	// info / debugging only
	std::shared_ptr<Texture> _texture;
	const Font* _font = nullptr;
	int _width = 0;
	int _height = 0;
	FontManager* _fontManager = nullptr;
	bool _hqOpaque = false;
	SDL_Color _bg = SDL_Color{ 0, 0, 0, 0 };

	// No good solutions here. But this is used as a cache invalid flag.
	// FIXME: replacing with a hash would make this clearer.
	mutable std::string _text;
	mutable SDL_Color _color = SDL_Color{ 0, 0, 0, 0 };
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

	// Do not call directly. Use TextField::Render()
	void renderTextField(const TextField* tf, const std::string& text, int x, int y, SDL_Color color);

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
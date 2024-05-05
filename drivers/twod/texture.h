#pragma once

#include "TaskScheduler.h"
#include "util.h"
#include "textureupdate.h"

#include <SDL.h>

#include <string>


struct Color {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 0;

	uint32_t u32() const {
		return *((uint32_t*)this);
	}
};

class Texture {
	friend class TextureManager;
	friend class FontManager;
	friend class TextureLoadTask;
public:
	Texture() {}
	~Texture();

	bool ready() const { return _sdlTexture != nullptr; }
	SDL_Texture* sdlTexture() const { return _sdlTexture; }
	int width() const { return _w; }
	int height() const { return _h; }

	static bool ready(std::vector<const Texture*> textures) {
		return std::all_of(textures.begin(), textures.end(), [](const Texture* t) { return t->ready(); });
	}

private:
	SDL_Texture* _sdlTexture = nullptr;
	std::string _name;
	std::string _path;
	int _generation = 0;
	int _w = 0;
	int _h = 0;
	int _bytes = 0;	// 3 or 4
	bool _textField = false;
};

class TextureManager
{
	friend class FontManager;
public:
	TextureManager(enki::TaskScheduler& pool, SDL_Renderer* renderer);
	~TextureManager();

	const Texture* loadTexture(const std::string& name, const std::string& path);
	const Texture* getTexture(const std::string& name) const;
	Texture* createTextField(const std::string& name, int w, int h);

	void update();
	void freeAll();

	int numTextures() const { return (int)_textures.size(); }
	int numTexturesReady() const {
		return (int)std::count_if(_textures.begin(), _textures.end(), [](const Texture* t) { return t->ready(); });
	}

private:
	int _generation = 0;
	SDL_Renderer* _sdlRenderer;
	enki::TaskScheduler& _pool;
	TextureLoadQueue _loadQueue;
	std::vector<Texture*> _textures;
};

void DrawTestPattern(SDL_Renderer* renderer, int w, int h, int size, Color c1, Color c2);

struct PreserveColor {
	PreserveColor(SDL_Renderer* renderer) : _renderer(renderer) {
		SDL_GetRenderDrawColor(_renderer, &r, &g, &b, &a);
	}

	~PreserveColor() {
		SDL_SetRenderDrawColor(_renderer, r, g, b, a);
	}

	SDL_Renderer* _renderer;
	Uint8 r, g, b, a;
};

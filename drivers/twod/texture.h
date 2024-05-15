#pragma once

#include "TaskScheduler.h"
#include "util.h"
#include "textureupdate.h"
#include "xform.h"

#include <SDL.h>

#include <string>
#include <algorithm>
#include <numeric>
#include <filesystem>

class XFormer;
class TextureManager;

inline bool ColorEqual(const SDL_Color& c1, const SDL_Color& c2) {
	return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b && c1.a == c2.a;
}

class Texture {
	friend class TextureManager;
	friend class FontManager;
	friend class TextureLoadTask;
public:
	Texture() {}
	~Texture();

	bool ready() const { return _sdlTexture != nullptr; }
	SDL_Texture* sdlTexture() const { _age = 0; return _sdlTexture; }
	int width() const { return _size.x; }
	int height() const { return _size.y; }
	const Point& size() const { return _size; }
	const Point& surfaceSize() const { return _surfaceSize; }	
	const std::string& path() const { return _path; }

	static bool ready(std::vector<const Texture*> textures) {
		return std::all_of(textures.begin(), textures.end(), [](const Texture* t) { return t->ready(); });
	}
	static bool ready(std::vector<std::shared_ptr<Texture>> textures) {
		return std::all_of(textures.begin(), textures.end(), [](const auto& t) { return t->ready(); });
	}

private:
	SDL_Texture* _sdlTexture = nullptr;
	std::string _path;
	int _generation = 0;
	Point _size;	// allocated size, and size of the texture
	Point _surfaceSize;	// size of the surface it was created from (smaller for text surfaces)
	int _bytes = 0;	// 3 or 4
	bool _textField = false;
	mutable uint32_t _age = 0;
};

class TextureManager
{
	friend class FontManager;
public:
	static constexpr uint32_t kMaxAge = 60;

	TextureManager(enki::TaskScheduler& pool, SDL_Renderer* renderer);
	~TextureManager();

	// Loads a texture, or returns the existing one if it's already loaded.
	//std::shared_ptr<Texture> loadTexture(const std::string& path);
	std::shared_ptr<Texture> loadTexture(const std::filesystem::path& path);

	// The TextureManager can throw textures away at will, so getTexture() isn't very useful.
	// Use loadTexture() instead, which can re-load the texture if it's been thrown away.
	// std::shared_ptr<Texture> getTexture(const std::string& name) const;
	std::shared_ptr<Texture> createTextField(int w, int h);

	void update();
	void freeAll();

	int numTextures() const { return (int)_textures.size(); }
	int numTexturesReady() const {
		return (int)std::count_if(_textures.begin(), _textures.end(), [](const auto& t) { return t->ready(); });
	}

	uint64_t totalTextureMemory() const {
		return std::accumulate(_textures.begin(), _textures.end(), 0, [](int sum, const auto& t) {
			return sum + (t->width() * t->height() * t->_bytes);
		});
	}
	uint64_t readyTextureMemory() const {
		return std::accumulate(_textures.begin(), _textures.end(), 0, [](int sum, const auto& t) {
			return sum + (t->ready() ? (t->width() * t->height() * t->_bytes) : 0);
		});
	}

private:
	int _generation = 0;
	SDL_Renderer* _sdlRenderer;
	enki::TaskScheduler& _pool;
	TextureLoadQueue _loadQueue;
	std::vector<std::shared_ptr<Texture>> _textures;
};

void DrawTestPattern(SDL_Renderer* renderer, int w, int h, int size, SDL_Color c1, SDL_Color c2, const XFormer& xf);

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

enum class RenderQuality {
	kFullscreen,
	kBlit,
	kText,

	kNearest,
	kLinear,
	kBest
};

void Draw(SDL_Renderer* renderer, 
	std::shared_ptr<Texture> texture, 
	const SDL_Rect* src, const SDL_Rect* dst, 
	RenderQuality quality,
	double alpha = 1.0);

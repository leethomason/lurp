#pragma once

#include "TaskScheduler.h"
#include "util.h"

#include <SDL.h>

#include <string>


class Texture : enki::ITaskSet {
	friend class TextureManager;
public:
	Texture() {}
	~Texture();

	bool ready() const { return sdlTexture != nullptr; }

	void ExecuteRange(enki::TaskSetPartition range_, uint32_t threadnum_) override;

	SDL_Texture* getSDLTexture() const { return sdlTexture; }

private:
	SDL_Texture* sdlTexture = nullptr;
	SDL_Surface* sdlSurface = nullptr;
	lurp::QueueMT<Texture*>* queue = nullptr;

	std::string name;
	std::string path;
	int w = 0;
	int h = 0;
	int bytes = 0;	// 3 or 4
};

class TextureManager
{
public:
	TextureManager(enki::TaskScheduler& pool, SDL_Renderer* renderer) : _pool(pool), _sdlRenderer(renderer) {}
	~TextureManager();

	const Texture* loadTexture(const std::string& name, const std::string& path);
	const Texture* getTexture(const std::string& name) const;
	void update();
	void freeAll();

private:
	enki::TaskScheduler& _pool;
	SDL_Renderer* _sdlRenderer;
	lurp::QueueMT<Texture*> _loadQueue;
	std::vector<Texture*> _textures;
};


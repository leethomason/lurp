#include "texture.h"

#include <SDL_image.h>

Texture::~Texture()
{
	assert(ready());
	assert(sdlSurface == nullptr);
	assert(sdlTexture);
	SDL_DestroyTexture(sdlTexture);	
}


void Texture::ExecuteRange(enki::TaskSetPartition, uint32_t)
{
	sdlSurface = IMG_Load(path.c_str());
	assert(sdlSurface);
	assert(queue);
	this->w = sdlSurface->w;
	this->h = sdlSurface->h;
	this->bytes = sdlSurface->format->BytesPerPixel;
	assert(this->bytes == 3 || this->bytes == 4);	

	queue->push(this);
}

TextureManager::~TextureManager()
{
	freeAll();
}

const Texture* TextureManager::loadTexture(const std::string& name, const std::string& path)
{
	Texture* texture = new Texture();
	texture->queue = &_loadQueue;
	texture->name = name;
	texture->path = path;
	_pool.AddTaskSetToPipe(texture);
	return texture;
}

const Texture* TextureManager::getTexture(const std::string& name) const
{
	for (Texture* t : _textures) {
		if (t->name == name) {
			return t;
		}
	}
	return nullptr;
}

void TextureManager::update()
{
	Texture* texture = nullptr;
	while(_loadQueue.tryPop(texture))
	{
		texture->sdlTexture = SDL_CreateTextureFromSurface(_sdlRenderer, texture->sdlSurface);
		SDL_FreeSurface(texture->sdlSurface);
		texture->sdlSurface = nullptr;
	}
}

void TextureManager::freeAll()
{
	_pool.WaitforAll();
	update();
	for (Texture* t : _textures) {
		assert(t->ready());
		delete t;
	}
	_textures.clear();
}

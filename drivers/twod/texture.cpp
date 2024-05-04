#include "texture.h"

#include <SDL_image.h>

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

Texture::~Texture()
{
	assert(ready());
	assert(_sdlSurface == nullptr);
	assert(_sdlTexture);
	SDL_DestroyTexture(_sdlTexture);	
}


void Texture::ExecuteRange(enki::TaskSetPartition, uint32_t)
{
	assert(!_sdlSurface);
	_sdlSurface = IMG_Load(_path.c_str());
	assert(_sdlSurface);
	assert(_queue);
	this->_w = _sdlSurface->w;
	this->_h = _sdlSurface->h;
	this->_bytes = _sdlSurface->format->BytesPerPixel;
	assert(this->_bytes == 3 || this->_bytes == 4);	

	_queue->push(this);
}

TextureManager::~TextureManager()
{
	freeAll();
}

const Texture* TextureManager::loadTexture(const std::string& name, const std::string& path)
{
	Texture* texture = new Texture();
	texture->_queue = &_loadQueue;
	texture->_name = name;
	texture->_path = path;
	_pool.AddTaskSetToPipe(texture);
	_textures.push_back(texture);
	return texture;
}

TextureManager::TextureManager(enki::TaskScheduler& pool, SDL_Renderer* renderer)
	: _pool(pool), _sdlRenderer(renderer)
{
}

const Texture* TextureManager::getTexture(const std::string& name) const
{
	for (Texture* t : _textures) {
		if (t->_name == name) {
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
		texture->_sdlTexture = SDL_CreateTextureFromSurface(_sdlRenderer, texture->_sdlSurface);
		SDL_FreeSurface(texture->_sdlSurface);
		texture->_sdlSurface = nullptr;
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

void DrawTestPattern(SDL_Renderer* renderer, int w, int h, int size, Color c1, Color c2)
{
	PreserveColor pc(renderer);

	int ni = w / size;
	int nj = h / size;
	for (int j = 0; j < nj; j++) {
		for(int i=0; i<ni; i++) {
			Color c = (i+j) % 2 == 0 ? c1 : c2;
			SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
			SDL_Rect rect = {i*size, j*size, size, size};
			SDL_RenderFillRect(renderer, &rect);
		}
	}
}

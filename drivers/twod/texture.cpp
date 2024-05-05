#include "texture.h"
#include "../task.h"

#include <SDL_image.h>

class TextureLoadTask : public lurp::SelfDeletingTask
{
public:
	virtual ~TextureLoadTask() {}

	virtual void ExecuteRange(enki::TaskSetPartition /*range_*/, uint32_t /*threadnum_*/) override {
		assert(!_texture->_textField);
		SDL_Surface* surface = IMG_Load(_texture->_path.c_str());
		assert(surface);
		assert(_queue);
		_texture->_w = surface->w;
		_texture->_h = surface->h;
		_texture->_bytes = surface->format->BytesPerPixel;
		assert(_texture->_bytes == 3 || _texture->_bytes == 4);

		TextureUpdate update{ _texture, surface, _generation };
		_queue->push(update);
	}

	Texture* _texture = nullptr;
	TextureLoadQueue* _queue = nullptr;
	int _generation = 0;
};

Texture::~Texture()
{
	SDL_DestroyTexture(_sdlTexture);
}

TextureManager::~TextureManager()
{
	freeAll();
}

const Texture* TextureManager::loadTexture(const std::string& name, const std::string& path)
{
	Texture* texture = new Texture();
	texture->_name = name;
	texture->_path = path;

	TextureLoadTask* task = new TextureLoadTask();
	task->_texture = texture;
	task->_queue = &_loadQueue;
	task->_generation = ++_generation;

	_pool.AddTaskSetToPipe(task);
	_textures.push_back(texture);
	return texture;
}

Texture* TextureManager::createTextField(const std::string& name, int w, int h)
{
	Texture* texture = new Texture();
	texture->_name = name;
	texture->_w = w;
	texture->_h = h;
	texture->_bytes = 4;
	texture->_textField = true;
	texture->_sdlTexture = SDL_CreateTexture(_sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);

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
	TextureUpdate update;
	while(_loadQueue.tryPop(update))
	{
		Texture* texture = update.texture;
		SDL_Surface* surface = update.surface;
		int generation = update.generation;

		if (generation < texture->_generation) {
			// This is an old surface. Discard it.
			SDL_FreeSurface(surface);
			continue;
		}

		if (texture->_textField) {
			assert(texture->_sdlTexture);	// fixed size - should always be there.
			SDL_Surface* target = nullptr;
			// Do not free 'target`: done be the unlock
			int err = SDL_LockTextureToSurface(texture->_sdlTexture, nullptr, &target);
			assert(err == 0);
			assert(target);
			SDL_BlitSurface(surface, nullptr, target, nullptr);
			SDL_UnlockTexture(texture->_sdlTexture);
		}
		else {
			if (texture->_sdlTexture) {
				// Don't expect this to happen very often. Keep an eye on it.
				SDL_DestroyTexture(texture->_sdlTexture);
			}
			texture->_sdlTexture = SDL_CreateTextureFromSurface(_sdlRenderer, surface);
		}
		texture->_generation = generation;
		SDL_FreeSurface(surface);
	}
}

void TextureManager::freeAll()
{
	_pool.WaitforAll();
	update();
	for (Texture* t : _textures) {
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

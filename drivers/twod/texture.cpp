#include "texture.h"
#include "xform.h"

#include "../task.h"

#include <SDL_image.h>

#include <fmt/core.h>

#define DEBUG_TEXTURES 0

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
#if DEBUG_TEXTURES
		fmt::print("TextureLoadTask -> queue: {} {} {} {}\n", _texture->_name, _texture->_w, _texture->_h, _texture->_bytes);
#endif
		_queue->push(update);
	}

	std::shared_ptr<Texture> _texture;
	TextureLoadQueue* _queue = nullptr;
	int _generation = 0;
};

Texture::~Texture()
{
#if DEBUG_TEXTURES
	fmt::print("~Texture() delete: {}\n", _name);
#endif
	SDL_DestroyTexture(_sdlTexture);
}

TextureManager::~TextureManager()
{
	freeAll();
}

std::shared_ptr<Texture> TextureManager::loadTexture(const std::string& path)
{
	auto it = std::find_if(_textures.begin(), _textures.end(), [&path](const auto& t) { return t->path() == path; });
	if (it != _textures.end()) {
		return *it;
	}

#if DEBUG_TEXTURES
	fmt::print("loadTexture: {}\n", name);
#endif
	Texture* texture = new Texture();
	texture->_path = path;
	auto ptr = std::shared_ptr<Texture>(texture);

	TextureLoadTask* task = new TextureLoadTask();
	task->_texture = ptr;
	task->_queue = &_loadQueue;
	task->_generation = ++_generation;

	_pool.AddTaskSetToPipe(task);

	_textures.push_back(ptr);
	return ptr;
}

std::shared_ptr<Texture> TextureManager::createTextField(int w, int h)
{
#if DEBUG_TEXTURES
	fmt::print("createTextField: {}\n", name);
#endif
	Texture* texture = new Texture();
	texture->_w = w;
	texture->_h = h;
	texture->_bytes = 4;
	texture->_textField = true;
	texture->_sdlTexture = SDL_CreateTexture(_sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	//texture->_manager = this;

	auto ptr = std::shared_ptr<Texture>(texture);
	_textures.push_back(ptr);
	return ptr;
}

TextureManager::TextureManager(enki::TaskScheduler& pool, SDL_Renderer* renderer)
	: _pool(pool), _sdlRenderer(renderer)
{
}

void TextureManager::update()
{ 
	TextureUpdate update;
	while(_loadQueue.tryPop(update))
	{
		std::shared_ptr<Texture> texture = update.texture;
		SDL_Surface* surface = update.surface;
		int generation = update.generation;

		if (generation < texture->_generation) {
#if DEBUG_TEXTURES
			fmt::print("update() discard {} of {} on {}\n", generation, texture->_generation, texture->name());
#endif
			// This is an old surface. Discard it.
			SDL_FreeSurface(surface);
			continue;
		}

		if (texture->_textField) {
#if DEBUG_TEXTURES
			fmt::print("update() streaming to {}\n", texture->name());
#endif
			assert(texture->_sdlTexture);	// fixed size - should always be there.
			SDL_Surface* target = nullptr;
			// Do not free 'target`: done be the unlock
			int err = SDL_LockTextureToSurface(texture->_sdlTexture, nullptr, &target);
			assert(err == 0);
			assert(target);

			if (update.hqOpaque) {
				SDL_FillRect(target, nullptr, SDL_MapRGBA(target->format, update.bg.r, update.bg.g, update.bg.b, 255));
			}
			else {
				memset(target->pixels, 0, target->pitch * target->h);
			}
			SDL_BlitSurface(surface, nullptr, target, nullptr);
			SDL_UnlockTexture(texture->_sdlTexture);
		}
		else {
			if (texture->_sdlTexture) {
#if DEBUG_TEXTURES
				fmt::print("update() DESTROY {}\n", texture->name());
#endif
				// Don't expect this to happen very often. Keep an eye on it.
				SDL_DestroyTexture(texture->_sdlTexture);
			}
#if DEBUG_TEXTURES
			fmt::print("update() create {}\n", texture->name());
#endif
			texture->_sdlTexture = SDL_CreateTextureFromSurface(_sdlRenderer, surface);
		}
		texture->_generation = generation;
		SDL_FreeSurface(surface);
	}
	for (auto& t : _textures) {
		t->_age++;
	}

	// If only this has a reference, and it's old, free it.
	_textures.erase(std::remove_if(_textures.begin(), _textures.end(), [](auto& t) 
		{ 
			return t->_age > kMaxAge && t.use_count() == 1;
		}),
		_textures.end());
}

void TextureManager::freeAll()
{
	_textures.clear();
}

void DrawTestPattern(SDL_Renderer* renderer, int w, int h, int size, SDL_Color c1, SDL_Color c2, const XFormer& xf)
{
	PreserveColor pc(renderer);

	int ni = w / size;
	int nj = h / size;
	for (int j = 0; j < nj; j++) {
		for(int i=0; i<ni; i++) {
			SDL_Color c = (i+j) % 2 == 0 ? c1 : c2;
			SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
			SDL_Rect rect = xf.t(SDL_Rect{i*size, j*size, size, size});
			SDL_RenderFillRect(renderer, &rect);
		}
	}
}

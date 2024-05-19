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

		// This path stuff is here just to get it off the main thread.
		std::filesystem::path path = _texture->_path;
		std::filesystem::path ext = path.extension();
		std::string extS = lurp::toLower(ext.string());

		if (extS == ".png" || extS == ".jpg" || extS == ".qoi") {
			// we have a supported, qualified path. do nothing.
		}
		else {
			path.replace_extension(".qoi");
			if (!std::filesystem::exists(path)) {
				path.replace_extension(".png");
				if (!std::filesystem::exists(path)) {
					path.replace_extension(".jpg");
				}
			}
		}

		SDL_Surface* surface = IMG_Load(path.string().c_str());
		assert(surface);
		assert(_queue);
		_texture->_size = Size({ surface->w, surface->h });
		_texture->_bytes = surface->format->BytesPerPixel;

		TextureUpdate update{ _texture, surface, _generation };
#if DEBUG_TEXTURES
		fmt::print("TextureLoadTask -> queue: '{}' {}x{}\n", _texture->path(), _texture->width(), _texture->height());
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
	fmt::print("~Texture() delete: '{}'\n", path());
#endif
	SDL_DestroyTexture(_sdlTexture);
}

TextureManager::~TextureManager()
{
	freeAll();
}

std::shared_ptr<Texture> TextureManager::loadTexture(const std::filesystem::path& path)
{
	auto it = std::find_if(_textures.begin(), _textures.end(), [&path](const auto& t) { return t->path() == path.string(); });
	if (it != _textures.end()) {
		return *it;
	}

	Texture* texture = new Texture();
	texture->_path = path.string();
#if DEBUG_TEXTURES
	fmt::print("loadTexture: '{}'\n", texture->path());
#endif
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
	Texture* texture = new Texture();
	texture->_size = Size{ w, h };
	texture->_bytes = 4;
	texture->_textField = true;
	texture->_sdlTexture = SDL_CreateTexture(_sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	texture->_path = fmt::format("field:{}x{}", w, h);

#if DEBUG_TEXTURES
	fmt::print("createTextField: '{}'\n", texture->path());
#endif
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
			fmt::print("update() discard {} of {} on '{}'\n", generation, texture->_generation, texture->path());
#endif
			// This is an old surface. Discard it.
			SDL_FreeSurface(surface);
			continue;
		}

		if (texture->_textField) {
#if DEBUG_TEXTURES
			fmt::print("update() streaming to '{}'\n", texture->path());
#endif
			assert(texture->_sdlTexture);	// fixed size - should always be there.
			SDL_Surface* target = nullptr;
			// Do not free 'target`: done by the unlock
			int err = SDL_LockTextureToSurface(texture->_sdlTexture, nullptr, &target);
			assert(err == 0);
			assert(target);

			if (update.hqOpaque) {
				SDL_FillRect(target, nullptr, SDL_MapRGBA(target->format, update.bg.r, update.bg.g, update.bg.b, 255));
			}
			else {
				memset(target->pixels, 0, target->pitch * target->h);
			}
			if (surface) {
				SDL_Rect r{ 0, 0, std::min(surface->w, texture->width()), std::min(surface->h, texture->height()) };
				SDL_BlitSurface(surface, &r, target, &r);
			}
			SDL_UnlockTexture(texture->_sdlTexture);
		}
		else {
			if (texture->_sdlTexture) {
#if DEBUG_TEXTURES
				fmt::print("update() DESTROY '{}'\n", texture->path());
#endif
				// Don't expect this to happen very often. Keep an eye on it.
				SDL_DestroyTexture(texture->_sdlTexture);
			}
#if DEBUG_TEXTURES
			fmt::print("update() create '{}'\n", texture->path());
#endif
			texture->_sdlTexture = SDL_CreateTextureFromSurface(_sdlRenderer, surface);
		}
		texture->_generation = generation;
		texture->_surfaceSize = surface ? Size{ surface->w, surface->h } : Size{ 0, 0 };
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

void Draw(SDL_Renderer* renderer,
	std::shared_ptr<Texture> texture,
	const SDL_Rect* src,
	const SDL_Rect* dst,
	RenderQuality quality,
	double alpha)
{
	assert(renderer);
	if (!texture) return;
	if (!texture->ready()) return;
	if (alpha <= 0.001) return;

	SDL_ScaleMode mode = SDL_ScaleMode::SDL_ScaleModeBest;
	switch (quality) {
	case RenderQuality::kFullscreen:
	case RenderQuality::kBlit:
	case RenderQuality::kBest:
		mode = SDL_ScaleMode::SDL_ScaleModeBest;
		break;
	case RenderQuality::kText:
	case RenderQuality::kNearest:
		mode = SDL_ScaleMode::SDL_ScaleModeNearest;
		break;
	case RenderQuality::kLinear:
		mode = SDL_ScaleMode::SDL_ScaleModeLinear;
		break;
	}
	uint8_t a8 = (Uint8)lurp::clamp(round(alpha * 255), 0.0, 255.0);
	if (a8 == 0)
		return;

	SDL_SetTextureScaleMode(texture->sdlTexture(), mode);
	SDL_SetTextureBlendMode(texture->sdlTexture(), SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(texture->sdlTexture(), a8);
	SDL_RenderCopy(renderer, texture->sdlTexture(), src, dst);
}

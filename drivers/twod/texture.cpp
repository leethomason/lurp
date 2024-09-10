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
		assert(_texture->_type == Texture::Type::texture);

		// FIXME: use ConstructPath()
		// FIXME: check that ConstructPath() is not called before the TextureLoadTask
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
		_texture->_size = lurp::Size({ surface->w, surface->h });

#if DEBUG_TEXTURES
		fmt::print("TextureLoadTask -> queue: '{}' {}x{}\n", _texture->path(), _texture->width(), _texture->height());
#endif
		TextureUpdateMsg updateMsg{ _texture, surface, _generation };
		_queue->push(updateMsg);
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

	fireTextureTask(ptr);
	_textures.push_back(ptr);
	return ptr;
}

std::shared_ptr<Texture> TextureManager::createStreaming(int w, int h)
{
	Texture* texture = new Texture();
	texture->_size = lurp::Size{ w, h };
	texture->_type = Texture::Type::streaming;
	// There's nothing to load, so create the texture here on the main thread.
	texture->_sdlTexture = SDL_CreateTexture(_sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	texture->_path = fmt::format("field:{}x{}", w, h);

#if DEBUG_TEXTURES
	fmt::print("createTextField: '{}'\n", texture->path());
#endif
	auto ptr = std::shared_ptr<Texture>(texture);
	_textures.push_back(ptr);
	return ptr;
}

void TextureManager::reload()
{
	for (auto& t : _textures) {
		if (t->_type == Texture::Type::texture)
			fireTextureTask(t);
	}
}

TextureManager::TextureManager(enki::TaskScheduler& pool, SDL_Renderer* renderer)
	: _pool(pool), _sdlRenderer(renderer)
{
}

void TextureManager::fireTextureTask(std::shared_ptr<Texture> texture)
{
	TextureLoadTask* task = new TextureLoadTask();
	task->_texture = texture;
	task->_queue = &_loadQueue;
	task->_generation = ++_generation;

	_pool.AddTaskSetToPipe(task);
}

void TextureManager::updateStreaming(const TextureUpdateMsg& updateMsg, std::shared_ptr<Texture>& texture, const XFormer& xf)
{
#	if DEBUG_TEXTURES
	fmt::print("update() streaming to '{}'\n", texture->path());
#	endif
	assert(texture->_sdlTexture);	// fixed size - should always be there.

	SDL_Surface* target = nullptr;
	// Do not free 'target`: done by the unlock. Sort of a weird API.
	int err = SDL_LockTextureToSurface(texture->_sdlTexture, nullptr, &target);
	assert(err == 0);
	assert(target);

	if (updateMsg.hqOpaque) {
		SDL_FillRect(target, nullptr, SDL_MapRGBA(target->format, updateMsg.bg.r, updateMsg.bg.g, updateMsg.bg.b, 255));
	}
	else {
		memset(target->pixels, 0, target->pitch * target->h);
	}

	SDL_Rect surfaceSize{ 0, 0, 0, 0 };
	int y = 0;
	for (size_t i = 0; i < updateMsg.textVec.size(); i++) {
		SDL_Surface* surface = updateMsg.textVec[i].surface;
		if (surface) {
			SDL_Rect src{ 0, 0, surface->w, surface->h };
			SDL_Rect dst{ 0, y, surface->w, surface->h };

			src = IntersectRect(src, SDL_Rect{ 0, 0, texture->width(), texture->height() });
			dst = IntersectRect(dst, SDL_Rect{ 0, 0, texture->width(), texture->height() });
			src.w = dst.w;
			src.h = dst.h;

			int e = SDL_BlitSurface(surface, &src, target, &dst);
			assert(e == 0);
			int space = xf.s(updateMsg.textVec[i].virtualSpace);
			dst.h += space;
			surfaceSize = UnionRect(surfaceSize, dst);
			y += surface->h + space;

			SDL_FreeSurface(surface);
		}
	}
	texture->_surfaceSize = lurp::Size{ surfaceSize.w, surfaceSize.h };
	SDL_UnlockTexture(texture->_sdlTexture);
}

void TextureManager::updateTexture(const TextureUpdateMsg& updateMsg, std::shared_ptr<Texture>& texture)
{
	SDL_Surface* surface = updateMsg.surface;
	if (texture->_sdlTexture) {
		assert(surface);
#				if DEBUG_TEXTURES
		fmt::print("update() DESTROY '{}'\n", texture->path());
#				endif
		// Don't expect this to happen very often. Keep an eye on it.
		SDL_DestroyTexture(texture->_sdlTexture);
	}
#			if DEBUG_TEXTURES
	fmt::print("update() create '{}'\n", texture->path());
#			endif
	texture->_sdlTexture = SDL_CreateTextureFromSurface(_sdlRenderer, surface);
	texture->_surfaceSize = { surface->w, surface->h };
	SDL_FreeSurface(surface);
}

void TextureManager::update(const XFormer& xf)
{
	TextureUpdateMsg updateMsg;
	while (_loadQueue.tryPop(updateMsg))
	{
		std::shared_ptr<Texture> texture = updateMsg.texture;

		if (updateMsg.generation < texture->_generation) {
#			if DEBUG_TEXTURES
			fmt::print("update() discard {} of {} on '{}'\n", updateMsg.generation, texture->_generation, texture->path());
#			endif
			// This is an old surface. Discard it.
			SDL_FreeSurface(updateMsg.surface);
			for(size_t i=0; i<updateMsg.textVec.size(); i++) {
				SDL_FreeSurface(updateMsg.textVec[i].surface);
			}
			continue;
		}

		if (texture->_type == Texture::Type::streaming) {
			updateStreaming(updateMsg, texture, xf);
		}
		else {
			updateTexture(updateMsg, texture);
		}
		texture->_generation = updateMsg.generation;
	}
	for (auto& t : _textures) {
		t->_age++;
	}

	// If only this has a reference, and it's old, free it.
	// Note that streaming textures don't auto-regen, so they can't be discarded.
	_textures.erase(std::remove_if(_textures.begin(), _textures.end(), [](auto& t)
		{
			return t->_type == Texture::Type::texture && t->_age > kMaxAge && t.use_count() == 1;
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

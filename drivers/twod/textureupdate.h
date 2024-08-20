#pragma once

#include "util.h"
#include "SDL.h"

class Texture;
struct SDL_Surface;

struct TextureUpdateMsg {
	struct TextUpdate {
		SDL_Surface* surface = nullptr;
		int virtualSpace = 0;
	};

	std::shared_ptr<Texture> texture;
	SDL_Surface* surface = nullptr;
	int generation = 0;
	bool hqOpaque = false;
	SDL_Color bg = SDL_Color{ 0, 0, 0, 0 };
	std::vector<TextUpdate> textVec;
};

using TextureLoadQueue = lurp::QueueMT<TextureUpdateMsg>;


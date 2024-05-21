#pragma once

#include "util.h"
#include "SDL.h"

class Texture;
struct SDL_Surface;

struct TextureUpdate {
	std::shared_ptr<Texture> texture;
	SDL_Surface* surface = nullptr;
	int generation = 0;
	bool hqOpaque = false;
	SDL_Color bg = SDL_Color{ 0, 0, 0, 0 };
	std::vector<SDL_Surface*> surfaceVec;
};
using TextureLoadQueue = lurp::QueueMT<TextureUpdate>;


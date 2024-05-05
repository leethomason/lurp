#pragma once

#include "util.h"

class Texture;
struct SDL_Surface;

struct TextureUpdate {
	Texture* texture = nullptr;
	SDL_Surface* surface = nullptr;
	int generation = 0;
};
using TextureLoadQueue = lurp::QueueMT<TextureUpdate>;


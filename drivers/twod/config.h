#pragma once

#include "xform.h"
#include "texture.h"

#include "SDL.h"

#include <string>
#include <vector>

struct GameRegion {
	std::string name;
	Rect position;
	
	enum class Type {
		image,
		text,
		hqText
	};
	Type type;
	SDL_Color bgColor;

	std::shared_ptr<Texture> texture;
	std::string text;
};

struct GameConfig {
	Point virtualSize;
	SDL_Color backgroundColor;

	std::vector<GameRegion> regions;
	std::vector<std::shared_ptr<Texture>> openingTitles;
	std::shared_ptr<Texture> menuBackground;
};

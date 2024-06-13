#pragma once

#include "geom.h"
#include "config.h"

#include <string>
#include <vector>
#include <filesystem>

struct Font;

struct GameRegion {
	std::string name;
	lurp::Rect position = { 0, 0, 1920, 1080 };
	
	enum class Type {
		kNone,
		kImage,
		kText,
		kInfo,					// fixme: how to handle this?
	};
	Type type = Type::kNone;
	std::string imagePath;		// default image - can be changed by the game
	lurp::Color bgColor = { 0, 0, 0, 255 };
};

struct GameConfig2D {
	lurp::GameConfig config;

	lurp::Point virtualSize = { 1920, 1080 };
	lurp::Color textColor = { 255, 255, 255, 255 };
	lurp::Color optionColor = { 0, 148, 255, 255 };
	lurp::Color backgroundColor = { 0, 0, 0, 255 };

	std::vector<GameRegion> regions;
	std::vector<std::string> openingTitles;
	std::string mainBackground;
	std::string settingsBackground;
	std::string saveLoadBackground;

	const Font* font = nullptr;

	static GameConfig2D demoConfig2D();
};

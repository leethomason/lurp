#pragma once

#include "xform.h"
#include "texture.h"

#include "SDL.h"

#include <string>
#include <vector>
#include <filesystem>

struct Font;

struct GameRegion {
	std::string name;
	Rect position = { 0, 0, 1920, 1080 };
	
	enum class Type {
		kNone,
		kImage,
		kText,
		kInfo,					// fixme: how to handle this?
	};
	Type type = Type::kNone;
	std::string imagePath;		// default image - can be changed by the game
	SDL_Color bgColor = SDL_Color{ 0, 0, 0, 0 };
};

struct GameConfig {
	Point virtualSize = { 1920, 1080 };
	SDL_Color textColor = { 255, 255, 255, 255 };
	SDL_Color backgroundColor = { 0, 0, 0, 255 };

	std::vector<GameRegion> regions;
	std::vector<std::string> openingTitles;
	std::string mainBackground;
	std::string settingsBackground;
	std::string saveLoadBackground;

	std::filesystem::path assetsDir;
	std::string scriptFile;
	std::string startingZone;

	const Font* font = nullptr;

	static GameConfig demoConfig();
	bool validate() const;

private:
	bool validatePath(const std::filesystem::path& dir, const std::string& stem) const;
};

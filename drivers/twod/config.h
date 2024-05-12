#pragma once

#include "xform.h"
#include "texture.h"

#include "SDL.h"

#include <string>
#include <vector>
#include <filesystem>

struct GameRegion {
	std::string name;
	Rect position = { 0, 0, 1920, 1080 };
	
	enum class Type {
		kNone,
		kConstImage,
		kText,
		kOpaqueText,
	};
	Type type = Type::kNone;
	std::string imagePath;		// kConstImage
	SDL_Color bgColor;
};

struct GameConfig {
	Point virtualSize = { 1920, 1080 };
	SDL_Color backgroundColor = { 0, 0, 0, 255 };

	std::vector<GameRegion> regions;
	std::vector<std::string> openingTitles;
	std::string mainBackground;
	std::string settingsBackground;
	std::string saveLoadBackground;

	std::filesystem::path assetsDir;
	std::string scriptFile;
	std::string startingZone;

	static GameConfig demoConfig();
	bool validate() const;

private:
	bool validatePath(const std::filesystem::path& dir, const std::string& stem) const;
};

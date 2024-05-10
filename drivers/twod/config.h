#pragma once

#include "xform.h"
#include "texture.h"

#include "SDL.h"

#include <string>
#include <vector>
#include <filesystem>

struct GameRegion {
	std::string name;
	Rect position = { 0, 0, 1920, 1080 }; // (960x540 at half scale)
	
	enum class Type {
		image,
		text,
		opaqueText,
	};
	Type type;
	SDL_Color bgColor;

	std::shared_ptr<Texture> texture;
	std::string text;
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

	static GameConfig demoConfig();
	bool validate() const;
//	bool getPath(const std::string& asset, std::filesystem::path& p) const;

private:
	bool validatePath(const std::filesystem::path& dir, const std::string& stem) const;
};

#pragma once

#include "geom.h"
#include "config.h"

#include <string>
#include <vector>
#include <filesystem>

struct Font;

namespace lurp {
	class ScriptBridge;
}

struct GameRegion {
	std::string name;							// "image", "text", "info" are special names
	lurp::Rect position = { 0, 0, 1920, 1080 };
	std::string image;							// default image - can be changed by the game

	lurp::Color textColor = { 0, 0, 0, 0 };		// {0, 0, 0, 0} uses the default	
	lurp::Color bgColor = { 0, 0, 0, 0 };		// if a < 255, then will use blend text, else opaque
};

struct GameConfig2D {
	GameConfig2D(const lurp::GameConfig& c) : config(c) {}

	lurp::GameConfig config;

	// File
	lurp::Point size = { 1920, 1080 };
	lurp::Color textColor = { 255, 255, 255, 255 };
	lurp::Color optionColor = { 0, 148, 255, 255 };
	lurp::Color choiceColor = { 0, 148, 255, 255 };
	lurp::Color backgroundColor = { 0, 0, 0, 255 };

	std::string font;
	std::string uiFont;

	std::vector<GameRegion> regions;
	std::vector<std::string> openingTitles;
	std::string mainBackground;
	std::string settingsBackground;
	std::string saveLoadBackground;

	// Derived
	const Font* font = nullptr;

	static GameConfig2D demoConfig2D(const lurp::GameConfig& c);
	void load(const lurp::ScriptBridge& bridge);
};

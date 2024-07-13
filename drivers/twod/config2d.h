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
	// special region names
	static constexpr char kImage[] = "image";
	static constexpr char kText[] = "text";
	static constexpr char kInfo[] = "info";

	std::string name;
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

	lurp::Color navNormalColor = { 0, 148, 230, 255 };
	lurp::Color navOverColor = { 0, 148, 255, 255 };
	lurp::Color navDownColor = { 0, 148, 240, 255 };
	lurp::Color navDisabledColor = { 0, 74, 128, 255 };

	lurp::Color optionNormalColor = { 0, 148, 230, 255 };
	lurp::Color optionOverColor = { 0, 148, 255, 255 };
	lurp::Color optionDownColor = { 0, 148, 240, 255 };
	lurp::Color optionDisabledColor = { 0, 74, 128, 255 };

	lurp::Color backgroundColor = { 0, 0, 0, 255 };

	std::string fontName;
	int fontSize = 28;
	std::string uiFont;
	int uiFontSize = 48;

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

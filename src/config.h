#pragma once

#include "geom.h"

#include <string>
#include <vector>
#include <filesystem>
#include <array>

namespace lurp {

struct GameConfig {
	std::vector<std::filesystem::path> assetsDirs;
	std::filesystem::path scriptFile;		// game/chullu/chulla.lua
	std::string			  startingZone;		// SAN_FRANCISCO
	std::string			  gameName;			// chullu
};

} // namespace lurp
#pragma once

#include "geom.h"

#include <string>
#include <vector>
#include <filesystem>
#include <array>

namespace lurp {

struct GameConfig {
	std::vector<std::filesystem::path> assetsDirs;
	std::string scriptFile;
	std::string startingZone;
};

} // namespace lurp
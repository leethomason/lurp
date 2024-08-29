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

	//bool validatePath(const std::filesystem::path& dir, const std::string& stem) const;
	//std::filesystem::path getAssetPath(const std::string& asset) const;
};

} // namespace lurp
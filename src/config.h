#pragma once

#include "geom.h"

#include <string>
#include <vector>
#include <filesystem>

namespace lurp {

struct GameConfig {
	std::filesystem::path assetsDir;
	std::string scriptFile;
	std::string startingZone;

	bool validatePath(const std::filesystem::path& dir, const std::string& stem) const;
};

} // namespace lurp
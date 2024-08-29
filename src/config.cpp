#include "config.h"

#include <fmt/core.h>
#include <plog/Log.h>

namespace lurp {

#if 0
bool GameConfig::validatePath(const std::filesystem::path& dir, const std::string& stem) const
{
	// Order:
	//		.qoi	assumed to be a fast loading version of png
	//		.png    most files
	//		.jpg    not the best format, but we'll support it

	std::filesystem::path p = dir / (stem + ".qoi");
	if (std::filesystem::exists(p)) {
		return true;
	}
	p = dir / (stem + ".png");
	if (std::filesystem::exists(p)) {
		return true;
	}
	p = dir / (stem + ".jpg");
	if (std::filesystem::exists(p)) {
		return true;
	}

	PLOG(plog::warning) << "Asset does not exist: '" << stem << "' in directory '" << dir.string() << "'";
	assert(false);
	return false;
}
#endif

#if 0
std::filesystem::path GameConfig::getAssetPath(const std::string& asset) const
{
	for (const auto& dir : assetsDirs) {
		std::filesystem::path p = dir / asset;
		if (std::filesystem::exists(p)) {
			return p;
		}
	}

	PLOG(plog::warning) << "Asset does not exist: '" << asset << "'";
	assert(false);
	return {};
}
#endif

} // namespace lurp
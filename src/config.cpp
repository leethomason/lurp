#include "config.h"

#include <fmt/core.h>
#include <plog/Log.h>

namespace lurp {

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

} // namespace lurp
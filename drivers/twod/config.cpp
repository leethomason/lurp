#include "config.h"

#include <fmt/core.h>
#include <plog/Log.h>

/*static*/ GameConfig GameConfig::demoConfig()
{
	GameConfig c;
	c.openingTitles = { "title1", "title2"};
	c.mainBackground = "main";

	return c;
}

#if 0
/*static*/bool GameConfig::getPath(const std::string& asset, std::filesystem::path& p)
{

}
#endif

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

bool GameConfig::validate() const
{
	bool okay = true;
	for (const auto& s : openingTitles)
		okay &= validatePath(assetsDir, s);
	if (!mainBackground.empty())
		okay &= validatePath(assetsDir, mainBackground);
	if (!settingsBackground.empty())
		okay &= validatePath(assetsDir, settingsBackground);
	if (!saveLoadBackground.empty())
		okay &= validatePath(assetsDir, saveLoadBackground);
	return okay;
}

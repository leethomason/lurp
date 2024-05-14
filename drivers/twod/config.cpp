#include "config.h"

#include <fmt/core.h>
#include <plog/Log.h>

/*static*/ GameConfig GameConfig::demoConfig()
{
	GameConfig c;
	c.openingTitles = { "title1", "title2"};
	c.mainBackground = "main";

	// Default 1920 x 1080
	// inset 30 pixel border
	// 3 frames (image, text, info)
	//   * x = 30, 568 x 1020
	//   * x = 600, 800 x 1020
	//   * x = 1402, 488 x 1020
	// But each frame has a 20 pixel border, yielding:
	//   * x = 50, 528 x 980
	//   * x = 620, 760 x 980
	//   * x = 1422, 448 x 980

	GameRegion background;
	background.name = "background";
	background.position = { 0, 0, 1920, 1080 };
	background.type = GameRegion::Type::kImage;
	background.imagePath = "game";
	c.regions.push_back(background);

	GameRegion image;
	image.name = "image";
	image.position = { 50, 50, 528, 980 };
	image.type = GameRegion::Type::kImage;
	c.regions.push_back(image);

	GameRegion text;
	text.name = "text";
	text.position = { 620, 50, 760, 980 };
	text.type = GameRegion::Type::kOpaqueText;
	text.bgColor = { 64, 64, 64, 255 };
	c.regions.push_back(text);

	GameRegion info;
	info.name = "info";
	info.position = { 1422, 50, 448, 980 };
	info.type = GameRegion::Type::kInfo;
	c.regions.push_back(info);

	return c;
}

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

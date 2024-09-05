#include "config2d.h"
#include "scriptbridge.h"

#include <fmt/core.h>
#include <plog/Log.h>

/*static*/ GameConfig2D GameConfig2D::demoConfig2D(const lurp::GameConfig& c1)
{
	GameConfig2D c(c1);
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
	background.image = "game";
	c.regions.push_back(background);

	GameRegion image;
	image.name = "image";
	image.position = { 50, 50, 528, 980 };
	c.regions.push_back(image);

	GameRegion text;
	text.name = "text";
	text.position = { 620, 50, 760, 980 };
	text.bgColor = { 64, 64, 64, 255 };
	c.regions.push_back(text);

	GameRegion info;
	info.name = "info";
	info.position = { 1422, 50, 448, 980 };
	info.bgColor = { 64, 64, 64, 255 };
	c.regions.push_back(info);

	return c;
}

void GameConfig2D::load(const lurp::ScriptBridge& bridge)
{
	openingTitles.clear();
	mainBackground.clear();
	settingsBackground.clear();
	saveLoadBackground.clear();	

	lua_State* L = bridge.getLuaState();
	lurp::ScriptBridge::LuaStackCheck check(L);

	bridge.pushGlobal("Config");
	if (!lua_istable(L, -1)) {
		PLOG(plog::warning) << "Config table not found";
		lua_pop(L, 1);
		return;
	}

	size = bridge.GetPointField("size", size);
	textColor = bridge.GetColorField("textColor", textColor);

	navNormalColor = bridge.GetColorField("navNormalColor", navNormalColor);
	navOverColor = bridge.GetColorField("navOverColor", navOverColor);
	navDownColor = bridge.GetColorField("navDownColor", navDownColor);
	navDisabledColor = bridge.GetColorField("navDisabledColor", navDisabledColor);
	
	optionNormalColor = bridge.GetColorField("optionNormalColor", optionNormalColor);
	optionOverColor = bridge.GetColorField("optionOverColor", optionOverColor);
	optionDownColor = bridge.GetColorField("optionDownColor", optionDownColor);
	optionDisabledColor = bridge.GetColorField("optionDisabledColor", optionDisabledColor);
	
	backgroundColor = bridge.GetColorField("backgroundColor", backgroundColor);

	fontName = bridge.getStrField("fontName", fontName);
	fontSize = bridge.getIntField("fontSize", fontSize);
	uiFont = bridge.getStrField("uiFont", uiFont);
	uiFontSize = bridge.getIntField("uiFontSize", uiFontSize);

	if (bridge.hasField("regions")) {
		regions.clear();

		for (lurp::TableIt it(L, -1); !it.done(); it.next()) {
			if (it.kType() == LUA_TNUMBER && it.vType() == LUA_TTABLE) {
				GameRegion r;
				r.name = lurp::ScriptBridge::getField(L, "", 1).str;
				r.position = bridge.GetRectField("pos", lurp::Rect{0, 0, 0, 0});
				r.image = bridge.getStrField("image", "");
				r.textColor = bridge.GetColorField("fg", r.textColor);
				r.bgColor = bridge.GetColorField("bg", r.bgColor);

				regions.push_back(r);
			}
		}
	}
	lua_pop(L, 1);
}

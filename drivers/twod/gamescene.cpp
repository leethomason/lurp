#include "gamescene.h"
#include "config.h"
#include "zonedriver.h"

void GameScene::load(Drawing& d, const FrameData& f)
{
	if (f.sceneFrame != 0) return;
	_data.resize(d.config.regions.size());

	_csassets = _bridge.readCSA(d.config.scriptFile);
	_assets = new lurp::ScriptAssets(_csassets);
	_zoneDriver = new lurp::ZoneDriver(*_assets, _bridge, d.config.startingZone, "player");
}

GameScene::~GameScene()
{
	delete _zoneDriver;
	delete _assets;
}

void GameScene::draw(Drawing&, const FrameData&, const XFormer&)
{

}

void GameScene::layoutGUI(nk_context*, float, const XFormer&)
{

}

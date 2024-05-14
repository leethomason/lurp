#include "gamescene.h"
#include "config.h"
#include "zonedriver.h"
#include "text.h"

void GameScene::load(Drawing& d, const FrameData& f)
{
	if (f.sceneFrame != 0) return;

	_data.resize(d.config.regions.size());

	for (size_t i = 0; i < d.config.regions.size(); ++i) {
		const GameRegion& r = d.config.regions[i];
		if (r.type == GameRegion::Type::kImage && !r.imagePath.empty()) {
			_data[i].texture = d.textureManager.loadTexture(d.config.assetsDir / r.imagePath);
		}	
		else if (r.type == GameRegion::Type::kText) {
			_data[i].textField = d.fontManager.createTextField(d.config.font, r.position.w, r.position.h, false, r.bgColor);
		}
		else if (r.type == GameRegion::Type::kOpaqueText) {
			_data[i].textField = d.fontManager.createTextField(d.config.font, r.position.w, r.position.h, true, r.bgColor);
		}
	}

	_csassets = _bridge.readCSA(d.config.scriptFile);
	_assets = new lurp::ScriptAssets(_csassets);
	_zoneDriver = new lurp::ZoneDriver(*_assets, _bridge, d.config.startingZone);
}

GameScene::~GameScene()
{
	delete _zoneDriver;
	delete _assets;
}

void GameScene::draw(Drawing& d, const FrameData&, const XFormer& x)
{
	process();

	for (size_t i = 0; i < d.config.regions.size(); ++i) {
		const GameRegion& r = d.config.regions[i];
		if (r.type == GameRegion::Type::kImage) {
			SDL_Rect dest = x.t(r.position).toSDLRect();
			Draw(d.renderer, _data[i].texture, nullptr, &dest, RenderQuality::kBlit);
		}
	}
}

void GameScene::layoutGUI(nk_context*, float, const XFormer&)
{

}

void GameScene::process()
{
	//if (_zoneDriver->mode() == lurp::ZoneDriver::Mode::kText) {

	//}
}

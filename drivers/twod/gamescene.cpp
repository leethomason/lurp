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
		else if (r.type == GameRegion::Type::kText || r.type == GameRegion::Type::kInfo) {
			if (r.bgColor.a < 255)
				_data[i].textField = d.fontManager.createTextField(d.config.font, r.position.w, r.position.h, false, r.bgColor);
			else
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
	process(d);

	for (size_t i = 0; i < d.config.regions.size(); ++i) {
		const GameRegion& r = d.config.regions[i];
		SDL_Rect dest = x.t(r.position).toSDLRect();

		if (_data[i].texture)
			Draw(d.renderer, _data[i].texture, nullptr, &dest, RenderQuality::kBlit);
		if (_data[i].textField)
			d.fontManager.draw(_data[i].textField, dest.x, dest.y);
	}
}

void GameScene::layoutGUI(nk_context*, float, const XFormer&)
{

}

std::pair<const GameRegion*, GameScene::GSData*> GameScene::getRegion(GameRegion::Type type, const std::vector<GameRegion>& regions)
{
	for (size_t i = 0; i < regions.size(); ++i) {
		const GameRegion& r = regions[i];
		if (r.type == type) {
			return { &r, &_data[i] };
		}
	}
	return { nullptr, nullptr };
}

void GameScene::process(Drawing& d)
{
	if (_zoneDriver->mode() == lurp::ZoneDriver::Mode::kText) {
		auto[region, data] = getRegion(GameRegion::Type::kText, d.config.regions);
		if (region && data) {
			//data->textField->setText(_zoneDriver->text().text);
			data->textField->setText("Hello there");
		}
	}
	{
		const std::string& zoneName = _zoneDriver->currentZone().name;
		auto[region, data] = getRegion(GameRegion::Type::kInfo, d.config.regions);
		if (region && data) {
			data->textField->setText(zoneName);
		}
	}
}

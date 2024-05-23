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
				_data[i].textField = d.fontManager.createTextBox(d.config.font, r.position.w, r.position.h, false);
			else
				_data[i].textField = d.fontManager.createTextBox(d.config.font, r.position.w, r.position.h, true);
			
			_data[i].textField->setColor(d.config.textColor);
			_data[i].textField->setBgColor(r.bgColor);
		}
	}

	_csassets = _bridge.readCSA(d.config.scriptFile);
	_assets = new lurp::ScriptAssets(_csassets);
	_zoneDriver = new lurp::ZoneDriver(*_assets, _bridge, d.config.startingZone);
	_needProcess = true;
}

GameScene::~GameScene()
{
	delete _zoneDriver;
	delete _assets;
}

void GameScene::draw(Drawing& d, const FrameData&, const XFormer& x)
{
	if (_needProcess) {
		process(d);
		_needProcess = false;
	}

	for (size_t i = 0; i < d.config.regions.size(); ++i) {
		const GameRegion& r = d.config.regions[i];

		if (_data[i].texture) {
			Rect rDst = x.t(r.position);
			SDL_Rect dst{ rDst.x, rDst.y, rDst.w, rDst.h };
			Draw(d.renderer, _data[i].texture, nullptr, &dst, RenderQuality::kBlit);
		}
		if (_data[i].textField) {
			Point pDst = x.t(Point{ r.position.x, r.position.y });
			d.fontManager.Draw(_data[i].textField, pDst.x, pDst.y);
		}
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
	lurp::ZoneDriver::Mode mode = _zoneDriver->mode();

	if (mode == lurp::ZoneDriver::Mode::kNavigation) {
		auto [region, data] = getRegion(GameRegion::Type::kText, d.config.regions);
		if (region && data) {
			std::shared_ptr<TextBox> tb = data->textField;

			std::vector<lurp::DirEdge> dirEdges = _zoneDriver->dirEdges();
			tb->resize(dirEdges.size());

			std::string text;
			for (size_t i = 0; i < dirEdges.size(); ++i) {
				const lurp::DirEdge& de = dirEdges[i];
				if (de.dir == lurp::Edge::Dir::kUnknown)
					text = fmt::format("{}", de.name);
				else
					text = fmt::format("{}:\t{}", de.dirLong, de.name);
				tb->setText(i, text);
				tb->setColor(i, { 0, 255, 255, 255 });
				tb->setFont(i, d.config.font);
			}
		}
	}
	else if (mode == lurp::ZoneDriver::Mode::kText) {
		auto [region, data] = getRegion(GameRegion::Type::kText, d.config.regions);
		if (region && data) {
			std::shared_ptr<TextBox> tb = data->textField;
			tb->resize(1);
			tb->setText(_zoneDriver->text().text);
		}
	}

	// Side panel
	{
		auto[region, data] = getRegion(GameRegion::Type::kInfo, d.config.regions);
		if (region && data) {
			const lurp::Zone& zone = _zoneDriver->currentZone();
			const lurp::Room& room = _zoneDriver->currentRoom();

			data->textField->resize(3);

			data->textField->setText(0, zone.name);
			data->textField->setColor(0, {192, 192, 192, 255});

			data->textField->setText(1, room.name);
			data->textField->setColor(1, { 255, 255, 255, 255 });

			data->textField->setText(2, room.desc);
			data->textField->setColor(2, { 192, 192, 192, 255 });
		}
	}
}

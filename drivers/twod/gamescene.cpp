#include "gamescene.h"
#include "config.h"
#include "zonedriver.h"
#include "text.h"

void GameScene::load(Drawing& d, const FrameData& f)
{
	if (f.sceneFrame != 0) return;

	//_data.resize(d.config.regions.size());

	for (size_t i = 0; i < d.config.regions.size(); ++i) {
		const GameRegion& r = d.config.regions[i];
		if (r.type == GameRegion::Type::kImage && !r.imagePath.empty()) {
			_imageTexture = d.textureManager.loadTexture(d.config.assetsDir / r.imagePath);
		}	
		else if (r.type == GameRegion::Type::kText || r.type == GameRegion::Type::kInfo) {
			std::shared_ptr<TextBox>& tb = r.type == GameRegion::Type::kText ? _mainText : _infoText;

			if (r.bgColor.a < 255)
				tb = d.fontManager.createTextBox(d.config.font, r.position.w, r.position.h, false);
			else
				tb = d.fontManager.createTextBox(d.config.font, r.position.w, r.position.h, true);
			
			tb->setColor(d.config.textColor);
			tb->setBgColor(r.bgColor);
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

	const GameRegion* imageRegion = getRegion(GameRegion::Type::kImage, d.config.regions);
	if (imageRegion) {
		Rect rDst = x.t(imageRegion->position);
		SDL_Rect dst{ rDst.x, rDst.y, rDst.w, rDst.h };
		Draw(d.renderer, _imageTexture, nullptr, &dst, RenderQuality::kBlit);
	}

	const GameRegion* textRegion = getRegion(GameRegion::Type::kText, d.config.regions);
	if (textRegion) {
		_mainText->pos = x.t(Point{ textRegion->position.x, textRegion->position.y });
		d.fontManager.Draw(_mainText);
	}

	const GameRegion* infoRegion = getRegion(GameRegion::Type::kInfo, d.config.regions);
	if (infoRegion) {
		_infoText->pos = x.t(Point{ infoRegion->position.x, infoRegion->position.y });
		d.fontManager.Draw(_infoText);
	}

}

void GameScene::layoutGUI(nk_context*, float, const XFormer&)
{

}

const GameRegion* GameScene::getRegion(GameRegion::Type type, const std::vector<GameRegion>& regions)
{
	for (size_t i = 0; i < regions.size(); ++i) {
		const GameRegion& r = regions[i];
		if (r.type == type) {
			return &r;
		}
	}
	return nullptr;
}

void GameScene::process(Drawing& d)
{
	lurp::ZoneDriver::Mode mode = _zoneDriver->mode();

	if (mode == lurp::ZoneDriver::Mode::kNavigation) {
		const GameRegion* region = getRegion(GameRegion::Type::kText, d.config.regions);
		if (region) {
			std::shared_ptr<TextBox>& tb = _mainText;
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
		const GameRegion* region = getRegion(GameRegion::Type::kText, d.config.regions);
		if (region) {
			std::shared_ptr<TextBox>& tb = _mainText;
			tb->resize(1);
			tb->setText(_zoneDriver->text().text);
		}
	}

	// Side panel
	{
		const GameRegion* region = getRegion(GameRegion::Type::kInfo, d.config.regions);
		if (region) {
			const lurp::Zone& zone = _zoneDriver->currentZone();
			const lurp::Room& room = _zoneDriver->currentRoom();

			std::shared_ptr<TextBox>& tb = _infoText;
			tb->resize(3);

			tb->setText(0, zone.name);
			tb->setColor(0, {192, 192, 192, 255});

			tb->setText(1, room.name);
			tb->setColor(1, { 255, 255, 255, 255 });

			tb->setText(2, room.desc);
			tb->setColor(2, { 192, 192, 192, 255 });
		}
	}
}
